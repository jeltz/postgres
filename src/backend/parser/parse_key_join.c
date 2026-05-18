/*-------------------------------------------------------------------------
 *
 * parse_key_join.c
 *	  handle key joins in parser
 *
 * A key join is accepted only after parse analysis proves that its rewritten
 * equijoin satisfies these conditions over the named referencing and
 * referenced columns:
 *
 *	  1. The referenced values are unique.
 *	  2. Every non-null referencing value is contained in those referenced
 *		 values.
 *	  3. Referencing values are non-null, unless the join type preserves
 *		 that side.
 *
 * Surface facts are transient parser summaries attached to RangeTblEntry
 * nodes while proving FOR KEY joins.  They are computed on demand and are
 * not part of stored query semantics.
 *
 *
 * Portions Copyright (c) 1996-2026, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * IDENTIFICATION
 *	  src/backend/parser/parse_key_join.c
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "access/genam.h"
#include "access/htup_details.h"
#include "access/relation.h"
#include "catalog/dependency.h"
#include "catalog/index.h"
#include "catalog/indexing.h"
#include "catalog/pg_class.h"
#include "catalog/pg_constraint.h"
#include "catalog/pg_inherits.h"
#include "catalog/pg_operator.h"
#include "catalog/pg_proc.h"
#include "catalog/pg_type.h"
#include "nodes/makefuncs.h"
#include "nodes/nodeFuncs.h"
#include "nodes/parsenodes.h"
#include "optimizer/clauses.h"
#include "optimizer/optimizer.h"
#include "parser/parse_key_join.h"
#include "parser/parse_relation.h"
#include "parser/parsetree.h"
#include "rewrite/rewriteHandler.h"
#include "storage/lmgr.h"
#include "utils/builtins.h"
#include "utils/errcodes.h"
#include "utils/fmgroids.h"
#include "utils/lsyscache.h"
#include "utils/rel.h"
#include "utils/relcache.h"
#include "utils/syscache.h"

/*
 * KeyJoinColumn
 *
 *		Resolved form of one column named by a raw key-join clause.
 *		Tracks both the namespace item column and the exposed operand
 *		surface column used by proof facts.
 */
typedef struct KeyJoinColumn
{
	char	   *name;
	ParseNamespaceItem *nsitem;
	AttrNumber	nsattno;
	AttrNumber	surface_attno;
	ParseNamespaceColumn *nscol;
} KeyJoinColumn;

/*
 * KeyJoinMatch
 *
 *		Complete proof selected for one validated key join.
 *		The parser uses this to build equality quals and to record catalog
 *		dependencies in the stored KeyJoinNode.
 */
typedef struct KeyJoinMatch
{
	Oid			constraint;
	List	   *eqoperators;
	List	   *eqtypes;
	List	   *eqtypmods;
	List	   *notnulldeps;
	List	   *proofdeps;
} KeyJoinMatch;

/*
 * KeyJoinQueryStack
 *
 *		Stored query revalidation has no ParseState chain.  Keep the copied
 *		Query ownership stack explicitly so CTE RTEs can resolve ctelevelsup
 *		without guessing or exposing facts from the wrong WITH level.
 */
typedef struct KeyJoinQueryStack
{
	struct KeyJoinQueryStack *parent;
	Query	   *query;
} KeyJoinQueryStack;

/*
 * KeyJoinFactContext
 *
 *		Fact computation runs either during live parse analysis or while
 *		revalidating a copied stored Query.  Keep that mode explicit instead
 *		of spreading nullable ParseState/Query arguments through the proof
 *		code.
 */
typedef struct KeyJoinFactContext
{
	ParseState *pstate;
	Query	   *query;
	KeyJoinQueryStack *query_stack;
	bool		revalidating_stored_query;
} KeyJoinFactContext;

/*
 * FKFilterRemapContext
 *
 *		State for remapping proof-filter Params between key lists.
 */
typedef struct FKFilterRemapContext
{
	List	   *position_map;
} FKFilterRemapContext;

/* local function prototypes */
static bool find_key_join_match(RangeTblEntry *referencing_rte,
								RangeTblEntry *referenced_rte,
								List *referencing_attnums,
								List *referenced_attnums,
								bool need_notnull, KeyJoinMatch *match);
static Node *remap_filter_conjunct(Node *conjunct, List *position_map);
static bool filter_conjunct_can_remap(Node *conjunct, List *position_map);
static bool filter_conjunct_matches_key_positions(Node *conjunct,
												  List *keyPositions);
static bool filter_value_allowed(Node *node);
static void ensure_key_join_surface_facts(ParseState *pstate,
										  RangeTblEntry *rte);
static void ensure_key_join_surface_facts_internal(KeyJoinFactContext *context,
												   RangeTblEntry *rte);
static void compute_key_join_relation_facts(KeyJoinFactContext *context,
											RangeTblEntry *rte,
											Relation rel);
static KeyJoinSurfaceFacts *project_key_join_query_facts(KeyJoinFactContext *context,
														 Query *query);
static JoinExpr *find_join_expr_for_rtindex(KeyJoinFactContext *context,
											Index rtindex);
static void project_key_join_facts_from_rte(KeyJoinSurfaceFacts *dst,
											RangeTblEntry *src, List **attrmap,
											bool preserve_notnull,
											bool preserve_unique,
											bool preserve_rowcoverage,
											Node *filter_qual, Query *filter_query,
											Node *filter_jtnode, Index filter_rtindex,
											List *rowcoverage_key_position_sets,
											List *extra_unique_deps);
static List *make_rowcollapse_key_positions(Query *query, List *clauses);
static bool add_filter_conjuncts(List **dst, List *keyPositions,
								 Node *qual, Query *filter_query,
								 Node *filter_jtnode, Index filter_rtindex,
								 List **filter_attrmap, int filter_natts,
								 List **dependencies, bool strict);
static List *map_var_to_jtnode_surface(Query *query, Node *jtnode,
									   Index varno, AttrNumber attno);
static List *append_filter_expr_dependencies(List *dependencies, Node *node);
static bool filter_dependency_walker(Node *node, void *context_arg);
static List *add_op_function_deps(List *deps, Oid opno, Oid opfuncid);
static void compute_join_output_facts(JoinExpr *j, Index left_rtindex,
									  RangeTblEntry *left_rte,
									  Index right_rtindex,
									  RangeTblEntry *right_rte,
									  RangeTblEntry *joinrte,
									  KeyJoinFactContext *context);
static List *collect_key_join_nodes(Node *node);
static bool revalidate_stored_key_join_node_walker(Node *node, void *context);
static void revalidate_stored_key_join_proofs_in_query(Query *query,
													   KeyJoinQueryStack *parent_stack);
static void revalidate_query_jointree_proofs(Query *query, Node *jtnode,
											 KeyJoinQueryStack *query_stack);

/* local leaf functions */
static KeyJoinColumn *resolve_columns_on_nsitem(ParseState *pstate,
												ParseNamespaceItem *lookup,
												ParseNamespaceItem *surface,
												List *names, ParseLoc location,
												bool is_referencing);
static Var *make_var_from_nscolumn(ParseState *pstate,
								   ParseNamespaceColumn *nscol);
static bool join_preserves_side(JoinType jointype, bool leftside);
static Node *build_key_join_quals(List *referenced_args,
								  List *referencing_args,
								  List *eqoperators,
								  List *eqtypes,
								  List *eqtypmods,
								  List *locations,
								  ParseLoc default_location);
static bool select_key_position_parts(List *selected_attnums,
									  List *keyPositions, List *baseAttnums,
									  List **selected_base_attnums,
									  List **selected_key_positions);
static int	key_position_index_for_attnum(List *keyPositions, int attno);
static bool key_position_identity_lists_equal(List *left, List *right);
static bool key_position_identity_equal(KeyJoinKeyPosition *left,
										KeyJoinKeyPosition *right);
static bool int_lists_same_members(List *a, List *b);
static List *make_filter_position_map(List *src_base_attnums,
									  List *src_selected_base,
									  List *dst_base_attnums,
									  List *dst_selected_base);
static Node *remap_filter_param_mutator(Node *node, void *context_arg);
static bool filter_conjunct_unremappable_param_walker(Node *node,
													  void *context_arg);
static bool list_contains_equal_node(List *list, Node *node);
static List *append_dependencies_unique(List *dst, List *src);
static bool dependency_member(List *deps, Oid classId, Oid objectId,
							  int32 objectSubId);
static KeyJoinProofDependency *make_dependency(Oid classId, Oid objectId);
static Index rtindex_for_rte(KeyJoinFactContext *context,
							 RangeTblEntry *rte);
static JoinExpr *find_join_expr_in_jointree(Node *jtnode, Index rtindex);
static List *project_key_positions(List *keyPositions, List **attrmap);
static Index jtnode_surface_rtindex(Node *jtnode);
static List *append_join_input_mapping(RangeTblEntry *joinrte, bool leftside,
									   List *input_attnums);
static int	join_output_attno_for_input(RangeTblEntry *joinrte,
										bool leftside, int input_colno);
static Var *direct_var_from_node_allow_outer(Node *node);
static Var *direct_var_from_node(Node *node);
static Var *direct_filter_var_from_node(Node *node);
static Node *make_filter_param(KeyJoinKeyPosition *keypos, int pos);
static void append_filter_conjunct_unique(List **dst, Node *conjunct);
static List *append_filter_dependency(List *dependencies, Oid classId,
									  Oid objectId);
static List **build_join_attrmap(RangeTblEntry *joinrte, bool leftside,
								 int nattrs);
static bool join_null_extends_side(JoinType jointype, bool leftside);
static Node *join_filter_for_side(JoinType jointype, bool leftside,
								  Node *filter);
static bool stored_node_contains_key_join_walker(Node *node, void *context);
static bool collect_key_join_nodes_walker(Node *node, void *context);
static bool dependency_list_is_subset(List *candidate, List *superset);
static void extract_key_join_qual_arg(Node *qual, List **referenced_args,
									  List **referencing_args,
									  List **locations);
static bool key_join_surface_facts_has_facts(KeyJoinSurfaceFacts *set);
static KeyJoinFact *add_fact(KeyJoinSurfaceFacts *set,
							 KeyJoinFactKind kind);
static void add_paired_row_coverage(KeyJoinSurfaceFacts *set,
									List *keypositions, Oid relid,
									List *baseAttnums, List *deps);
static bool key_join_collation_is_usable(Oid collationOid);
static bool key_join_equality_operator_is_usable(Oid opno, Oid typeOid,
												 List **dependencies);
static Oid	key_join_equality_type(Oid typeOid, int32 typmod,
								   int32 *eqTypmod);
static List *make_key_positions_from_attrnums(TupleDesc tupdesc,
											  const AttrNumber *attnums,
											  int nattnums,
											  const Oid *eqOperators);
static KeyJoinKeyPosition *make_key_position(List *attnums, Oid typeOid,
											 int32 typmod, Oid collationOid,
											 Oid eqOperator);
static List *list_make_attrnums(const AttrNumber *attnums, int nattnums);

/*
 * transformAndValidateKeyJoin
 *
 *		Transform raw key-join syntax into proven join quals and a
 *		KeyJoinNode.
 *
 *		This resolves the named columns, ensures operand RTEs expose proof
 *		facts, proves the key join, and installs strict equality quals.
 *
 * Called by:
 *		no local callers
 */
void
transformAndValidateKeyJoin(ParseState *pstate, JoinExpr *j,
							ParseNamespaceItem *l_nsitem,
							ParseNamespaceItem *r_nsitem,
							List *l_namespace)
{
	KeyJoinClause *key_clause = castNode(KeyJoinClause, j->keyJoin);
	ParseNamespaceItem *ref_nsitem = NULL;
	bool		local_is_referencing = (key_clause->direction == KEY_JOIN_TO);
	bool		referencing_left = !local_is_referencing;
	ParseNamespaceItem *fk_surface = local_is_referencing ?
		r_nsitem : l_nsitem;
	ParseNamespaceItem *pk_surface = local_is_referencing ?
		l_nsitem : r_nsitem;
	KeyJoinColumn *local_cols;
	KeyJoinColumn *ref_cols;
	List	   *referencing_attnums = NIL;
	List	   *referenced_attnums = NIL;
	List	   *ref_alias_attnums = NIL;
	List	   *referencing_vars = NIL;
	List	   *referenced_vars = NIL;
	KeyJoinMatch match;
	KeyJoinNode *key_join;

	if (list_length(key_clause->localCols) != list_length(key_clause->refCols))
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_FOREIGN_KEY),
				 errmsg("key join column lists must have the same length"),
				 parser_errposition(pstate, key_clause->location)));

	/* The key-join alias must name exactly one visible relation on the left. */
	foreach_ptr(ParseNamespaceItem, nsitem, l_namespace)
	{
		Assert(nsitem->p_names != NULL);
		if (!nsitem->p_rel_visible ||
			strcmp(nsitem->p_names->aliasname, key_clause->refAlias) != 0)
			continue;
		if (ref_nsitem != NULL)
			ereport(ERROR,
					(errcode(ERRCODE_AMBIGUOUS_ALIAS),
					 errmsg("table reference \"%s\" is ambiguous",
							key_clause->refAlias),
					 parser_errposition(pstate, key_clause->location)));
		ref_nsitem = nsitem;
	}
	if (ref_nsitem == NULL)
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_TABLE),
				 errmsg("key join alias \"%s\" is not present in the left join operand",
						key_clause->refAlias),
				 parser_errposition(pstate, key_clause->location)));

	/*
	 * Local columns are named on the right operand itself.  The arrow alias,
	 * however, may name a visible item inside the left operand, so resolve
	 * against that item and map onto the whole left operand surface.
	 */
	local_cols = resolve_columns_on_nsitem(pstate, r_nsitem, r_nsitem,
										   key_clause->localCols,
										   key_clause->location,
										   local_is_referencing);
	ref_cols = resolve_columns_on_nsitem(pstate, ref_nsitem, l_nsitem,
										 key_clause->refCols,
										 key_clause->location,
										 !local_is_referencing);

	for (int i = 0; i < list_length(key_clause->localCols); i++)
	{
		Var		   *localvar = make_var_from_nscolumn(pstate,
													  local_cols[i].nscol);
		Var		   *refvar = make_var_from_nscolumn(pstate, ref_cols[i].nscol);
		KeyJoinColumn *fkcol = local_is_referencing ?
			&local_cols[i] : &ref_cols[i];
		KeyJoinColumn *pkcol = local_is_referencing ?
			&ref_cols[i] : &local_cols[i];

		ref_alias_attnums = lappend_int(ref_alias_attnums,
										ref_cols[i].nsattno);
		referencing_attnums = lappend_int(referencing_attnums,
										  fkcol->surface_attno);
		referenced_attnums = lappend_int(referenced_attnums,
										 pkcol->surface_attno);
		referencing_vars = lappend(referencing_vars,
								   local_is_referencing ? localvar : refvar);
		referenced_vars = lappend(referenced_vars,
								  local_is_referencing ? refvar : localvar);
	}

	ensure_key_join_surface_facts(pstate, fk_surface->p_rte);
	ensure_key_join_surface_facts(pstate, pk_surface->p_rte);

	/*
	 * Deliberately ignore j->joinFilter here: the proof is about the two
	 * operand surfaces before any join-local FILTER is applied.
	 */
	if (!find_key_join_match(fk_surface->p_rte, pk_surface->p_rte,
							 referencing_attnums, referenced_attnums,
							 !join_preserves_side(j->jointype, referencing_left),
							 &match))
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_FOREIGN_KEY),
				 errmsg("key join cannot be proven from available constraints"),
				 parser_errposition(pstate, key_clause->location)));

	/* Install the equality quals proven by condition 2. */
	j->quals = build_key_join_quals(referenced_vars, referencing_vars,
									match.eqoperators, match.eqtypes,
									match.eqtypmods, NIL,
									key_clause->location);

	key_join = makeNode(KeyJoinNode);
	key_join->direction = key_clause->direction;
	key_join->referencingVarno = fk_surface->p_rtindex;
	key_join->referencedVarno = pk_surface->p_rtindex;
	key_join->referencingAttnums = referencing_attnums;
	key_join->referencedAttnums = referenced_attnums;
	key_join->refAliasVarno = ref_nsitem->p_rtindex;
	key_join->refAliasAttnums = ref_alias_attnums;
	key_join->constraint = match.constraint;
	key_join->notNullConstraints = match.notnulldeps;
	key_join->proofDependencies = match.proofdeps;

	j->keyJoin = (Node *) key_join;
}

/*
 * resolve_columns_on_nsitem
 *
 *		Resolve key-join column names against an operand namespace item.
 *
 *		Reject names that are missing, ambiguous, or not exposed by the
 *		operand surface.
 *
 * Called by:
 *		transformAndValidateKeyJoin
 */
static KeyJoinColumn *
resolve_columns_on_nsitem(ParseState *pstate,
						  ParseNamespaceItem *lookup,
						  ParseNamespaceItem *surface, List *names,
						  ParseLoc location, bool is_referencing)
{
	int			ncols = list_length(names);
	int			surface_ncols = list_length(surface->p_names->colnames);
	KeyJoinColumn *cols = palloc0(sizeof(KeyJoinColumn) * ncols);
	int			i = 0;

	foreach_ptr(Node, namenode, names)
	{
		char	   *name = strVal(namenode);
		int			attno = 0;
		int			match = 0;

		foreach_ptr(Node, cnnode, lookup->p_names->colnames)
		{
			attno++;
			if (strcmp(strVal(cnnode), name) == 0)
			{
				cols[i].name = name;
				cols[i].nsitem = lookup;
				cols[i].nsattno = attno;
				cols[i].nscol = lookup->p_nscolumns + attno - 1;
				match++;
			}
		}
		if (match == 0)
			ereport(ERROR,
					(errcode(ERRCODE_UNDEFINED_COLUMN),
					 is_referencing ?
					 errmsg("referencing column \"%s\" does not exist", name) :
					 errmsg("referenced column \"%s\" does not exist", name),
					 parser_errposition(pstate, location)));
		if (match > 1)
			ereport(ERROR,
					(errcode(ERRCODE_AMBIGUOUS_COLUMN),
					 is_referencing ?
					 errmsg("referencing column \"%s\" is ambiguous", name) :
					 errmsg("referenced column \"%s\" is ambiguous", name),
					 parser_errposition(pstate, location)));

		/* Find the operand surface attnum for the resolved namespace column. */
		for (int j = 0; j < surface_ncols; j++)
		{
			ParseNamespaceColumn *scol = surface->p_nscolumns + j;

			if (scol->p_varno == cols[i].nscol->p_varno &&
				scol->p_varattno == cols[i].nscol->p_varattno)
			{
				cols[i].surface_attno = j + 1;
				break;
			}
		}

		if (cols[i].surface_attno == 0)
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_COLUMN_REFERENCE),
					 is_referencing ?
					 errmsg("referencing column \"%s\" is not exposed by the join operand",
							name) :
					 errmsg("referenced column \"%s\" is not exposed by the join operand",
							name),
					 parser_errposition(pstate, location)));
		i++;
	}
	return cols;
}

/*
 * make_var_from_nscolumn
 *
 *		Build a Var for a namespace column and mark its nullable state.
 *
 * Called by:
 *		transformAndValidateKeyJoin
 */
static Var *
make_var_from_nscolumn(ParseState *pstate, ParseNamespaceColumn *nscol)
{
	Var		   *var = makeVar(nscol->p_varno, nscol->p_varattno,
							  nscol->p_vartype, nscol->p_vartypmod,
							  nscol->p_varcollid, 0);

	var->varreturningtype = nscol->p_varreturningtype;
	var->varnosyn = nscol->p_varnosyn;
	var->varattnosyn = nscol->p_varattnosyn;
	markNullableIfNeeded(pstate, var);
	return var;
}

/*
 * join_preserves_side
 *
 *		Return true if the join type preserves rows from the requested side.
 *
 * Called by:
 *		transformAndValidateKeyJoin
 *		compute_join_output_facts
 *		revalidate_query_jointree_proofs
 */
static bool
join_preserves_side(JoinType jointype, bool leftside)
{
	return jointype == JOIN_FULL ||
		(leftside ? jointype == JOIN_LEFT : jointype == JOIN_RIGHT);
}

/*
 * build_key_join_quals
 *
 *		Build the executable equality quals for a proven key join.
 *
 * Called by:
 *		transformAndValidateKeyJoin
 *		revalidate_query_jointree_proofs
 */
static Node *
build_key_join_quals(List *referenced_args,
					 List *referencing_args, List *eqoperators,
					 List *eqtypes, List *eqtypmods,
					 List *locations, ParseLoc default_location)
{
	List	   *quals = NIL;
	ListCell   *lcrpk;
	ListCell   *lcrfk;
	ListCell   *lcop;
	ListCell   *lctype;
	ListCell   *lctypmod;
	ListCell   *lcloc;

	Assert(list_length(referenced_args) == list_length(referencing_args));
	Assert(list_length(referenced_args) == list_length(eqoperators));
	Assert(list_length(referenced_args) == list_length(eqtypes));
	Assert(list_length(referenced_args) == list_length(eqtypmods));
	Assert(locations == NIL ||
		   list_length(locations) == list_length(eqoperators));

	lcloc = list_head(locations);
	forfive(lcrpk, referenced_args, lcrfk, referencing_args,
			lcop, eqoperators, lctype, eqtypes, lctypmod, eqtypmods)
	{
		Node	   *pkarg = (Node *) lfirst(lcrpk);
		Node	   *fkarg = (Node *) lfirst(lcrfk);
		Oid			opno = lfirst_oid(lcop);
		Oid			eqtype = lfirst_oid(lctype);
		int32		eqtypmod = lfirst_int(lctypmod);
		ParseLoc	location = default_location;
		Oid			opfuncid;
#ifdef USE_ASSERT_CHECKING
		Oid			lefttype;
		Oid			righttype;
#endif
		OpExpr	   *result;

		if (lcloc != NULL)
		{
			location = lfirst_int(lcloc);
			lcloc = lnext(locations, lcloc);
		}

		/*
		 * Build a key-join equality OpExpr from a catalog operator OID.  For
		 * domains, the proof identity remains the domain type, while the
		 * executable equality operator is proven and run against the base type.
		 * The proof matcher only passes operators checked against that
		 * normalized equality-input identity and known to be strict,
		 * non-set-returning boolean equality.
		 */
		opfuncid = get_opcode(opno);
		pkarg = applyRelabelType(pkarg, eqtype, eqtypmod,
								 exprCollation(pkarg),
								 COERCE_IMPLICIT_CAST, -1, false);
		fkarg = applyRelabelType(fkarg, eqtype, eqtypmod,
								 exprCollation(fkarg),
								 COERCE_IMPLICIT_CAST, -1, false);
#ifdef USE_ASSERT_CHECKING
		op_input_types(opno, &lefttype, &righttype);
#endif
		Assert(RegProcedureIsValid(opfuncid));
		Assert(lefttype == exprType(pkarg));
		Assert(righttype == exprType(fkarg));
		Assert(get_op_rettype(opno) == BOOLOID);
		Assert(exprTypmod(pkarg) == exprTypmod(fkarg));
		Assert(exprCollation(pkarg) == exprCollation(fkarg));
		Assert(!get_func_retset(opfuncid));
		Assert(func_strict(opfuncid));

		result = makeNode(OpExpr);
		result->opno = opno;
		result->opfuncid = opfuncid;
		result->opresulttype = BOOLOID;
		result->opretset = false;
		result->opcollid = InvalidOid;
		result->inputcollid = exprCollation(pkarg);
		result->args = list_make2(pkarg, fkarg);
		result->location = location;

		quals = lappend(quals, result);
	}

	return (list_length(quals) == 1) ? linitial(quals) :
		(Node *) makeBoolExpr(AND_EXPR, quals, -1);
}

/*
 * find_key_join_match
 *
 *		Find proof facts that validate one key join.
 *
 * The proof has three pieces:
 *	  1. The referenced side has a unique fact covering the selected columns.
 *	  2. The referencing side has a foreign-key fact pointing at that unique
 *		 fact, and any filter quals on the referenced side remap into matching
 *		 filter quals on the referencing side.
 *	  3. If the join type does not preserve the referencing side, each
 *		 referencing column has not-null evidence.
 *
 * On success, *match receives the selected constraint, equality operators
 * and equality-input identities in key-join column order, and the
 * accumulated dependency lists.  On failure, *match is left zeroed.  The
 * caller is expected to use *match only if this function returns true.
 *
 * Called by:
 *		transformAndValidateKeyJoin
 *		revalidate_query_jointree_proofs
 */
static bool
find_key_join_match(RangeTblEntry *referencing_rte,
					RangeTblEntry *referenced_rte,
					List *referencing_attnums,
					List *referenced_attnums,
					bool need_notnull,
					KeyJoinMatch *match)
{
	KeyJoinSurfaceFacts *rfacts;
	KeyJoinSurfaceFacts *pfacts;

	Assert(match != NULL);
	Assert(referencing_rte != NULL);
	Assert(referenced_rte != NULL);
	memset(match, 0, sizeof(*match));

	if (referenced_rte->tablesample != NULL)
		return false;

	Assert(referencing_rte->keyJoinFactsComputed);
	Assert(referenced_rte->keyJoinFactsComputed);

	if (referencing_rte->keyJoinFacts == NULL ||
		referenced_rte->keyJoinFacts == NULL)
		return false;

	rfacts = referencing_rte->keyJoinFacts;
	pfacts = referenced_rte->keyJoinFacts;

	/* ---- Candidate 1: pick an FK fact on the referencing side ---- */
	foreach_node(KeyJoinFact, fkfact, rfacts->facts)
	{
		List	   *referencing_base;

		if (fkfact->kind != KJF_FOREIGN_KEY)
			continue;

		Assert(list_length(fkfact->keyPositions) ==
			   list_length(fkfact->baseAttnums));
		Assert(list_length(fkfact->baseAttnums) ==
			   list_length(fkfact->referencedAttnums));

		if (!select_key_position_parts(referencing_attnums,
									   fkfact->keyPositions,
									   fkfact->baseAttnums,
									   &referencing_base, NULL))
			continue;

		/* ---- Candidate 2: pick a unique fact on the referenced side ---- */
		foreach_node(KeyJoinFact, uniqfact, pfacts->facts)
		{
			List	   *unique_base;
			List	   *unique_key_positions;
			bool		catalog_unique;

			if (uniqfact->kind != KJF_UNIQUE)
				continue;

			Assert(list_length(uniqfact->keyPositions) ==
				   list_length(uniqfact->baseAttnums));

			if (!select_key_position_parts(referenced_attnums,
										   uniqfact->keyPositions,
										   uniqfact->baseAttnums,
										   &unique_base,
										   &unique_key_positions))
				continue;
			catalog_unique = OidIsValid(uniqfact->relid);
			Assert(int_lists_same_members(uniqfact->baseAttnums, unique_base));

			/*
			 * A catalog-backed unique fact has to be on the same relation the
			 * FK fact targets; without that, condition 1 doesn't hold over
			 * the relevant rows.
			 */
			if (catalog_unique &&
				uniqfact->relid != fkfact->referencedRelid)
				continue;

			/* ---- Candidate 3: pick a row-coverage fact ---- */
			foreach_node(KeyJoinFact, coverage, pfacts->facts)
			{
				/*
				 * Per-coverage scratch.  These live only inside this
				 * candidate; if any check below rejects it, control jumps to
				 * next_coverage and the next iteration starts fresh.
				 */
				List	   *coverage_base;
				List	   *coverage_key_positions;
				List	   *fk_key_positions = NIL;
				List	   *fk_eqoperators = NIL;
				List	   *fk_eqtypes = NIL;
				List	   *fk_eqtypmods = NIL;
				List	   *local_notnulldeps = NIL;

				if (coverage->kind != KJF_ROW_COVERAGE)
					continue;

				Assert(list_length(coverage->keyPositions) ==
					   list_length(coverage->baseAttnums));

				if (!select_key_position_parts(referenced_attnums,
											   coverage->keyPositions,
											   coverage->baseAttnums,
											   &coverage_base,
											   &coverage_key_positions))
					continue;

				/*
				 * Matching row-coverage key positions for the selected
				 * referenced columns must be rooted in the FK target. Fact
				 * projection does not merge row-coverage identities across
				 * base relations.
				 */
				if (coverage->relid != fkfact->referencedRelid)
					continue;
				Assert(!catalog_unique ||
					   int_lists_same_members(coverage_base, unique_base));
				Assert(int_lists_same_members(coverage->baseAttnums,
											  coverage_base));

				/* ---- Condition 2a: FK pairs match the selected columns ---- */
				{
					ListCell   *lcfkbase;
					ListCell   *lcpkbase;

					/*
					 * Key-join columns must cover the whole FK; a partial FK
					 * match cannot prove containment for a multi-column key.
					 * referencing_base and coverage_base are already in
					 * key-join column order, so we drive the result lists
					 * from them.
					 */
					Assert(list_length(referencing_base) ==
						   list_length(fkfact->keyPositions));
					Assert(list_length(coverage_base) ==
						   list_length(fkfact->keyPositions));

					forboth(lcfkbase, referencing_base,
							lcpkbase, coverage_base)
					{
						int			fkbase = lfirst_int(lcfkbase);
						int			pkbase = lfirst_int(lcpkbase);
						int			fk_catalog_pos = -1;
						ListCell   *lcfkatt;
						ListCell   *lcrefatt;
						KeyJoinKeyPosition *keypos;

						/*
						 * FK facts pair referencing and referenced base
						 * attnums in catalog order.  Locate the pair for this
						 * selected referencing column; the matching
						 * referenced attnum must equal the column we selected
						 * on the other side.
						 */
						forboth(lcfkatt, fkfact->baseAttnums,
								lcrefatt, fkfact->referencedAttnums)
						{
							if (lfirst_int(lcfkatt) != fkbase)
								continue;
							if (lfirst_int(lcrefatt) != pkbase)
								goto next_coverage;
							fk_catalog_pos = foreach_current_index(lcfkatt);
							break;
						}

						/*
						 * referencing_base was selected from
						 * fkfact->baseAttnums, so this lookup must find the
						 * catalog position.
						 */
						Assert(fk_catalog_pos >= 0);

						keypos = list_nth_node(KeyJoinKeyPosition,
											   fkfact->keyPositions,
											   fk_catalog_pos);
						fk_key_positions = lappend(fk_key_positions, keypos);
						fk_eqoperators = lappend_oid(fk_eqoperators,
													 keypos->eqOperator);
						fk_eqtypes = lappend_oid(fk_eqtypes,
												 keypos->eqTypeOid);
						fk_eqtypmods = lappend_int(fk_eqtypmods,
												   keypos->eqTypmod);
					}
				}

				/*
				 * ---- Condition 2b: unique + coverage agree on identity ----
				 *
				 * A relation can expose multiple usable unique indexes on the
				 * same column list.  Only the unique and row-coverage facts
				 * whose key identity matches the FK key identity can prove
				 * this key join.
				 */
				if (!key_position_identity_lists_equal(unique_key_positions,
													   fk_key_positions))
					goto next_coverage;
				if (!key_position_identity_lists_equal(coverage_key_positions,
													   fk_key_positions))
					goto next_coverage;

				/* ---- Condition 2c: referenced filters remap into FK ---- */
				if (coverage->filterConjuncts != NIL)
				{
					List	   *position_map;
					bool		filters_match = true;

					position_map =
						make_filter_position_map(coverage->baseAttnums,
												 coverage_base,
												 fkfact->baseAttnums,
												 referencing_base);

					foreach_ptr(Node, needed, coverage->filterConjuncts)
					{
						Node	   *remapped;

						/*
						 * The selected key-join columns cover both complete
						 * key lists, so every canonical coverage filter has
						 * a target FK key position.
						 */
						Assert(filter_conjunct_can_remap(needed,
														 position_map));
						remapped = remap_filter_conjunct(needed, position_map);
						Assert(filter_conjunct_matches_key_positions(remapped,
																	 fkfact->keyPositions));
						if (!list_contains_equal_node(fkfact->filterConjuncts,
													  remapped))
						{
							filters_match = false;
							break;
						}
					}
					if (!filters_match)
						continue;
				}

				/* ---- Condition 3: not-null evidence on referencing side ---- */
				if (need_notnull)
				{
					foreach_int(attno, referencing_attnums)
					{
						bool		found = false;

						foreach_node(KeyJoinFact, fact, rfacts->facts)
						{
							if (fact->kind != KJF_NOT_NULL)
								continue;
							if (fact->attnum != attno)
								continue;
							local_notnulldeps =
								append_dependencies_unique(local_notnulldeps,
														   fact->dependencies);
							found = true;
							break;
						}
						if (!found)
							goto next_coverage;
					}
				}

				/*
				 * ---- Success: construct the result, exactly once ----
				 *
				 * *match is touched only here, so any next_coverage path
				 * above leaves it untouched.  proofdeps is built locally
				 * before commit, so a partial accumulation cannot leak into
				 * the result.
				 */
				{
					List	   *proofdeps = NIL;

					proofdeps = append_dependencies_unique(proofdeps,
														   fkfact->dependencies);
					proofdeps = append_dependencies_unique(proofdeps,
														   uniqfact->dependencies);
					proofdeps = append_dependencies_unique(proofdeps,
														   coverage->dependencies);
					proofdeps = append_dependencies_unique(proofdeps,
														   local_notnulldeps);

					match->constraint = fkfact->constraint;
					match->eqoperators = fk_eqoperators;
					match->eqtypes = fk_eqtypes;
					match->eqtypmods = fk_eqtypmods;
					match->notnulldeps = local_notnulldeps;
					match->proofdeps = proofdeps;
				}
				return true;

		next_coverage:
				;
			}
		}
	}

	return false;
}

/*
 * select_key_position_parts
 *
 *		Map selected surface attnums to base attnums and/or key positions.
 *
 *		Each selected attnum must identify a distinct key position.
 *
 * Called by:
 *		find_key_join_match
 *		compute_join_output_facts
 */
static bool
select_key_position_parts(List *selected_attnums, List *keyPositions,
						  List *baseAttnums, List **selected_base_attnums,
						  List **selected_key_positions)
{
	List	   *base_result = NIL;
	List	   *position_result = NIL;
	List	   *used_positions = NIL;

	if (list_length(selected_attnums) != list_length(keyPositions))
		return false;
	if (selected_base_attnums != NULL)
	{
		Assert(list_length(keyPositions) == list_length(baseAttnums));
	}

	foreach_int(attno, selected_attnums)
	{
		int			pos = key_position_index_for_attnum(keyPositions, attno);

		if (pos < 0 || list_member_int(used_positions, pos))
			return false;
		used_positions = lappend_int(used_positions, pos);

		if (selected_base_attnums != NULL)
			base_result = lappend_int(base_result,
									  list_nth_int(baseAttnums, pos));
		if (selected_key_positions != NULL)
			position_result = lappend(position_result,
									  list_nth(keyPositions, pos));
	}

	if (selected_base_attnums != NULL)
		*selected_base_attnums = base_result;
	if (selected_key_positions != NULL)
		*selected_key_positions = position_result;
	return true;
}

/*
 * key_position_index_for_attnum
 *
 *		Return the key-position index containing an attnum.
 *
 *		Returns -1 if no position contains it.
 *
 * Called by:
 *		select_key_position_parts
 *		add_filter_conjuncts
 */
static int
key_position_index_for_attnum(List *keyPositions, int attno)
{
	int			match = -1;

	foreach_node(KeyJoinKeyPosition, keypos, keyPositions)
	{
		if (list_member_int(keypos->attnums, attno))
		{
			if (match >= 0)
				return -1;
			match = foreach_current_index(keypos);
		}
	}
	return match;
}

/*
 * key_position_identity_lists_equal
 *
 *		Return true if two key-position lists have matching key identities.
 *
 * Called by:
 *		find_key_join_match
 */
static bool
key_position_identity_lists_equal(List *left, List *right)
{
	ListCell   *lcleft;
	ListCell   *lcright;

	Assert(list_length(left) == list_length(right));
	forboth(lcleft, left, lcright, right)
	{
		if (!key_position_identity_equal(lfirst_node(KeyJoinKeyPosition, lcleft),
										 lfirst_node(KeyJoinKeyPosition, lcright)))
			return false;
	}
	return true;
}

/*
 * key_position_identity_equal
 *
 *		Return true if two key positions have the same type/collation/op
 *		identity.
 *
 * Called by:
 *		key_position_identity_lists_equal
 *		project_key_join_facts_from_rte
 *		compute_join_output_facts
 */
static bool
key_position_identity_equal(KeyJoinKeyPosition *left, KeyJoinKeyPosition *right)
{
	if (left->typeOid != right->typeOid)
		return false;
	if (left->typmod != right->typmod)
		return false;
	if (left->collationOid != right->collationOid)
		return false;
	/* These are cached derivations of typeOid and typmod. */
	Assert(left->eqTypeOid == right->eqTypeOid);
	Assert(left->eqTypmod == right->eqTypmod);
	return left->eqOperator == right->eqOperator;
}

/*
 * int_lists_same_members
 *
 *		Return true if two integer lists contain the same members.
 *
 * Called by:
 *		find_key_join_match
 *		compute_join_output_facts
 */
static bool
int_lists_same_members(List *a, List *b)
{
	if (list_length(a) != list_length(b))
		return false;
	foreach_int(value, a)
	{
		if (!list_member_int(b, value))
			return false;
	}
	return true;
}

/*
 * make_filter_position_map
 *
 *		Build a Param-position map between source and target key lists.
 *
 *		Entries of -1 mark source filters outside the selected key join.
 *
 * Called by:
 *		find_key_join_match
 *		compute_join_output_facts
 */
static List *
make_filter_position_map(List *src_base_attnums, List *src_selected_base,
						 List *dst_base_attnums, List *dst_selected_base)
{
	List	   *result = NIL;

	Assert(list_length(src_selected_base) == list_length(dst_selected_base));

	foreach_int(srcbase, src_base_attnums)
	{
		int			dstpos = -1;
		ListCell   *lcsrc;
		ListCell   *lcdst;

		forboth(lcsrc, src_selected_base, lcdst, dst_selected_base)
		{
			if (lfirst_int(lcsrc) == srcbase)
			{
				int			dstbase = lfirst_int(lcdst);

				foreach_int(baseattno, dst_base_attnums)
				{
					if (baseattno == dstbase)
					{
						dstpos = foreach_current_index(baseattno);
						break;
					}
				}
				break;
			}
		}
		result = lappend_int(result, dstpos);
	}
	return result;
}

/*
 * remap_filter_conjunct
 *
 *		Remap proof-filter Params through a position map.
 *
 * Called by:
 *		find_key_join_match
 *		compute_join_output_facts
 */
static Node *
remap_filter_conjunct(Node *conjunct, List *position_map)
{
	FKFilterRemapContext context;
	Node	   *result;

	memset(&context, 0, sizeof(context));
	context.position_map = position_map;
	result = remap_filter_param_mutator(conjunct, &context);
	return result;
}

/*
 * filter_conjunct_can_remap
 *
 *		Return true if every proof-filter Param used by this conjunct has a
 *		target key position in the supplied map.
 *
 * Called by:
 *		find_key_join_match
 *		compute_join_output_facts
 */
static bool
filter_conjunct_can_remap(Node *conjunct, List *position_map)
{
	FKFilterRemapContext context;

	memset(&context, 0, sizeof(context));
	context.position_map = position_map;
	return !filter_conjunct_unremappable_param_walker(conjunct, &context);
}

/*
 * remap_filter_param_mutator
 *
 *		Mutator callback for remapping key-join proof filter Params.
 *
 * Called by:
 *		remap_filter_conjunct
 */
static Node *
remap_filter_param_mutator(Node *node, void *context_arg)
{
	FKFilterRemapContext *context = (FKFilterRemapContext *) context_arg;

	if (node == NULL)
		return NULL;

	if (IsA(node, Param))
	{
		Param	   *param = castNode(Param, node);
		Param	   *newparam;
		int			oldpos;
		int			newpos;

		Assert(param->paramkind == PARAM_KEYJOIN);

		/*
		 * paramid is a 1-based position into the key-position list that the
		 * caller already sized.  These two conditions are therefore
		 * programmer invariants, not user-reachable errors.
		 */
		oldpos = param->paramid - 1;
		Assert(oldpos >= 0);
		Assert(oldpos < list_length(context->position_map));

		newpos = list_nth_int(context->position_map, oldpos);
		Assert(newpos >= 0);
		newparam = copyObject(param);
		newparam->paramid = newpos + 1;
		return (Node *) newparam;
	}

	return expression_tree_mutator(node, remap_filter_param_mutator, context);
}

/*
 * filter_conjunct_unremappable_param_walker
 *
 *		Walker callback for finding proof-filter Params that cannot remap.
 *
 * Called by:
 *		filter_conjunct_can_remap
 */
static bool
filter_conjunct_unremappable_param_walker(Node *node, void *context_arg)
{
	FKFilterRemapContext *context = (FKFilterRemapContext *) context_arg;

	Assert(node != NULL);

	if (IsA(node, Param))
	{
		Param	   *param = castNode(Param, node);
		int			oldpos;

		Assert(param->paramkind == PARAM_KEYJOIN);

		oldpos = param->paramid - 1;
		Assert(oldpos >= 0);
		Assert(oldpos < list_length(context->position_map));
		return list_nth_int(context->position_map, oldpos) < 0;
	}

	return expression_tree_walker(node,
								  filter_conjunct_unremappable_param_walker,
								  context);
}

/*
 * filter_conjunct_matches_key_positions
 *
 *		Check that a key-join proof filter matches key identity.
 *
 * Called by:
 *		find_key_join_match
 *		compute_join_output_facts
 */
static bool
filter_conjunct_matches_key_positions(Node *conjunct, List *keyPositions)
{
	OpExpr	   *op;
	Param	   *param;
	KeyJoinKeyPosition *keypos;
	int			pos;
#ifdef USE_ASSERT_CHECKING
	Node	   *value;
#endif

	Assert(conjunct != NULL);
	Assert(IsA(conjunct, OpExpr));
	op = castNode(OpExpr, conjunct);
	Assert(list_length(op->args) == 2);
	Assert(IsA(linitial(op->args), Param));
	param = castNode(Param, linitial(op->args));
	Assert(param->paramkind == PARAM_KEYJOIN);

	pos = param->paramid - 1;
	Assert(pos >= 0);
	Assert(pos < list_length(keyPositions));
	keypos = list_nth_node(KeyJoinKeyPosition, keyPositions, pos);

	/*
	 * add_filter_conjuncts() stores only canonical key = value filters.  The
	 * Param and value equality-input identities are copied from the source key
	 * position, and remapping changes only Param ids.  This caller has already
	 * matched another fact for the same base key, so exposed identity,
	 * equality-input identity, and collation still match; equality can differ
	 * for distinct same-column facts backed by different opclasses.
	 */
	Assert(param->paramtype == keypos->eqTypeOid);
	Assert(param->paramtypmod == keypos->eqTypmod);
	Assert(param->paramcollid == keypos->collationOid);
	if (op->opno != keypos->eqOperator)
		return false;
	Assert(op->inputcollid == keypos->collationOid);

#ifdef USE_ASSERT_CHECKING
	value = lsecond(op->args);
	Assert(filter_value_allowed(value));
	Assert(exprType(value) == keypos->eqTypeOid);
	Assert(exprTypmod(value) == keypos->eqTypmod);
	Assert(exprCollation(value) == keypos->collationOid);
#endif
	return true;
}

/*
 * filter_value_allowed
 *
 *		Return true if a filter value can be stored in proof-filter form.
 *		Proof filters use a strict allowlist: constants, SQL value functions,
 *		non-volatile scalar functions, and transparent type/collation wrappers
 *		whose arguments are also allowed.
 *
 * Called by:
 *		filter_conjunct_matches_key_positions
 *		add_filter_conjuncts
 */
static bool
filter_value_allowed(Node *node)
{
	Assert(node != NULL);

	if (IsA(node, Const))
		return true;

	if (IsA(node, SQLValueFunction))
		return true;

	if (IsA(node, FuncExpr))
	{
		FuncExpr   *expr = castNode(FuncExpr, node);

		Assert(!expr->funcretset);
		if (func_volatile(expr->funcid) == PROVOLATILE_VOLATILE)
			return false;
		foreach_ptr(Node, arg, expr->args)
		{
			if (!filter_value_allowed(arg))
				return false;
		}
		return true;
	}

	if (IsA(node, RelabelType))
		return filter_value_allowed((Node *) castNode(RelabelType, node)->arg);

	if (IsA(node, CoerceViaIO))
		return filter_value_allowed((Node *) castNode(CoerceViaIO, node)->arg);

	if (IsA(node, CollateExpr))
		return filter_value_allowed((Node *) castNode(CollateExpr, node)->arg);

	return false;
}

/*
 * list_contains_equal_node
 *
 *		Return true if a list already contains an equal expression node.
 *
 * Called by:
 *		find_key_join_match
 *		append_filter_conjunct_unique
 */
static bool
list_contains_equal_node(List *list, Node *node)
{
	foreach_ptr(Node, oldnode, list)
	{
		if (equal(node, oldnode))
			return true;
	}
	return false;
}

/*
 * append_dependencies_unique
 *
 *		Append dependency entries from src to dst, suppressing duplicates.
 *
 * Called by:
 *		find_key_join_match
 *		project_key_join_facts_from_rte
 *		compute_join_output_facts
 */
static List *
append_dependencies_unique(List *dst, List *src)
{
	foreach_node(KeyJoinProofDependency, dep, src)
	{
		if (!dependency_member(dst, dep->classId, dep->objectId,
							   dep->objectSubId))
			dst = lappend(dst, copyObject(dep));
	}
	return dst;
}

/*
 * dependency_member
 *
 *		Return true if a dependency list contains the given object address.
 *
 * Called by:
 *		append_dependencies_unique
 *		append_filter_dependency
 *		dependency_list_is_subset
 *		key_join_equality_operator_is_usable
 */
static bool
dependency_member(List *deps, Oid classId, Oid objectId, int32 objectSubId)
{
	Assert(objectSubId == 0);

	foreach_node(KeyJoinProofDependency, dep, deps)
	{
		Assert(dep->objectSubId == 0);
		if (dep->classId != classId)
			continue;
		if (dep->objectId != objectId)
			continue;
		return true;
	}
	return false;
}

/*
 * make_dependency
 *
 *		Build a KeyJoinProofDependency node for a whole-object dependency.
 *
 * Called by:
 *		compute_key_join_relation_facts
 *		append_filter_dependency
 *		key_join_equality_operator_is_usable
 */
static KeyJoinProofDependency *
make_dependency(Oid classId, Oid objectId)
{
	KeyJoinProofDependency *dep = makeNode(KeyJoinProofDependency);

	dep->classId = classId;
	dep->objectId = objectId;
	dep->objectSubId = 0;
	return dep;
}

/*
 * ensure_key_join_surface_facts
 *
 *		Entry point for live parse analysis.
 *
 * Called by:
 *		transformAndValidateKeyJoin
 */
static void
ensure_key_join_surface_facts(ParseState *pstate, RangeTblEntry *rte)
{
	KeyJoinFactContext context;

	memset(&context, 0, sizeof(context));
	context.pstate = pstate;
	ensure_key_join_surface_facts_internal(&context, rte);
}

/*
 * ensure_key_join_surface_facts_internal
 *
 *		Ensure that an RTE exposes key-join surface facts when possible.
 *
 * Called by:
 *		ensure_key_join_surface_facts
 *		project_key_join_query_facts
 *		compute_join_output_facts
 *		revalidate_query_jointree_proofs
 */
static void
ensure_key_join_surface_facts_internal(KeyJoinFactContext *context,
									   RangeTblEntry *rte)
{
	Assert(context != NULL);
	Assert(rte != NULL);

	if (rte->keyJoinFactsComputed)
		return;

	Assert(rte->keyJoinFacts == NULL);

	switch (rte->rtekind)
	{
		case RTE_RELATION:
			{
				/*
				 * Live parser RTEs already hold their rellockmode lock from
				 * range table construction, but stored-query revalidation and
				 * view fact computation can reach this helper with only a
				 * copied Query tree.  Take our own short-lived lock so
				 * relcache access is safe in both paths.
				 */
				Relation	rel = relation_open(rte->relid, AccessShareLock);

				if (rte->relkind == RELKIND_VIEW)
				{
					/*
					 * Views expose facts by projecting a revalidated copy of
					 * their query.  Keep this query-projection path at the
					 * RTE dispatcher rather than inside the catalog fact
					 * collector for base relations.
					 */
					Query	   *viewquery = get_view_query(rel);
					KeyJoinQueryStack qs;
					KeyJoinFactContext view_context = *context;

					viewquery = copyObject(viewquery);
					revalidate_stored_key_join_proofs_in_query(viewquery,
															   NULL);
					qs.parent = NULL;
					qs.query = viewquery;
					view_context.pstate = NULL;
					view_context.query = viewquery;
					view_context.query_stack = &qs;
					view_context.revalidating_stored_query = true;
					rte->keyJoinFacts =
						project_key_join_query_facts(&view_context,
													 viewquery);
				}
				else
					compute_key_join_relation_facts(context, rte, rel);
				relation_close(rel, AccessShareLock);
			}
			break;
		case RTE_SUBQUERY:
			{
				KeyJoinQueryStack qs;

				Assert(rte->subquery != NULL);

				/*
				 * LATERAL subqueries need per-outer-row cardinality proof
				 * before their projected facts can be used soundly, so expose
				 * no facts from them.
				 */
				if (rte->lateral)
				{
					rte->keyJoinFacts = NULL;
					break;
				}

				if (context->revalidating_stored_query)
					revalidate_stored_key_join_proofs_in_query(rte->subquery,
															   context->query_stack);

				qs.parent = context->query_stack;
				qs.query = rte->subquery;
				{
					KeyJoinFactContext subcontext = *context;

					subcontext.query = rte->subquery;
					subcontext.query_stack = &qs;
					rte->keyJoinFacts =
						project_key_join_query_facts(&subcontext,
													 rte->subquery);
				}
				break;
			}
		case RTE_CTE:
			{
				CommonTableExpr *cte = NULL;
				KeyJoinQueryStack *cte_owner_stack = NULL;

				if (context->query_stack != NULL)
				{
					int			levelsup = (int) rte->ctelevelsup;

					/*
					 * Stored revalidation starts from this RTE reference
					 * site.  Walk rte->ctelevelsup to the Query that owns the
					 * CTE, and keep that owner frame as the CTE query's
					 * parent stack.
					 */
					for (KeyJoinQueryStack *qs = context->query_stack;
						 qs != NULL;
						 qs = qs->parent)
					{
						if (levelsup == 0)
						{
							foreach_node(CommonTableExpr, candidate,
										 qs->query->cteList)
							{
								if (strcmp(candidate->ctename,
										   rte->ctename) == 0)
								{
									cte = candidate;
									cte_owner_stack = qs;
									break;
								}
							}
							break;
						}
						levelsup--;
					}
					Assert(cte == NULL || cte_owner_stack != NULL);
				}

				/*
				 * A recursive self-reference names the recursive working
				 * table, not an ordinary CTE result surface.
				 */
				if (rte->self_reference)
					break;

				if (cte == NULL)
				{
					/*
					 * Stored revalidation resolves ordinary CTE references
					 * through the query stack.  If that failed, only live
					 * demand-driven projection can still resolve the CTE
					 * from a visible WITH namespace.
					 */
					for (ParseState *ps = context->pstate;
						 cte == NULL;
						 ps = ps->parentParseState)
					{
						Assert(ps != NULL);
						foreach_node(CommonTableExpr, candidate,
									 ps->p_ctenamespace)
						{
							if (strcmp(candidate->ctename,
									   rte->ctename) == 0)
							{
								cte = candidate;
								break;
							}
						}
					}
					cte_owner_stack = NULL;
				}

				Assert(cte_owner_stack == NULL || cte != NULL);

				/*
				 * Only ordinary, resolved, non-recursive CTE queries have a
				 * single query result surface we can project facts from.  A
				 * recursive CTE needs fixpoint reasoning this proof model does
				 * not attempt.
				 */
				Assert(cte != NULL);
				Assert(IsA(cte->ctequery, Query));
				if (cte->cterecursive)
					break;

				revalidate_stored_key_join_proofs_in_query((Query *) cte->ctequery,
														   cte_owner_stack);

				{
					KeyJoinQueryStack qs;

					qs.parent = cte_owner_stack;
					qs.query = (Query *) cte->ctequery;
					{
						KeyJoinFactContext cte_context = *context;

						cte_context.query = (Query *) cte->ctequery;
						cte_context.query_stack = &qs;
						rte->keyJoinFacts =
							project_key_join_query_facts(&cte_context,
														 (Query *) cte->ctequery);
					}
				}
				break;
			}
		case RTE_JOIN:
			{
				Index		rtindex = rtindex_for_rte(context, rte);
				JoinExpr   *j = find_join_expr_for_rtindex(context, rtindex);
				Index		left_rtindex;
				Index		right_rtindex;
				RangeTblEntry *left_rte;
				RangeTblEntry *right_rte;
				bool		use_query = (context->pstate == NULL);

				/*
				 * Live parse analysis can find the JoinExpr through
				 * ParseState.  Stored-query revalidation has only the copied
				 * Query tree, so retry the lookup there.
				 */
				if (j == NULL)
				{
					KeyJoinFactContext query_context = *context;

					Assert(context->query != NULL);
					query_context.pstate = NULL;
					rtindex = rtindex_for_rte(&query_context, rte);
					j = find_join_expr_for_rtindex(&query_context, rtindex);
					use_query = true;
				}

				/* Without the JoinExpr, we cannot find the input surfaces. */
				Assert(j != NULL);

				/*
				 * Ordinary joins expose no key-join facts.  Only accepted key
				 * joins and plain inner cross joins can need the input
				 * surfaces below.
				 */
				if (j->keyJoin == NULL &&
					(j->quals != NULL || j->jointype != JOIN_INNER))
					break;

				left_rtindex = jtnode_surface_rtindex(j->larg);
				right_rtindex = jtnode_surface_rtindex(j->rarg);

				/*
				 * A transformed JoinExpr's operands must expose concrete
				 * surface RTEs.
				 */
				Assert(left_rtindex != 0);
				Assert(right_rtindex != 0);
				if (use_query)
				{
					left_rte = rt_fetch(left_rtindex, context->query->rtable);
					right_rte = rt_fetch(right_rtindex, context->query->rtable);
				}
				else
				{
					left_rte = rt_fetch(left_rtindex, context->pstate->p_rtable);
					right_rte = rt_fetch(right_rtindex, context->pstate->p_rtable);
				}

				ensure_key_join_surface_facts_internal(context, left_rte);
				ensure_key_join_surface_facts_internal(context, right_rte);

				compute_join_output_facts(j, left_rtindex, left_rte,
										  right_rtindex, right_rte,
										  rte, context);
				break;
			}
		default:
			break;
	}

	rte->keyJoinFactsComputed = true;
}

/*
 * compute_key_join_relation_facts
 *
 *		Collect catalog-backed surface facts from a base relation RTE.
 *
 *		Validated enforced NOT NULL constraints, validated nondeferrable
 *		FKs, and usable unique indexes become surface facts.
 *
 * Called by:
 *		ensure_key_join_surface_facts_internal
 */
static void
compute_key_join_relation_facts(KeyJoinFactContext *context,
								RangeTblEntry *rte, Relation rel)
{
	KeyJoinSurfaceFacts *set;
	TupleDesc	tupdesc;

	Assert(rte->rtekind == RTE_RELATION);

	/* An ONLY scan of a partitioned parent is not the partition tree. */
	if (rte->relkind == RELKIND_PARTITIONED_TABLE && !rte->inh)
		return;

	/* An inherited scan of a table with children may include child rows. */
	if (rte->relkind == RELKIND_RELATION &&
		rte->inh &&
		has_subclass(rte->relid))
		return;

	/* Other relkinds do not provide base table facts here. */
	if (rte->relkind != RELKIND_RELATION &&
		rte->relkind != RELKIND_PARTITIONED_TABLE)
		return;

	if (rel->rd_rel->relrowsecurity)
		return;

	set = makeNode(KeyJoinSurfaceFacts);
	tupdesc = RelationGetDescr(rel);

	/* Validated enforced NOT NULL constraints feed condition 3. */
	for (int attno = 1; attno <= tupdesc->natts; attno++)
	{
		Form_pg_attribute att = TupleDescAttr(tupdesc, attno - 1);
		HeapTuple	contup;
		Form_pg_constraint con;

		if (att->attisdropped || !att->attnotnull)
			continue;
		contup = findNotNullConstraintAttnum(rte->relid, attno);
		if (!HeapTupleIsValid(contup))
			continue;
		con = (Form_pg_constraint) GETSTRUCT(contup);

		/*
		 * NO INHERIT NOT NULL constraints on a partitioned parent do not
		 * cover child partition rows, so they are not proof facts for an
		 * inherited partitioned-table scan.
		 */
		Assert(con->conenforced);
		if (con->convalidated)
		{
			bool		covers_rows = true;

			if (con->connoinherit)
			{
				if (rte->relkind == RELKIND_PARTITIONED_TABLE)
					covers_rows = false;
			}

			if (covers_rows)
			{
				KeyJoinFact *fact = add_fact(set, KJF_NOT_NULL);

				fact->attnum = attno;
				fact->dependencies =
					list_make1(make_dependency(ConstraintRelationId,
											   con->oid));
			}
		}
		heap_freetuple(contup);
	}

	/* Usable unique indexes feed condition 1 plus base row coverage. */
	foreach_oid(indexoid, RelationGetIndexList(rel))
	{
		Relation	indexrel = index_open(indexoid, AccessShareLock);
		Form_pg_index index = indexrel->rd_index;
		AttrNumber	attnums[INDEX_MAX_KEYS];
		Oid			eqoperators[INDEX_MAX_KEYS];
		List	   *deps = NIL;
		Oid			constraint;
		bool		usable = true;

		/*
		 * Unique proof facts must be unconditional, immediate, valid, plain
		 * column indexes.  Expressions and predicates would need additional
		 * implication proof before they could validate arbitrary key joins.
		 */
		if (!index->indisunique)
		{
			index_close(indexrel, AccessShareLock);
			continue;
		}
		if (!index->indisvalid)
		{
			index_close(indexrel, AccessShareLock);
			continue;
		}
		if (!index->indimmediate)
		{
			index_close(indexrel, AccessShareLock);
			continue;
		}
		Assert(index->indnkeyatts > 0);
		if (RelationGetIndexExpressions(indexrel) != NIL)
		{
			index_close(indexrel, AccessShareLock);
			continue;
		}
		if (RelationGetIndexPredicate(indexrel) != NIL)
		{
			index_close(indexrel, AccessShareLock);
			continue;
		}

		constraint = get_index_constraint(indexoid);
		deps = list_make1(make_dependency(OidIsValid(constraint) ?
										  ConstraintRelationId :
										  RelationRelationId,
										  OidIsValid(constraint) ?
										  constraint : indexoid));

		for (int i = 0; i < index->indnkeyatts; i++)
		{
			AttrNumber	attno = index->indkey.values[i];
			Form_pg_attribute att;
			Oid			eqtype;
			Oid			eqop;

			Assert(attno > 0);
			att = TupleDescAttr(tupdesc, attno - 1);
			eqtype = key_join_equality_type(att->atttypid,
											att->atttypmod, NULL);

			/*
			 * The index key must be the live table column under that column's
			 * collation, and the collation must make equality deterministic.
			 */
			Assert(!att->attisdropped);
			if (indexrel->rd_indcollation[i] != att->attcollation)
			{
				usable = false;
				break;
			}
			if (!key_join_collation_is_usable(att->attcollation))
			{
				usable = false;
				break;
			}
			eqop = get_opfamily_member_for_cmptype(indexrel->rd_opfamily[i],
												   eqtype, eqtype, COMPARE_EQ);
			if (!key_join_equality_operator_is_usable(eqop, eqtype,
													  &deps))
			{
				usable = false;
				break;
			}
			attnums[i] = attno;
			eqoperators[i] = eqop;
		}

		if (usable)
		{
			List	   *keyattnums =
				list_make_attrnums(attnums, index->indnkeyatts);
			List	   *keypositions =
				make_key_positions_from_attrnums(tupdesc, attnums,
												 index->indnkeyatts,
												 eqoperators);
			KeyJoinFact *ufact = add_fact(set, KJF_UNIQUE);

			ufact->keyPositions = copyObject(keypositions);
			ufact->relid = rte->relid;
			ufact->baseAttnums = list_copy(keyattnums);
			ufact->dependencies = copyObject(deps);

			add_paired_row_coverage(set, keypositions, rte->relid,
									keyattnums, copyObject(deps));
		}

		index_close(indexrel, AccessShareLock);
	}

	/*
	 * Validated, catalog-enforced, nondeferrable equality FKs feed condition
	 * 2 plus base row coverage; period FKs are skipped.
	 *
	 * This intentionally trusts pg_constraint's catalog contract.  We do not
	 * inspect current or historical RI trigger enablement here; orphan rows
	 * created through privileged trigger bypass are referential-integrity
	 * corruption outside the key-join proof model, not proof facts to audit
	 * during parse analysis.
	 */
	foreach_node(ForeignKeyCacheInfo, fk, RelationGetFKeyList(rel))
	{
		HeapTuple	contup;
		Form_pg_constraint con;
		KeyJoinFact *fact;
		Relation	refrel;
		TupleDesc	reftupdesc;
		List	   *deps;
		int			nkeys;
		AttrNumber	conkey[INDEX_MAX_KEYS];
		AttrNumber	confkey[INDEX_MAX_KEYS];
		Oid			pf_eq_oprs[INDEX_MAX_KEYS];
		Oid			pp_eq_oprs[INDEX_MAX_KEYS];
		Oid			ff_eq_oprs[INDEX_MAX_KEYS];
		Oid			eqoperators[INDEX_MAX_KEYS];
		bool		usable = true;

		if (!fk->conenforced)
			continue;
		contup = SearchSysCache1(CONSTROID, ObjectIdGetDatum(fk->conoid));
		/* XXX Should be elog(ERROR) for a missing pg_constraint tuple. */
		Assert(HeapTupleIsValid(contup));
		con = (Form_pg_constraint) GETSTRUCT(contup);
		if (!con->convalidated || con->condeferrable || con->conperiod)
		{
			ReleaseSysCache(contup);
			continue;
		}

		/*
		 * FKs referencing partitioned tables have child pg_constraint rows
		 * for each referenced partition.  Those rows are enforcement
		 * machinery for the parent FK; they do not prove that every
		 * referencing value is contained in that one partition's keyspace. In
		 * contrast, a referencing-side partition can inherit an FK whose
		 * referenced relation is the same as the root FK's referenced
		 * relation, and that remains a valid containment proof for the leaf
		 * relation.
		 */
		if (OidIsValid(con->conparentid))
		{
			Oid			referencedRelid = con->confrelid;
			Oid			parentid = con->conparentid;

			while (OidIsValid(parentid))
			{
				HeapTuple	parenttup;
				Form_pg_constraint parentcon;
				Oid			nextparentid;

				parenttup = SearchSysCache1(CONSTROID,
											ObjectIdGetDatum(parentid));
				/* XXX Should be elog(ERROR) for a missing pg_constraint tuple. */
				Assert(HeapTupleIsValid(parenttup));

				parentcon = (Form_pg_constraint) GETSTRUCT(parenttup);
				Assert(parentcon->contype == CONSTRAINT_FOREIGN);

				nextparentid = parentcon->conparentid;
				if (!OidIsValid(nextparentid))
				{
					if (parentcon->confrelid != referencedRelid)
						usable = false;
				}

				ReleaseSysCache(parenttup);

				if (!usable)
					break;
				if (!OidIsValid(nextparentid))
					break;
				parentid = nextparentid;
			}

			if (!usable)
			{
				ReleaseSysCache(contup);
				continue;
			}
		}

		DeconstructFkConstraintRow(contup, &nkeys, conkey, confkey,
								   pf_eq_oprs, pp_eq_oprs, ff_eq_oprs,
								   NULL, NULL);
		Assert(nkeys == fk->nkeys);
		deps = list_make1(make_dependency(ConstraintRelationId, fk->conoid));

		/*
		 * The per-column checks below are proof-eligibility checks, not
		 * catalog invariants.  PostgreSQL can enforce valid FKs whose types,
		 * typmods, collations, or RI equality operators are not identical
		 * enough for key-join proof facts.  Such FKs remain valid; they just
		 * do not contribute facts here.
		 */
		refrel = relation_open(fk->confrelid, AccessShareLock);
		reftupdesc = RelationGetDescr(refrel);
		for (int i = 0; i < nkeys; i++)
		{
			Form_pg_attribute fkatt;
			Form_pg_attribute pkatt;
			Oid			eqtype;

			Assert(conkey[i] > 0);
			Assert(confkey[i] > 0);
			fkatt = TupleDescAttr(tupdesc, conkey[i] - 1);
			pkatt = TupleDescAttr(reftupdesc, confkey[i] - 1);

			/*
			 * A live foreign-key constraint cannot name dropped columns:
			 * dependency processing would have removed the constraint, or
			 * rejected the drop, before either column could be marked
			 * dropped.
			 */
			Assert(!fkatt->attisdropped);
			Assert(!pkatt->attisdropped);
			if (fkatt->atttypid != pkatt->atttypid)
			{
				usable = false;
				break;
			}
			if (fkatt->atttypmod != pkatt->atttypmod)
			{
				usable = false;
				break;
			}
			if (fkatt->attcollation != pkatt->attcollation)
			{
				usable = false;
				break;
			}
			if (!key_join_collation_is_usable(fkatt->attcollation))
			{
				usable = false;
				break;
			}
			if (pf_eq_oprs[i] != pp_eq_oprs[i])
			{
				usable = false;
				break;
			}
			/*
			 * Once the PK/FK equality operator matches the PK/PK equality
			 * operator, the FK/FK equality operator must match too.
			 * ATAddForeignKeyConstraint() either stores all three operators
			 * as the opclass primary equality operator through its
			 * implicit-cast fallback, or stores exact opfamily operators
			 * whose fixed signatures cannot make conpfeqop equal conppeqop
			 * while differing from conffeqop.
			 */
			Assert(pf_eq_oprs[i] == ff_eq_oprs[i]);
			eqtype = key_join_equality_type(fkatt->atttypid,
											fkatt->atttypmod, NULL);
			if (!key_join_equality_operator_is_usable(pf_eq_oprs[i], eqtype,
													  &deps))
			{
				usable = false;
				break;
			}
			eqoperators[i] = pf_eq_oprs[i];
		}
		relation_close(refrel, AccessShareLock);
		if (!usable)
		{
			ReleaseSysCache(contup);
			continue;
		}

		fact = add_fact(set, KJF_FOREIGN_KEY);
		fact->keyPositions =
			make_key_positions_from_attrnums(tupdesc, conkey, nkeys,
											 eqoperators);
		fact->relid = fk->conrelid;
		fact->baseAttnums = list_make_attrnums(conkey, nkeys);
		fact->referencedRelid = fk->confrelid;
		fact->referencedAttnums = list_make_attrnums(confkey, nkeys);
		fact->constraint = fk->conoid;
		fact->dependencies = copyObject(deps);

		add_paired_row_coverage(set, copyObject(fact->keyPositions),
								fact->relid, list_copy(fact->baseAttnums), deps);

		ReleaseSysCache(contup);
	}

	if (key_join_surface_facts_has_facts(set))
		rte->keyJoinFacts = set;
}

/*
 * rtindex_for_rte
 *
 *		Find the range-table index for an RTE pointer in the live ParseState
 *		or saved Query that owns it.
 *
 * Called by:
 *		ensure_key_join_surface_facts_internal
 */
static Index
rtindex_for_rte(KeyJoinFactContext *context, RangeTblEntry *rte)
{
	List	   *rtable;
	int			rtindex = 1;

	Assert(context != NULL);
	Assert(rte != NULL);

	if (context->pstate != NULL)
		rtable = context->pstate->p_rtable;
	else
	{
		Assert(context->query != NULL);
		rtable = context->query->rtable;
	}

	foreach_node(RangeTblEntry, candidate, rtable)
	{
		if (candidate == rte)
			return rtindex;
		rtindex++;
	}

	Assert(context->pstate != NULL);
	return 0;
}

/*
 * find_join_expr_for_rtindex
 *
 *		Find the JoinExpr attached to a JOIN RTE.
 *
 * Called by:
 *		ensure_key_join_surface_facts_internal
 */
static JoinExpr *
find_join_expr_for_rtindex(KeyJoinFactContext *context, Index rtindex)
{
	Assert(context != NULL);

	if (rtindex == 0)
		return NULL;

	if (context->pstate != NULL)
	{
		Node	   *node;

		Assert(rtindex <= list_length(context->pstate->p_joinexprs));
		node = list_nth(context->pstate->p_joinexprs, rtindex - 1);
		Assert(node != NULL);
		return castNode(JoinExpr, node);
	}

	Assert(context->query != NULL);
	return find_join_expr_in_jointree((Node *) context->query->jointree,
									  rtindex);
}

/*
 * find_join_expr_in_jointree
 *
 *		Search a saved Query jointree for the JoinExpr with rtindex.
 *
 * Called by:
 *		find_join_expr_for_rtindex
 */
static JoinExpr *
find_join_expr_in_jointree(Node *jtnode, Index rtindex)
{
	Assert(jtnode != NULL);

	if (IsA(jtnode, JoinExpr))
	{
		JoinExpr   *j = castNode(JoinExpr, jtnode);
		JoinExpr   *result;

		if (j->rtindex == rtindex)
			return j;
		result = find_join_expr_in_jointree(j->larg, rtindex);
		return result ? result : find_join_expr_in_jointree(j->rarg, rtindex);
	}
	if (IsA(jtnode, FromExpr))
	{
		JoinExpr   *result = NULL;

		foreach_ptr(Node, child, castNode(FromExpr, jtnode)->fromlist)
		{
			result = find_join_expr_in_jointree(child, rtindex);

			if (result != NULL)
				break;
		}
		return result;
	}

	return NULL;
}

/*
 * project_key_join_query_facts
 *
 *		Compute surface facts exposed by a query targetlist.
 *
 *		Query shape determines which base facts survive projection and
 *		whether GROUP BY or DISTINCT introduces query-level uniqueness.
 *
 * Called by:
 *		ensure_key_join_surface_facts_internal
 */
static KeyJoinSurfaceFacts *
project_key_join_query_facts(KeyJoinFactContext *context, Query *query)
{
	KeyJoinSurfaceFacts *result;
	int			natts;
	int		   *srcvarno;
	int		   *srcattno;
	int			outattno = 0;
	Node	   *topjtnode = NULL;
	Index		top_rtindex = 0;
	RangeTblEntry *toprte = NULL;
	bool		block_rowcoverage;
	List	   *rowcoverage_key_position_sets = NIL;

	Assert(context != NULL);
	Assert(query != NULL);

	/* These shapes destroy all proof meaning. */
	if (query->commandType != CMD_SELECT ||
		query->setOperations != NULL ||
		query->hasTargetSRFs ||
		query->groupingSets != NIL)
		return NULL;

	/*
	 * Volatile expressions can change state that matched-filter proofs read
	 * while the executor evaluates later operands.  Treat them as a complete
	 * proof barrier for computed query facts.
	 */
	if (contain_volatile_functions((Node *) query))
		return NULL;

	natts = list_length(query->targetList);
	srcvarno = palloc0((natts + 1) * sizeof(int));
	srcattno = palloc0((natts + 1) * sizeof(int));

	foreach_node(TargetEntry, tle, query->targetList)
	{
		Var		   *var;

		if (tle->resjunk)
			continue;
		outattno++;
		var = direct_var_from_node((Node *) tle->expr);
		if (var)
		{
			RangeTblEntry *varrte = rt_fetch(var->varno, query->rtable);

			if (varrte->rtekind == RTE_GROUP)
			{
				Assert(var->varattno > 0);
				Assert(var->varattno <= list_length(varrte->groupexprs));
				var = direct_var_from_node((Node *)
										   list_nth(varrte->groupexprs,
													var->varattno - 1));
			}
		}
		if (var)
		{
			srcvarno[outattno] = var->varno;
			srcattno[outattno] = var->varattno;
		}
	}

	result = makeNode(KeyJoinSurfaceFacts);

	/*
	 * HAVING/LIMIT/OFFSET/FOR UPDATE can remove rows post-base, so they block
	 * row coverage but not other facts.
	 */
	block_rowcoverage = query->havingQual != NULL ||
		query->limitOffset != NULL ||
		query->limitCount != NULL ||
		query->rowMarks != NIL;

	/*
	 * GROUP BY and DISTINCT can remove rows.  A row-coverage key survives
	 * only when every row-collapsing stage groups/distincts by that key under
	 * exactly the same equality identity used by the key proof.  If both
	 * stages are present, either one could otherwise discard a needed key.
	 */
	if (query->groupClause != NIL)
		rowcoverage_key_position_sets =
			lappend(rowcoverage_key_position_sets,
					make_rowcollapse_key_positions(query, query->groupClause));
	if (query->distinctClause != NIL)
		rowcoverage_key_position_sets =
			lappend(rowcoverage_key_position_sets,
					make_rowcollapse_key_positions(query, query->distinctClause));

	Assert(query->jointree != NULL);
	if (list_length(query->jointree->fromlist) == 1)
	{
		topjtnode = linitial(query->jointree->fromlist);
		top_rtindex = jtnode_surface_rtindex(topjtnode);
		Assert(top_rtindex > 0);
		toprte = rt_fetch(top_rtindex, query->rtable);
		ensure_key_join_surface_facts_internal(context, toprte);
	}

	if (toprte != NULL && toprte->keyJoinFacts != NULL)
	{
		int			ncols = list_length(toprte->eref->colnames);
		List	  **attrmap = palloc0((ncols + 1) * sizeof(List *));

		for (int i = 1; i <= outattno; i++)
		{
			List	   *mapped_attnums;

			if (srcattno[i] <= 0)
				continue;
			mapped_attnums = map_var_to_jtnode_surface(query, topjtnode,
													   srcvarno[i], srcattno[i]);
			foreach_int(topattno, mapped_attnums)
			{
				Assert(topattno > 0);
				Assert(topattno <= ncols);
				attrmap[topattno] = lappend_int(attrmap[topattno], i);
			}
		}

		project_key_join_facts_from_rte(result, toprte, attrmap,
										true, true, !block_rowcoverage,
										query->jointree->quals,
										query, topjtnode, top_rtindex,
										rowcoverage_key_position_sets, NIL);
		pfree(attrmap);
	}

	/*
	 * GROUP BY / DISTINCT also proves uniqueness of the grouped/distinct
	 * output columns; query-level so no relid/catalog dependency.
	 */
	if (query->groupClause != NIL || query->distinctClause != NIL)
	{
		List	   *clauses = query->groupClause != NIL ?
			query->groupClause : query->distinctClause;
		List	   *attnums = NIL;
		List	   *keypositions = NIL;
		List	   *deps = NIL;
		bool		usable = true;

		foreach_node(SortGroupClause, sgc, clauses)
		{
			TargetEntry *tle = get_sortgroupref_tle(sgc->tleSortGroupRef,
													query->targetList);

			Assert(tle != NULL);
			Assert(OidIsValid(sgc->eqop));
			if (tle->resjunk)
			{
				usable = false;
				break;
			}
			if (direct_var_from_node((Node *) tle->expr) == NULL)
			{
				usable = false;
				break;
			}
			if (!list_member_int(attnums, tle->resno))
			{
				Node	   *expr = (Node *) tle->expr;
				Oid			eqtype;

				if (!key_join_collation_is_usable(exprCollation(expr)))
				{
					usable = false;
					break;
				}
				eqtype = key_join_equality_type(exprType(expr),
												exprTypmod(expr), NULL);
				if (!key_join_equality_operator_is_usable(sgc->eqop, eqtype,
														  &deps))
				{
					usable = false;
					break;
				}
				attnums = lappend_int(attnums, tle->resno);
				keypositions =
					lappend(keypositions,
							make_key_position(list_make1_int(tle->resno),
											  exprType(expr),
											  exprTypmod(expr),
											  exprCollation(expr),
											  sgc->eqop));
			}
		}

		if (usable)
		{
			KeyJoinFact *fact = add_fact(result, KJF_UNIQUE);

			Assert(attnums != NIL);
			fact->keyPositions = keypositions;
			fact->relid = InvalidOid;
			fact->baseAttnums = list_copy(attnums);
			fact->dependencies = deps;
		}
	}

	pfree(srcvarno);
	pfree(srcattno);

	if (!key_join_surface_facts_has_facts(result))
		return NULL;
	return result;
}

/*
 * project_key_join_facts_from_rte
 *
 *		Project surface facts from one RTE into another surface fact set.
 *
 *		The preservation flags describe which proof meanings the caller preserved.
 *		Foreign-key containment projects whenever its key columns survive;
 *		row-coverage filter handling is strict because filters define the
 *		referenced multiset.
 *
 * Called by:
 *		project_key_join_query_facts
 *		compute_join_output_facts
 */
static void
project_key_join_facts_from_rte(KeyJoinSurfaceFacts *dst, RangeTblEntry *src,
								List **attrmap, bool preserve_notnull,
								bool preserve_unique, bool preserve_rowcoverage,
								Node *filter_qual, Query *filter_query,
								Node *filter_jtnode, Index filter_rtindex,
								List *rowcoverage_key_position_sets,
								List *extra_unique_deps)
{
	bool		tablesample;
	int			natts;

	Assert(src != NULL);
	if (src->keyJoinFacts == NULL)
		return;

	tablesample = (src->rtekind == RTE_RELATION && src->tablesample != NULL);
	Assert(attrmap != NULL);
	natts = list_length(src->eref->colnames);

	foreach_node(KeyJoinFact, old, src->keyJoinFacts->facts)
	{
		KeyJoinFact *new;
		List	   *newpositions;

		switch (old->kind)
		{
			case KJF_NOT_NULL:
				if (!preserve_notnull)
					continue;
				Assert(old->attnum > 0 && old->attnum <= natts);
				if (attrmap[old->attnum] == NIL)
					continue;
				foreach_int(attno, attrmap[old->attnum])
				{
					new = copyObject(old);
					new->attnum = attno;
					dst->facts = lappend(dst->facts, new);
				}
				continue;

			case KJF_UNIQUE:
				if (!preserve_unique)
					continue;
				Assert(list_length(old->baseAttnums) ==
					   list_length(old->keyPositions));
				newpositions = project_key_positions(old->keyPositions, attrmap);
				if (newpositions == NIL)
					continue;
				new = copyObject(old);
				new->keyPositions = newpositions;
				new->dependencies =
					append_dependencies_unique(new->dependencies,
											   extra_unique_deps);
				dst->facts = lappend(dst->facts, new);
				continue;

			case KJF_ROW_COVERAGE:
				{
					bool		rowcollapse_sets_cover = true;

					if (!preserve_rowcoverage || tablesample)
						continue;
					Assert(list_length(old->baseAttnums) ==
						   list_length(old->keyPositions));
					newpositions = project_key_positions(old->keyPositions,
														 attrmap);
					if (newpositions == NIL)
						continue;

					/*
					 * Every row-collapsing stage must cover every key position
					 * with matching output attnum and equality identity.
					 */
					foreach_ptr(List, rowcollapse_key_positions,
								rowcoverage_key_position_sets)
					{
						foreach_node(KeyJoinKeyPosition, keypos, newpositions)
						{
							bool		found = false;

							foreach_node(KeyJoinKeyPosition, rowpos,
										 rowcollapse_key_positions)
							{
								if (!key_position_identity_equal(rowpos, keypos))
									continue;
								foreach_int(attno, rowpos->attnums)
								{
									if (list_member_int(keypos->attnums, attno))
									{
										found = true;
										break;
									}
								}
								if (found)
									break;
							}
							if (!found)
							{
								rowcollapse_sets_cover = false;
								break;
							}
						}
						if (!rowcollapse_sets_cover)
							break;
					}
					if (!rowcollapse_sets_cover)
						continue;

					new = copyObject(old);
					new->keyPositions = newpositions;
					if (!add_filter_conjuncts(&new->filterConjuncts,
											  new->keyPositions, filter_qual,
											  filter_query, filter_jtnode,
											  filter_rtindex, attrmap, natts,
											  &new->dependencies, true))
						continue;
					dst->facts = lappend(dst->facts, new);
					continue;
				}

			default:
				Assert(old->kind == KJF_FOREIGN_KEY);
				Assert(list_length(old->baseAttnums) ==
					   list_length(old->keyPositions));
				newpositions = project_key_positions(old->keyPositions, attrmap);
				if (newpositions == NIL)
					continue;
				new = copyObject(old);
				new->keyPositions = newpositions;
				(void) add_filter_conjuncts(&new->filterConjuncts,
											new->keyPositions, filter_qual,
											filter_query, filter_jtnode,
											filter_rtindex, attrmap, natts,
											&new->dependencies, false);
				dst->facts = lappend(dst->facts, new);
				continue;
		}
	}
}

/*
 * project_key_positions
 *
 *		Project key positions through an attrmap.
 *
 *		Returns NIL if any key position is lost by the projection.
 *
 * Called by:
 *		project_key_join_facts_from_rte
 *		compute_join_output_facts
 */
static List *
project_key_positions(List *keyPositions, List **attrmap)
{
	List	   *result = NIL;

	foreach_node(KeyJoinKeyPosition, oldpos, keyPositions)
	{
		List	   *newattnums = NIL;

		foreach_int(attno, oldpos->attnums)
		{
			Assert(attno > 0);
			newattnums = list_concat(newattnums, list_copy(attrmap[attno]));
		}
		if (newattnums == NIL)
			return NIL;
		result = lappend(result,
						 make_key_position(newattnums, oldpos->typeOid,
										   oldpos->typmod, oldpos->collationOid,
										   oldpos->eqOperator));
	}
	return result;
}

/*
 * make_rowcollapse_key_positions
 *
 *		Build key-position evidence for one row-collapsing stage.
 *
 * Called by:
 *		project_key_join_query_facts
 */
static List *
make_rowcollapse_key_positions(Query *query, List *clauses)
{
	List	   *result = NIL;
	List	   *attnums = NIL;

	foreach_node(SortGroupClause, sgc, clauses)
	{
		TargetEntry *tle = get_sortgroupref_tle(sgc->tleSortGroupRef,
												query->targetList);
		Node	   *expr;

		Assert(tle != NULL);
		Assert(OidIsValid(sgc->eqop));
		if (tle->resjunk)
			continue;
		if (direct_var_from_node((Node *) tle->expr) == NULL)
			continue;
		if (list_member_int(attnums, tle->resno))
			continue;

		expr = (Node *) tle->expr;
		attnums = lappend_int(attnums, tle->resno);
		result =
			lappend(result,
					make_key_position(list_make1_int(tle->resno),
									  exprType(expr),
									  exprTypmod(expr),
									  exprCollation(expr),
									  sgc->eqop));
	}
	return result;
}

/*
 * add_filter_conjuncts
 *
 *		Canonicalize filter conjuncts for a projected proof fact.
 *
 *		Only direct key = value filters matching the key's equality-input
 *		identity are retained; row-coverage callers are strict, while FK
 *		callers ignore unusable conjuncts.
 *
 * Called by:
 *		project_key_join_facts_from_rte
 */
static bool
add_filter_conjuncts(List **dst, List *keyPositions,
					 Node *qual, Query *filter_query, Node *filter_jtnode,
					 Index filter_rtindex, List **filter_attrmap,
					 int filter_natts, List **dependencies, bool strict)
{
	if (qual == NULL)
		return true;
	Assert(filter_attrmap != NULL);

	foreach_ptr(Node, conjunct, make_ands_implicit((Expr *) qual))
	{
		Node	   *canon = NULL;

		Assert(conjunct != NULL);
		if (IsA(conjunct, OpExpr))
		{
			OpExpr	   *op = castNode(OpExpr, conjunct);

			if (list_length(op->args) == 2)
			{
				Node	   *left = linitial(op->args);
				Node	   *right = lsecond(op->args);
				Var		   *leftvar = direct_filter_var_from_node(left);
				int			pos = -1;

				if (leftvar != NULL &&
					!contain_vars_of_level(right, 0) &&
					filter_value_allowed(right))
				{
					List	   *surface_attnums = NIL;

					/*
					 * Map the filter Var through the optional query,
					 * jointree, and attrmap context before matching a
					 * projected key position.
					 */
					Assert(leftvar->varlevelsup == 0);
					Assert(leftvar->varattno > 0);
					if (filter_query != NULL)
					{
						Assert(filter_jtnode != NULL);

						/*
						 * A filter attached above a join tree names an RTE
						 * below that tree.  Map it to the visible surface
						 * column(s) of the filtered jointree.
						 */
						surface_attnums =
							map_var_to_jtnode_surface(filter_query,
													  filter_jtnode,
													  leftvar->varno,
													  leftvar->varattno);
					}
					else
					{
						Assert(filter_rtindex != 0);
						if (leftvar->varno == filter_rtindex)
						{
							/*
							 * A base-relation filter already names its
							 * surface directly.
							 */
							surface_attnums =
								list_make1_int(leftvar->varattno);
						}
					}

					foreach_int(srcattno, surface_attnums)
					{
						List	   *dst_attnums;

						Assert(srcattno > 0);
						Assert(srcattno <= filter_natts);

						/*
						 * Projection may duplicate or drop source columns.
						 * Follow every surviving output attnum.
						 */
						dst_attnums = filter_attrmap[srcattno];

						foreach_int(dstattno, dst_attnums)
						{
							int			keypos =
								key_position_index_for_attnum(keyPositions,
															  dstattno);

							if (keypos < 0)
								continue;
							Assert(pos < 0 || pos == keypos);
							pos = keypos;
						}
					}
				}

				if (pos >= 0)
				{
					KeyJoinKeyPosition *keypos =
						list_nth_node(KeyJoinKeyPosition, keyPositions, pos);
					bool		filter_identity_matches = true;

					Assert(leftvar->vartype == keypos->typeOid);
					Assert(leftvar->vartypmod == keypos->typmod);
					Assert(leftvar->varcollid == keypos->collationOid);
					Assert(exprType(left) == keypos->eqTypeOid);
					Assert(exprTypmod(left) == keypos->eqTypmod);
					if (op->opno != keypos->eqOperator)
						filter_identity_matches = false;
					if (op->inputcollid != keypos->collationOid)
						filter_identity_matches = false;
					if (exprType(right) != keypos->eqTypeOid)
						filter_identity_matches = false;
					if (exprTypmod(right) != keypos->eqTypmod)
						filter_identity_matches = false;
					if (exprCollation(right) != keypos->collationOid)
						filter_identity_matches = false;

					if (filter_identity_matches)
					{
						OpExpr	   *newop = copyObject(op);

						Assert(OidIsValid(op->opfuncid));
						newop->opfuncid = op->opfuncid;
						newop->opcollid = InvalidOid;
						newop->inputcollid = keypos->collationOid;
						newop->args = list_make2(make_filter_param(keypos, pos),
												 copyObject(right));
						newop->location = -1;
						canon = (Node *) newop;
					}
				}
			}
		}

		if (canon != NULL)
		{
			List	   *lockdeps = NIL;

			/*
			 * Lock filter functions so concurrent DDL can't change volatility
			 * between proof validation and stored dependency creation.
			 */
			lockdeps = append_filter_expr_dependencies(lockdeps, canon);
			foreach_node(KeyJoinProofDependency, dep, lockdeps)
			{
				if (dep->classId == ProcedureRelationId)
					LockDatabaseObject(dep->classId, dep->objectId,
									   dep->objectSubId, AccessShareLock);
			}
			Assert(!contain_subplans(canon));
			if (contain_volatile_functions(canon))
			{
				Assert(!strict);
				continue;
			}
			Assert(dependencies != NULL);
			*dependencies = append_filter_expr_dependencies(*dependencies,
															 canon);
			append_filter_conjunct_unique(dst, canon);
			continue;
		}
		if (strict)
			return false;
	}
	return true;
}

/*
 * jtnode_surface_rtindex
 *
 *		Return the RTE index for a jointree node surface.
 *
 * Called by:
 *		ensure_key_join_surface_facts_internal
 *		project_key_join_query_facts
 *		revalidate_query_jointree_proofs
 */
static Index
jtnode_surface_rtindex(Node *jtnode)
{
	Assert(jtnode != NULL);
	Assert(IsA(jtnode, RangeTblRef) || IsA(jtnode, JoinExpr));
	if (IsA(jtnode, RangeTblRef))
		return castNode(RangeTblRef, jtnode)->rtindex;
	return castNode(JoinExpr, jtnode)->rtindex;
}

/*
 * map_var_to_jtnode_surface
 *
 *		Map a Var reference to column numbers on a jointree surface.
 *
 * Called by:
 *		project_key_join_query_facts
 *		add_filter_conjuncts
 */
static List *
map_var_to_jtnode_surface(Query *query, Node *jtnode,
						  Index varno, AttrNumber attno)
{
	Assert(jtnode != NULL);
	Assert(attno > 0);

	if (IsA(jtnode, RangeTblRef))
	{
		Index		rtindex = castNode(RangeTblRef, jtnode)->rtindex;

		return (rtindex == varno) ? list_make1_int(attno) : NIL;
	}

	Assert(IsA(jtnode, JoinExpr));
	{
		JoinExpr   *j = castNode(JoinExpr, jtnode);
		RangeTblEntry *joinrte = rt_fetch(j->rtindex, query->rtable);
		List	   *result = NIL;

		Assert(j->rtindex != varno);

		result = list_concat(result,
							 append_join_input_mapping(joinrte, true,
													   map_var_to_jtnode_surface(query, j->larg,
																				 varno, attno)));
		result = list_concat(result,
							 append_join_input_mapping(joinrte, false,
													   map_var_to_jtnode_surface(query, j->rarg,
																				 varno, attno)));
		return result;
	}
}

/*
 * append_join_input_mapping
 *
 *		Map input attnums from one join side to JOIN output attnums.
 *
 * Called by:
 *		map_var_to_jtnode_surface
 */
static List *
append_join_input_mapping(RangeTblEntry *joinrte, bool leftside,
						  List *input_attnums)
{
	List	   *result = NIL;
	List	   *joincols = leftside ? joinrte->joinleftcols :
		joinrte->joinrightcols;

	foreach_int(input_attno, input_attnums)
	{
		foreach_int(joinattno, joincols)
		{
			int			input_colno = foreach_current_index(joinattno) + 1;

			if (joinattno == input_attno)
				result = list_append_unique_int(result,
												join_output_attno_for_input(joinrte,
																			leftside,
																			input_colno));
		}
	}
	return result;
}

/*
 * join_output_attno_for_input
 *
 *		Return the JOIN output attnum for one input column number.
 *
 * Called by:
 *		append_join_input_mapping
 *		build_join_attrmap
 */
static int
join_output_attno_for_input(RangeTblEntry *joinrte, bool leftside, int input_colno)
{
	if (leftside)
		return input_colno;

	/*
	 * compute_join_output_facts() handles accepted key joins and plain inner
	 * cross joins only.  Neither form can merge JOIN USING columns.
	 */
	Assert(joinrte->joinmergedcols == 0);
	return list_length(joinrte->joinleftcols) +
		input_colno;
}

/*
 * direct_var_from_node_allow_outer
 *
 *		Return a direct Var, rejecting parser coercion wrappers.
 *
 *		Unlike direct_var_from_node, this allows outer Vars.
 *
 * Called by:
 *		direct_var_from_node
 */
static Var *
direct_var_from_node_allow_outer(Node *node)
{
	Assert(node != NULL);

	if (IsA(node, Var))
	{
		Var		   *var = castNode(Var, node);

		return (var->varattno > 0) ? var : NULL;
	}
	if (IsA(node, RelabelType))
	{
		RelabelType *relabel = castNode(RelabelType, node);
		Node	   *arg PG_USED_FOR_ASSERTS_ONLY = (Node *) relabel->arg;

		Assert(arg != NULL);
		/*
		 * The parser constructors that can reach key-join proof either return
		 * the original node for no-op coercions or generate RelabelTypes that
		 * change type, typmod, or collation.  An identity RelabelType here
		 * would be a parse-tree invariant violation, not a testable SQL
		 * shape.
		 */
		Assert(relabel->resulttype != exprType(arg) ||
			   relabel->resulttypmod != exprTypmod(arg) ||
			   relabel->resultcollid != exprCollation(arg));
		return NULL;
	}
	return NULL;
}

/*
 * direct_var_from_node
 *
 *		Return a direct current-query Var.
 *
 * Called by:
 *		project_key_join_query_facts
 *		make_rowcollapse_key_positions
 *		direct_filter_var_from_node
 */
static Var *
direct_var_from_node(Node *node)
{
	Var		   *var = direct_var_from_node_allow_outer(node);

	if (var == NULL)
		return NULL;
	if (var->varlevelsup != 0)
		return NULL;
	return var;
}

/*
 * direct_filter_var_from_node
 *
 *		Return a direct current-query Var from a key-filter operand.  Domain
 *		equality operators are resolved on the base type, so the parser may
 *		wrap a domain Var in a RelabelType before filter canonicalization sees
 *		the OpExpr.  Other RelabelType shapes are not direct key filters.
 *
 * Called by:
 *		add_filter_conjuncts
 */
static Var *
direct_filter_var_from_node(Node *node)
{
	Var		   *var = direct_var_from_node(node);

	if (var != NULL)
		return var;
	Assert(node != NULL);
	if (!IsA(node, RelabelType))
		return NULL;

	{
		RelabelType *relabel = castNode(RelabelType, node);
		int32		baseTypmod;
		Oid			baseType;

		var = direct_var_from_node((Node *) relabel->arg);
		if (var == NULL)
			return NULL;

		baseTypmod = var->vartypmod;
		baseType = key_join_equality_type(var->vartype, var->vartypmod,
										  &baseTypmod);
		if (relabel->resulttype != baseType)
			return NULL;
		if (relabel->resulttypmod != baseTypmod)
			return NULL;
		Assert(relabel->resultcollid == var->varcollid);
		Assert(baseType != var->vartype);
		return var;
	}
}

/*
 * make_filter_param
 *
 *		Build the placeholder Param for a key-join proof filter.
 *
 *		These PARAM_KEYJOIN nodes are parser-private placeholders that may
 *		appear only inside transient KeyJoinSurfaceFacts.filterConjuncts.
 *		Accepted KeyJoinNodes carry only the dependencies consumed by the proof,
 *		never these proof filter expressions.
 *
 * Called by:
 *		add_filter_conjuncts
 */
static Node *
make_filter_param(KeyJoinKeyPosition *keypos, int pos)
{
	Param	   *param = makeNode(Param);

	param->paramkind = PARAM_KEYJOIN;
	param->paramid = pos + 1;
	param->paramtype = keypos->eqTypeOid;
	param->paramtypmod = keypos->eqTypmod;
	param->paramcollid = keypos->collationOid;
	param->location = -1;
	return (Node *) param;
}

/*
 * append_filter_conjunct_unique
 *
 *		Append a filter conjunct unless an equal conjunct is already present.
 *
 * Called by:
 *		add_filter_conjuncts
 *		compute_join_output_facts
 */
static void
append_filter_conjunct_unique(List **dst, Node *conjunct)
{
	if (!list_contains_equal_node(*dst, conjunct))
		*dst = lappend(*dst, conjunct);
}

/*
 * append_filter_expr_dependencies
 *
 *		Append function and operator dependencies used by a filter expression.
 *
 * Called by:
 *		add_filter_conjuncts
 *		compute_join_output_facts
 */
static List *
append_filter_expr_dependencies(List *dependencies, Node *node)
{
	(void) filter_dependency_walker(node, &dependencies);
	return dependencies;
}

/*
 * filter_dependency_walker
 *
 *		Walker callback for collecting filter expression dependencies.
 *
 * Called by:
 *		append_filter_expr_dependencies
 */
static bool
filter_dependency_walker(Node *node, void *context_arg)
{
	List	  **dependencies = (List **) context_arg;

	Assert(node != NULL);

	if (IsA(node, FuncExpr))
		*dependencies =
			append_filter_dependency(*dependencies, ProcedureRelationId,
									 castNode(FuncExpr, node)->funcid);
	else if (IsA(node, OpExpr))
	{
		OpExpr	   *expr = (OpExpr *) node;

		*dependencies = add_op_function_deps(*dependencies, expr->opno,
											 expr->opfuncid);
	}
	return expression_tree_walker(node, filter_dependency_walker, context_arg);
}

/*
 * add_op_function_deps
 *
 *		Append an operator OID and its underlying function as dependencies.
 *
 * Called by:
 *		filter_dependency_walker
 */
static List *
add_op_function_deps(List *deps, Oid opno, Oid opfuncid)
{
	Assert(OidIsValid(opno));
	Assert(OidIsValid(opfuncid));

	deps = append_filter_dependency(deps, OperatorRelationId, opno);
	return append_filter_dependency(deps, ProcedureRelationId, opfuncid);
}

/*
 * append_filter_dependency
 *
 *		Append one filter dependency if the object OID is valid and new.
 *
 * Called by:
 *		filter_dependency_walker
 *		add_op_function_deps
 */
static List *
append_filter_dependency(List *dependencies, Oid classId, Oid objectId)
{
	Assert(OidIsValid(objectId));
	if (dependency_member(dependencies, classId, objectId, 0))
		return dependencies;
	return lappend(dependencies, make_dependency(classId, objectId));
}

/*
 * compute_join_output_facts
 *
 *		Project still-valid surface facts into a join result RTE.
 *
 *		Join type, key-join proof, referencing-side uniqueness, and filters
 *		decide which facts survive.
 *
 * Called by:
 *		ensure_key_join_surface_facts_internal
 */
static void
compute_join_output_facts(JoinExpr *j,
						  Index left_rtindex, RangeTblEntry *left_rte,
						  Index right_rtindex, RangeTblEntry *right_rte,
						  RangeTblEntry *joinrte,
						  KeyJoinFactContext *context)
{
	KeyJoinSurfaceFacts *result;
	List	  **lmap;
	List	  **rmap;

	Assert(context != NULL);
	Assert(left_rte != NULL);
	Assert(right_rte != NULL);
	Assert(joinrte->rtekind == RTE_JOIN);
	Assert(j->keyJoin != NULL ||
		   (j->quals == NULL && j->jointype == JOIN_INNER));

	joinrte->keyJoinFacts = NULL;
	result = makeNode(KeyJoinSurfaceFacts);

	lmap = build_join_attrmap(joinrte, true,
							  list_length(left_rte->eref->colnames));
	rmap = build_join_attrmap(joinrte, false,
							  list_length(right_rte->eref->colnames));

	/* Accepted key joins can export facts for later key-join proofs. */
	if (j->keyJoin != NULL)
	{
		KeyJoinNode *key_join_node;
		bool		referencing_left;
		bool		referenced_left;
		RangeTblEntry *referencing_rte;
		RangeTblEntry *referenced_rte;
		List	  **referencing_map;
		List	  **referenced_map;
		Index		referencing_rtindex;
		Index		referenced_rtindex;
		bool		referenced_preserved;
		bool		preserve_referencing_notnull;
		bool		preserve_referenced_notnull;
		bool		referencing_unique = false;
		List	   *referencing_unique_deps = NIL;

		Assert(IsA(j->keyJoin, KeyJoinNode));
		key_join_node = castNode(KeyJoinNode, j->keyJoin);
		referencing_left = (key_join_node->referencingVarno == left_rtindex);
		referenced_left = (key_join_node->referencedVarno == left_rtindex);
		referencing_rte = referencing_left ? left_rte : right_rte;
		referenced_rte = referenced_left ? left_rte : right_rte;
		referencing_map = referencing_left ? lmap : rmap;
		referenced_map = referenced_left ? lmap : rmap;
		referencing_rtindex = referencing_left ? left_rtindex : right_rtindex;
		referenced_rtindex = referenced_left ? left_rtindex : right_rtindex;
		referenced_preserved = join_preserves_side(j->jointype, referenced_left);
		preserve_referencing_notnull =
			!join_null_extends_side(j->jointype, referencing_left);
		preserve_referenced_notnull =
			!join_null_extends_side(j->jointype, referenced_left);
		Assert(key_join_node->referencingVarno == left_rtindex ||
			   key_join_node->referencingVarno == right_rtindex);
		Assert(key_join_node->referencedVarno == left_rtindex ||
			   key_join_node->referencedVarno == right_rtindex);
		Assert(key_join_node->referencingVarno != key_join_node->referencedVarno);

		Assert(referencing_rte->keyJoinFactsComputed);
		Assert(referenced_rte->keyJoinFactsComputed);
		Assert(referencing_rte->keyJoinFacts != NULL);
		Assert(referenced_rte->keyJoinFacts != NULL);

		/*
		 * Output-fact maintenance: detect referencing-side uniqueness
		 * compatible with the accepted FK join predicate.
		 */
		{
			KeyJoinSurfaceFacts *set = referencing_rte->keyJoinFacts;

			foreach_node(KeyJoinFact, fkfact, set->facts)
			{
				List	   *fk_key_positions;

				if (fkfact->kind != KJF_FOREIGN_KEY)
					continue;
				if (fkfact->constraint != key_join_node->constraint)
					continue;
				if (!select_key_position_parts(key_join_node->referencingAttnums,
											   fkfact->keyPositions,
											   NIL, NULL, &fk_key_positions))
					continue;
				foreach_node(KeyJoinFact, fact, set->facts)
				{
					bool		unique_matches = true;

					if (fact->kind != KJF_UNIQUE)
						continue;

					Assert(list_length(key_join_node->referencingAttnums) ==
						   list_length(fk_key_positions));

					/*
					 * A referencing-side unique fact proves at-most-one match
					 * only if it covers each referencing join column with the
					 * same key identity as the FK positions selected for the
					 * accepted join predicate.
					 */
					foreach_node(KeyJoinKeyPosition, keypos, fact->keyPositions)
					{
						ListCell   *lcattno;
						ListCell   *lcfkpos;
						bool		found = false;

						forboth(lcattno, key_join_node->referencingAttnums,
								lcfkpos, fk_key_positions)
						{
							KeyJoinKeyPosition *fkpos =
								lfirst_node(KeyJoinKeyPosition, lcfkpos);

							if (!list_member_int(keypos->attnums,
												 lfirst_int(lcattno)))
								continue;
							if (!key_position_identity_equal(keypos, fkpos))
							{
								unique_matches = false;
								break;
							}
							found = true;
							break;
						}
						if (!found)
							unique_matches = false;
						if (!unique_matches)
							break;
					}

					if (unique_matches)
					{
						referencing_unique = true;
						referencing_unique_deps =
							append_dependencies_unique(referencing_unique_deps,
													   fact->dependencies);
						break;
					}
				}
				if (referencing_unique)
					break;
			}
		}

		/*
		 * Project both input surfaces through the join output.  Null
		 * extension can kill not-null facts; FK containment survives as
		 * nullable containment, with not-null facts carrying condition 3.
		 */
		project_key_join_facts_from_rte(result, referencing_rte, referencing_map,
										preserve_referencing_notnull, true, true,
										join_filter_for_side(j->jointype,
															 referencing_left,
															 j->joinFilter),
										NULL, NULL, referencing_rtindex, NIL, NIL);
		project_key_join_facts_from_rte(result, referenced_rte, referenced_map,
										preserve_referenced_notnull,
										referencing_unique,
										referenced_preserved,
										join_filter_for_side(j->jointype,
															 referenced_left,
															 j->joinFilter),
										NULL, NULL, referenced_rtindex, NIL,
										referencing_unique_deps);

		/*
		 * Filter propagation: only safe when referenced side is fully
		 * matched.  Filters may move only onto facts rooted in the FK's
		 * referenced relation; FK output facts additionally match their own
		 * constraint OID below.
		 */
		if (!referenced_preserved)
		{
			KeyJoinSurfaceFacts *rfacts = referencing_rte->keyJoinFacts;
			KeyJoinSurfaceFacts *pfacts = referenced_rte->keyJoinFacts;

			foreach_node(KeyJoinFact, source, rfacts->facts)
			{
				List	   *source_selected_base;
				List	   *target_selected_base = NIL;

				if (source->kind != KJF_FOREIGN_KEY)
					continue;
				if (source->constraint != key_join_node->constraint ||
					source->filterConjuncts == NIL)
					continue;
				Assert(list_length(source->baseAttnums) ==
					   list_length(source->referencedAttnums));
				if (!select_key_position_parts(key_join_node->referencingAttnums,
											   source->keyPositions,
											   source->baseAttnums,
											   &source_selected_base, NULL))
					continue;
				foreach_int(srcbase, source_selected_base)
				{
					ListCell   *lcbase;
					ListCell   *lcref;
#ifdef USE_ASSERT_CHECKING
					bool		found = false;
#endif

					forboth(lcbase, source->baseAttnums,
							lcref, source->referencedAttnums)
					{
						if (lfirst_int(lcbase) == srcbase)
						{
							target_selected_base =
								lappend_int(target_selected_base,
											lfirst_int(lcref));
#ifdef USE_ASSERT_CHECKING
							found = true;
#endif
							break;
						}
					}
					Assert(found);
				}

				/*
				 * Match FK and RowCoverage targets to out-facts; the FK case
				 * also requires constraint match so canonical filters can't
				 * cross distinct FKs.
				 */
				foreach_node(KeyJoinFact, target, pfacts->facts)
				{
					List	   *projected_positions;
					List	   *position_map;

					if (target->kind != KJF_FOREIGN_KEY &&
						target->kind != KJF_ROW_COVERAGE)
						continue;
					if (target->relid != source->referencedRelid)
						continue;
					projected_positions =
						project_key_positions(target->keyPositions,
											  referenced_map);
					Assert(projected_positions != NIL);
					position_map =
						make_filter_position_map(source->baseAttnums,
												 source_selected_base,
												 target->baseAttnums,
												 target_selected_base);

					foreach_node(KeyJoinFact, out, result->facts)
					{
						if (out->kind != target->kind)
							continue;
						if (out->kind == KJF_FOREIGN_KEY)
						{
							if (out->constraint != target->constraint)
								continue;
						}
						if (out->relid != target->relid)
							continue;
						if (!int_lists_same_members(out->baseAttnums,
													target->baseAttnums))
							continue;
						if (!equal(out->keyPositions, projected_positions))
							continue;

						/*
						 * Remap each canonical FK-side filter onto the output
						 * fact.  Keep only filters that still constrain the
						 * output key positions after projection.
						 */
						foreach_ptr(Node, conjunct, source->filterConjuncts)
						{
							Node	   *remapped;

							if (!filter_conjunct_can_remap(conjunct,
														   position_map))
								continue;
							remapped = remap_filter_conjunct(conjunct,
															 position_map);
							if (!filter_conjunct_matches_key_positions(remapped,
																	   out->keyPositions))
								continue;

							out->dependencies =
								append_filter_expr_dependencies(out->dependencies,
																remapped);
							append_filter_conjunct_unique(&out->filterConjuncts,
														  remapped);
						}
					}
				}
			}
		}
	}
	else
	{
		RangeTblEntry *rtes[2];
		bool		single_row[2];

		/*
		 * A plain inner cross join with one single-row input preserves the
		 * other side's facts.  Keep this narrow: grouped aggregates without
		 * grouping, and non-set-returning non-volatile functions.
		 */
		Assert(j->quals == NULL);
		Assert(j->jointype == JOIN_INNER);
		Assert(left_rte->keyJoinFactsComputed);
		Assert(right_rte->keyJoinFactsComputed);

		rtes[0] = left_rte;
		rtes[1] = right_rte;
		single_row[0] = false;
		single_row[1] = false;

		for (int i = 0; i < 2; i++)
		{
			RangeTblEntry *rte = rtes[i];

			if (rte->rtekind == RTE_SUBQUERY)
			{
				Query	   *query = rte->subquery;

				/*
				 * Key-join fact computation works on parser Query trees.  Later
				 * planner phases may clear RTE_SUBQUERY.subquery, but those
				 * trees do not reach this code.  Parser-built subquery RTEs
				 * likewise contain only SELECT queries; data-modifying CTEs
				 * remain RTE_CTE.
				 */
				Assert(query != NULL);
				Assert(query->commandType == CMD_SELECT);
				if (query->setOperations != NULL)
					continue;
				if (!query->hasAggs)
					continue;
				if (query->hasTargetSRFs)
					continue;
				if (query->groupClause != NIL)
					continue;
				if (query->groupingSets != NIL)
					continue;
				if (query->havingQual != NULL)
					continue;
				if (query->limitOffset != NULL)
					continue;
				if (query->limitCount != NULL)
					continue;
				if (contain_volatile_functions((Node *) query))
					continue;
				single_row[i] = true;
			}
			else if (rte->rtekind == RTE_FUNCTION && !rte->funcordinality)
			{
				single_row[i] = true;
				foreach_node(RangeTblFunction, rtfunc, rte->functions)
				{
					if (expression_returns_set(rtfunc->funcexpr))
					{
						single_row[i] = false;
						break;
					}
					if (contain_volatile_functions(rtfunc->funcexpr))
					{
						single_row[i] = false;
						break;
					}
				}
			}
		}

		if (single_row[0])
			project_key_join_facts_from_rte(result, right_rte, rmap,
											true, true, true, NULL, NULL, NULL, 0,
											NIL, NIL);
		else if (single_row[1])
			project_key_join_facts_from_rte(result, left_rte, lmap,
											true, true, true, NULL, NULL, NULL, 0,
											NIL, NIL);
	}

	if (key_join_surface_facts_has_facts(result))
		joinrte->keyJoinFacts = result;

	pfree(lmap);
	pfree(rmap);
}

/*
 * build_join_attrmap
 *
 *		Build a mapping from one join input to JOIN output columns.
 *
 * Called by:
 *		compute_join_output_facts
 */
static List **
build_join_attrmap(RangeTblEntry *joinrte, bool leftside, int nattrs)
{
	List	  **attrmap = palloc0((nattrs + 1) * sizeof(List *));
	List	   *joincols = leftside ? joinrte->joinleftcols :
		joinrte->joinrightcols;

	foreach_int(input_attno, joincols)
	{
		int			jcolno =
			join_output_attno_for_input(joinrte, leftside,
										foreach_current_index(input_attno) + 1);

		Assert(input_attno > 0);
		Assert(input_attno <= nattrs);
		attrmap[input_attno] = lappend_int(attrmap[input_attno], jcolno);
	}
	return attrmap;
}

/*
 * join_null_extends_side
 *
 *		Return true if the join type can null-extend the requested side.
 *
 * Called by:
 *		compute_join_output_facts
 */
static bool
join_null_extends_side(JoinType jointype, bool leftside)
{
	return jointype == JOIN_FULL ||
		(leftside ? jointype == JOIN_RIGHT : jointype == JOIN_LEFT);
}

/*
 * join_filter_for_side
 *
 *		Return the join filter applicable to facts projected from one side.
 *
 * Called by:
 *		compute_join_output_facts
 */
static Node *
join_filter_for_side(JoinType jointype, bool leftside, Node *filter)
{
	Assert(jointype == JOIN_INNER || jointype == JOIN_LEFT ||
		   jointype == JOIN_RIGHT || jointype == JOIN_FULL);

	if (jointype == JOIN_INNER)
		return filter;
	if (jointype == JOIN_LEFT)
		return leftside ? NULL : filter;
	if (jointype == JOIN_RIGHT)
		return leftside ? filter : NULL;
	return NULL;
}

/*
 * storedNodeContainsKeyJoin
 *
 *		Report whether a stored query or expression tree contains a
 *		KeyJoinNode.
 *
 * Called by:
 *		no local callers
 */
bool
storedNodeContainsKeyJoin(Node *node)
{
	return stored_node_contains_key_join_walker(node, NULL);
}

/*
 * stored_node_contains_key_join_walker
 *
 *		Walk stored query and expression nodes looking for KeyJoinNodes.
 *
 * Called by:
 *		storedNodeContainsKeyJoin
 */
static bool
stored_node_contains_key_join_walker(Node *node, void *context)
{
	if (node == NULL)
		return false;

	if (IsA(node, KeyJoinNode))
		return true;

	if (IsA(node, JoinExpr))
	{
		JoinExpr   *join = castNode(JoinExpr, node);

		if (stored_node_contains_key_join_walker(join->keyJoin, context))
			return true;
	}

	if (IsA(node, Query))
		return query_tree_walker(castNode(Query, node),
								 stored_node_contains_key_join_walker,
								 context, 0);

	return expression_tree_walker(node, stored_node_contains_key_join_walker,
								  context);
}

/*
 * revalidatedStoredKeyJoinProofsAreSafe
 *
 *		Return true if a revalidated copy of a stored key-join tree can be
 *		discarded without updating the stored tree or catalog dependencies.
 *
 *		A replayed proof may lose proof dependencies: stale extra pg_depend
 *		entries are conservative and can be removed by explicitly recreating
 *		the owning object.  It must not gain a new dependency or otherwise
 *		change stored semantics, because then the stored object would be left
 *		without a pg_depend edge for an object the proof now needs.
 *
 *		This may mutate the revalidated copy by restoring old dependency
 *		lists before the final equality check.  Callers discard that copy.
 *
 * Called by:
 *		no local callers
 */
bool
revalidatedStoredKeyJoinProofsAreSafe(Node *stored, Node *revalidated)
{
	List	   *stored_nodes;
	List	   *revalidated_nodes;
	ListCell   *lc1;
	ListCell   *lc2;

	if (equal(stored, revalidated))
		return true;

	stored_nodes = collect_key_join_nodes(stored);
	revalidated_nodes = collect_key_join_nodes(revalidated);
	Assert(list_length(stored_nodes) == list_length(revalidated_nodes));

	forboth(lc1, stored_nodes, lc2, revalidated_nodes)
	{
		KeyJoinNode *old_node = castNode(KeyJoinNode, lfirst(lc1));
		KeyJoinNode *new_node = castNode(KeyJoinNode, lfirst(lc2));

		if (!dependency_list_is_subset(new_node->notNullConstraints,
									   old_node->notNullConstraints))
			return false;
		if (!dependency_list_is_subset(new_node->proofDependencies,
									   old_node->proofDependencies))
			return false;

		/*
		 * Normalize away the only allowed difference: dependency shrinkage.
		 * The revalidated copy is discarded by callers, so it is safe to
		 * restore the old dependency lists before the final whole-tree check.
		 */
		new_node->notNullConstraints = old_node->notNullConstraints;
		new_node->proofDependencies = old_node->proofDependencies;
	}

	/*
	 * After normalization, every non-dependency field and every executable
	 * expression must match the stored tree exactly.
	 */
	return equal(stored, revalidated);
}

/*
 * collect_key_join_nodes
 *
 *		Return KeyJoinNodes in tree-walk order.
 *
 * Called by:
 *		revalidatedStoredKeyJoinProofsAreSafe
 */
static List *
collect_key_join_nodes(Node *node)
{
	List	   *result = NIL;

	(void) collect_key_join_nodes_walker(node, &result);
	return result;
}

/*
 * collect_key_join_nodes_walker
 *
 *		Walk stored query and expression nodes collecting KeyJoinNodes.
 *
 * Called by:
 *		collect_key_join_nodes
 */
static bool
collect_key_join_nodes_walker(Node *node, void *context)
{
	List	  **result = (List **) context;

	if (node == NULL)
		return false;

	if (IsA(node, KeyJoinNode))
	{
		*result = lappend(*result, node);
		return false;
	}

	if (IsA(node, JoinExpr))
	{
		JoinExpr   *join = castNode(JoinExpr, node);

		(void) collect_key_join_nodes_walker(join->keyJoin, context);
	}

	if (IsA(node, Query))
		return query_tree_walker(castNode(Query, node),
								 collect_key_join_nodes_walker,
								 context, 0);

	return expression_tree_walker(node, collect_key_join_nodes_walker,
								  context);
}

/*
 * dependency_list_is_subset
 *
 *		Return true if every dependency entry in candidate also appears in
 *		superset.
 *
 * Called by:
 *		revalidatedStoredKeyJoinProofsAreSafe
 */
static bool
dependency_list_is_subset(List *candidate, List *superset)
{
	foreach_node(KeyJoinProofDependency, dep, candidate)
	{
		if (!dependency_member(superset, dep->classId, dep->objectId,
							   dep->objectSubId))
			return false;
	}
	return true;
}

/*
 * revalidateStoredKeyJoinProofsInNode
 *
 *		Revalidate and rebuild key-join proofs contained in a copied stored
 *		query or expression tree.
 *
 * Called by:
 *		no local callers
 */
void
revalidateStoredKeyJoinProofsInNode(Node *node)
{
	(void) revalidate_stored_key_join_node_walker(node, NULL);
}

/*
 * revalidate_stored_key_join_node_walker
 *
 *		Walk an expression tree and revalidate any nested Query nodes with the
 *		supplied owning-query stack, if any.  A top-level Query is handled by
 *		revalidateStoredKeyJoinProofsInQuery(), which performs query-specific
 *		fact rebuilding and recurses into its own subqueries.
 *
 * Called by:
 *		revalidateStoredKeyJoinProofsInNode
 *		revalidate_stored_key_join_proofs_in_query
 */
static bool
revalidate_stored_key_join_node_walker(Node *node, void *context)
{
	if (node == NULL)
		return false;

	if (IsA(node, Query))
	{
		revalidate_stored_key_join_proofs_in_query(castNode(Query, node),
												   (KeyJoinQueryStack *) context);
		return false;
	}

	return expression_tree_walker(node, revalidate_stored_key_join_node_walker,
								  context);
}

/*
 * revalidateStoredKeyJoinProofsInQuery
 *
 *		Revalidate and rebuild key-join proofs in a copied stored query tree.
 *
 * Called by:
 *		no local callers
 */
void
revalidateStoredKeyJoinProofsInQuery(Query *query)
{
	revalidate_stored_key_join_proofs_in_query(query, NULL);
}

/*
 * revalidate_stored_key_join_proofs_in_query
 *
 *		Revalidate one stored Query with an explicit stack of owning query
 *		levels.
 *
 *		Stored key-join proofs may depend on CTE ownership, outer references,
 *		and facts cached on RTEs.  This routine rebuilds that context for one
 *		Query level, clears stale cached facts, recursively revalidates CTEs
 *		and FROM-subqueries with the current Query as their parent frame, then
 *		revalidates key joins in the jointree.  The final expression walker
 *		handles expression subqueries and intentionally skips FROM/CTE
 *		subqueries already handled in owner-aware passes above.
 *
 * Called by:
 *		ensure_key_join_surface_facts_internal
 *		revalidate_stored_key_join_node_walker
 *		revalidateStoredKeyJoinProofsInQuery
 */
static void
revalidate_stored_key_join_proofs_in_query(Query *query,
										   KeyJoinQueryStack *parent_stack)
{
	KeyJoinQueryStack qs;

	Assert(query != NULL);
	Assert(IsA(query, Query));

	qs.parent = parent_stack;
	qs.query = query;

	/* Clear stale RTE facts before demand-driven revalidation. */
	foreach_node(RangeTblEntry, rte, query->rtable)
	{
		rte->keyJoinFacts = NULL;
		rte->keyJoinFactsComputed = false;
	}

	foreach_node(CommonTableExpr, cte, query->cteList)
	{
		if (IsA(cte->ctequery, Query))
			revalidate_stored_key_join_proofs_in_query((Query *) cte->ctequery,
													   &qs);
	}

	foreach_node(RangeTblEntry, rte, query->rtable)
	{
		if (rte->rtekind == RTE_SUBQUERY)
		{
			Assert(rte->subquery != NULL);
			revalidate_stored_key_join_proofs_in_query(rte->subquery, &qs);
		}
	}

	revalidate_query_jointree_proofs(query, (Node *) query->jointree, &qs);

	(void) query_tree_walker(query, revalidate_stored_key_join_node_walker,
							 &qs,
							 QTW_IGNORE_RT_SUBQUERIES |
							 QTW_IGNORE_CTE_SUBQUERIES);
}

/*
 * revalidate_query_jointree_proofs
 *
 *		Recompute join facts and revalidate stored KeyJoinNode proofs.
 *
 * Called by:
 *		revalidate_stored_key_join_proofs_in_query
 */
static void
revalidate_query_jointree_proofs(Query *query, Node *jtnode,
								 KeyJoinQueryStack *query_stack)
{
	Assert(query != NULL);
	Assert(jtnode != NULL);

	if (IsA(jtnode, FromExpr))
	{
		foreach_ptr(Node, child, castNode(FromExpr, jtnode)->fromlist)
			revalidate_query_jointree_proofs(query, child, query_stack);
		return;
	}
	if (!IsA(jtnode, JoinExpr))
		return;

	{
		JoinExpr   *j = castNode(JoinExpr, jtnode);
		Index		left_rtindex = jtnode_surface_rtindex(j->larg);
		Index		right_rtindex = jtnode_surface_rtindex(j->rarg);
		RangeTblEntry *left_rte;
		RangeTblEntry *right_rte;

		revalidate_query_jointree_proofs(query, j->larg, query_stack);
		revalidate_query_jointree_proofs(query, j->rarg, query_stack);

		Assert(left_rtindex != 0);
		Assert(right_rtindex != 0);
		Assert(j->rtindex != 0);

		left_rte = rt_fetch(left_rtindex, query->rtable);
		right_rte = rt_fetch(right_rtindex, query->rtable);

		if (j->keyJoin != NULL)
		{
			KeyJoinNode *key_join;
			bool		referencing_left;
			RangeTblEntry *referencing_rte;
			RangeTblEntry *referenced_rte;
			List	   *referenced_args;
			List	   *referencing_args;
			List	   *locations;
			Node	   *key_quals = NULL;
			int			nkeys;
			KeyJoinMatch match;
			KeyJoinFactContext context;

			Assert(IsA(j->keyJoin, KeyJoinNode));
			key_join = castNode(KeyJoinNode, j->keyJoin);
			referencing_left = (key_join->referencingVarno == left_rtindex);
			referencing_rte = referencing_left ? left_rte : right_rte;
			referenced_rte =
				(key_join->referencedVarno == left_rtindex) ? left_rte : right_rte;
			nkeys = list_length(key_join->referencingAttnums);

			Assert(key_join->referencingVarno == left_rtindex ||
				   key_join->referencingVarno == right_rtindex);
			Assert(key_join->referencedVarno == left_rtindex ||
				   key_join->referencedVarno == right_rtindex);
			Assert(key_join->referencingVarno != key_join->referencedVarno);

			memset(&context, 0, sizeof(context));
			context.query = query;
			context.query_stack = query_stack;
			context.revalidating_stored_query = true;
			ensure_key_join_surface_facts_internal(&context, referencing_rte);
			ensure_key_join_surface_facts_internal(&context, referenced_rte);

			if (!find_key_join_match(referencing_rte, referenced_rte,
									 key_join->referencingAttnums,
									 key_join->referencedAttnums,
									 !join_preserves_side(j->jointype,
														  referencing_left),
									 &match))

				/*
				 * No parser_errposition() here.  This fires during
				 * stored-query revalidation, when the original SQL text is
				 * long gone and the source location is no longer meaningful.
				 */
				ereport(ERROR,
						(errcode(ERRCODE_INVALID_FOREIGN_KEY),
						 errmsg("key join cannot be proven from available constraints")));

			/*
			 * Join-local FILTER quals are kept separately on joinFilter and
			 * are re-merged after the key equality is rebuilt.  Stored
			 * non-filtered joins keep the key equality directly in quals;
			 * filtered joins store quals as key equality AND joinFilter.
			 */
			if (j->joinFilter == NULL)
				key_quals = j->quals;
			else
			{
				BoolExpr   *andexpr;

				Assert(j->quals != NULL);
				Assert(IsA(j->quals, BoolExpr));
				andexpr = castNode(BoolExpr, j->quals);
				Assert(andexpr->boolop == AND_EXPR);
				Assert(list_length(andexpr->args) == 2);
				Assert(equal(lsecond(andexpr->args), j->joinFilter));

				key_quals = linitial(andexpr->args);
			}
			Assert(key_quals != NULL);

			referenced_args = NIL;
			referencing_args = NIL;
			locations = NIL;

			/*
			 * The parser stores a single key equality as the OpExpr itself.
			 * Multi-column key joins are stored as an AND whose arguments are
			 * the key equalities in key-column order.
			 */
			if (nkeys == 1)
				extract_key_join_qual_arg(key_quals, &referenced_args,
										  &referencing_args, &locations);
			else
			{
				BoolExpr   *andexpr;

				Assert(IsA(key_quals, BoolExpr));
				andexpr = castNode(BoolExpr, key_quals);
				Assert(andexpr->boolop == AND_EXPR);
				Assert(list_length(andexpr->args) == nkeys);

				foreach_ptr(Node, qual, andexpr->args)
					extract_key_join_qual_arg(qual, &referenced_args,
											  &referencing_args, &locations);
			}

			/*
			 * Rebuild the executable equality quals from the stored argument
			 * expressions and the freshly proven equality operators.  The
			 * stored operators might no longer be the proof operators after
			 * DDL, but the argument expressions preserve locations and parse
			 * structure.
			 */
			key_quals = build_key_join_quals(referenced_args,
											 referencing_args,
											 match.eqoperators,
											 match.eqtypes,
											 match.eqtypmods,
											 locations, -1);
			j->quals = key_quals;
			if (j->joinFilter != NULL)
				j->quals = (Node *) makeBoolExpr(AND_EXPR,
												 list_make2(j->quals,
															j->joinFilter),
												 -1);

			key_join->constraint = match.constraint;
			key_join->notNullConstraints = match.notnulldeps;
			key_join->proofDependencies = match.proofdeps;
		}
	}
}

/*
 * extract_key_join_qual_arg
 *
 *		Extract one stored key equality.  The parser constructs key equality
 *		operators as referenced-column argument first, referencing-column
 *		argument second.
 *
 * Called by:
 *		revalidate_query_jointree_proofs
 */
static void
extract_key_join_qual_arg(Node *qual, List **referenced_args,
						  List **referencing_args, List **locations)
{
	OpExpr	   *op;

	Assert(qual != NULL);
	Assert(IsA(qual, OpExpr));
	op = castNode(OpExpr, qual);
	Assert(list_length(op->args) == 2);

	*referenced_args = lappend(*referenced_args,
							   copyObject(linitial(op->args)));
	*referencing_args = lappend(*referencing_args,
								copyObject(lsecond(op->args)));
	*locations = lappend_int(*locations, op->location);
}

/*
 * key_join_surface_facts_has_facts
 *
 *		Return true if a KeyJoinSurfaceFacts contains any facts.
 *
 * Called by:
 *		compute_key_join_relation_facts
 *		project_key_join_query_facts
 *		compute_join_output_facts
 */
static bool
key_join_surface_facts_has_facts(KeyJoinSurfaceFacts *set)
{
	Assert(set != NULL);
	return set->facts != NIL;
}

/*
 * add_fact
 *
 *		Append a fresh surface fact of the given kind to a fact set.
 *
 * Called by:
 *		compute_key_join_relation_facts
 *		project_key_join_query_facts
 *		add_paired_row_coverage
 */
static KeyJoinFact *
add_fact(KeyJoinSurfaceFacts *set, KeyJoinFactKind kind)
{
	KeyJoinFact *fact = makeNode(KeyJoinFact);

	fact->kind = kind;
	set->facts = lappend(set->facts, fact);
	return fact;
}

/*
 * add_paired_row_coverage
 *
 *		Append a row-coverage fact paired with the current unique or FK fact.
 *
 * Called by:
 *		compute_key_join_relation_facts
 */
static void
add_paired_row_coverage(KeyJoinSurfaceFacts *set, List *keypositions,
						Oid relid, List *baseAttnums, List *deps)
{
	KeyJoinFact *cov = add_fact(set, KJF_ROW_COVERAGE);

	cov->keyPositions = keypositions;
	cov->relid = relid;
	cov->baseAttnums = baseAttnums;
	cov->dependencies = deps;
}

/*
 * key_join_collation_is_usable
 *
 *		Return true for noncollatable keys or deterministic collations.
 *
 * Called by:
 *		compute_key_join_relation_facts
 *		project_key_join_query_facts
 */
static bool
key_join_collation_is_usable(Oid collationOid)
{
	if (!OidIsValid(collationOid))
		return true;
	return get_collation_isdeterministic(collationOid);
}

/*
 * key_join_equality_operator_is_usable
 *
 *		Check whether an operator is usable as key-join equality.
 *
 *		The operator must be exact, immutable, strict, boolean, and
 *		non-set-returning for one equality input type.
 *
 * Called by:
 *		compute_key_join_relation_facts
 *		project_key_join_query_facts
 */
static bool
key_join_equality_operator_is_usable(Oid opno, Oid typeOid, List **dependencies)
{
	RegProcedure funcid;
	Oid			lefttype;
	Oid			righttype;
	Oid			rettype;
	bool		signature_ok;

	if (!OidIsValid(opno))
		return false;

	op_input_types(opno, &lefttype, &righttype);
	rettype = get_op_rettype(opno);

	signature_ok = (lefttype == typeOid);
	signature_ok &= (righttype == typeOid);
	signature_ok &= (rettype == BOOLOID);
	if (!signature_ok)
		return false;

	funcid = get_opcode(opno);
	Assert(RegProcedureIsValid(funcid));

	LockDatabaseObject(ProcedureRelationId, (Oid) funcid, 0, AccessShareLock);
	if (get_func_retset((Oid) funcid))
		return false;
	if (func_volatile((Oid) funcid) != PROVOLATILE_IMMUTABLE)
		return false;
	if (!func_strict((Oid) funcid))
		return false;

	Assert(dependencies != NULL);
	if (!dependency_member(*dependencies, ProcedureRelationId, (Oid) funcid, 0))
		*dependencies = lappend(*dependencies,
								make_dependency(ProcedureRelationId,
												(Oid) funcid));
	return true;
}

/*
 * key_join_equality_type
 *
 *		Return the type identity used by equality operators for a key value.
 *		Domains remain the exposed proof identity, but their equality
 *		operators are resolved and executed on the base type.
 *
 * Called by:
 *		direct_filter_var_from_node
 *		compute_key_join_relation_facts
 *		project_key_join_query_facts
 *		make_key_position
 */
static Oid
key_join_equality_type(Oid typeOid, int32 typmod, int32 *eqTypmod)
{
	int32		localTypmod = typmod;
	Oid			result;

	result = getBaseTypeAndTypmod(typeOid, &localTypmod);
	if (eqTypmod != NULL)
		*eqTypmod = localTypmod;
	return result;
}

/*
 * make_key_positions_from_attrnums
 *
 *		Build key-position descriptors for relation attribute numbers.
 *
 * Called by:
 *		compute_key_join_relation_facts
 */
static List *
make_key_positions_from_attrnums(TupleDesc tupdesc, const AttrNumber *attnums,
								 int nattnums, const Oid *eqOperators)
{
	List	   *result = NIL;

	for (int i = 0; i < nattnums; i++)
	{
		Form_pg_attribute att;

		Assert(attnums[i] > 0);
		att = TupleDescAttr(tupdesc, attnums[i] - 1);
		result = lappend(result,
						 make_key_position(list_make1_int(att->attnum),
										   att->atttypid, att->atttypmod,
										   att->attcollation, eqOperators[i]));
	}
	return result;
}

/*
 * make_key_position
 *
 *		Build one KeyJoinKeyPosition node.
 *
 * Called by:
 *		project_key_join_query_facts
 *		project_key_positions
 *		make_rowcollapse_key_positions
 *		make_key_positions_from_attrnums
 */
static KeyJoinKeyPosition *
make_key_position(List *attnums, Oid typeOid, int32 typmod,
				  Oid collationOid, Oid eqOperator)
{
	KeyJoinKeyPosition *pos = makeNode(KeyJoinKeyPosition);
	int32		eqTypmod;

	pos->attnums = list_copy(attnums);
	pos->typeOid = typeOid;
	pos->typmod = typmod;
	pos->collationOid = collationOid;
	pos->eqTypeOid = key_join_equality_type(typeOid, typmod, &eqTypmod);
	pos->eqTypmod = eqTypmod;
	pos->eqOperator = eqOperator;
	return pos;
}

/*
 * list_make_attrnums
 *
 *		Build an integer list from an AttrNumber array.
 *
 * Called by:
 *		compute_key_join_relation_facts
 */
static List *
list_make_attrnums(const AttrNumber *attnums, int nattnums)
{
	List	   *result = NIL;

	for (int i = 0; i < nattnums; i++)
		result = lappend_int(result, attnums[i]);
	return result;
}
