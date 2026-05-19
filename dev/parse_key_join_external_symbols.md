# External PostgreSQL Symbols Used by parse_key_join.c

Source inspected: `src/backend/parser/parse_key_join.c`

Scope: source-visible PostgreSQL C function identifiers and struct/struct-typedef type identifiers used in `parse_key_join.c`, excluding anything defined in that file. Static inline functions in headers are included. Pure function-like macros are not listed unless the same identifier has a C function definition in `src/`.

Counts: 62 functions, 39 struct-like types.

## Functions

### `DeconstructFkConstraintRow`

Definition path: `src/backend/catalog/pg_constraint.c:1538`

```c
/*
 * Extract data from the pg_constraint tuple of a foreign-key constraint.
 *
 * All arguments save the first are output arguments.  All output arguments
 * other than numfks, conkey and confkey can be passed as NULL if caller
 * doesn't need them.
 */
void
DeconstructFkConstraintRow(HeapTuple tuple, int *numfks,
						   AttrNumber *conkey, AttrNumber *confkey,
						   Oid *pf_eq_oprs, Oid *pp_eq_oprs, Oid *ff_eq_oprs,
						   int *num_fk_del_set_cols, AttrNumber *fk_del_set_cols);
```

### `GETSTRUCT`

Definition path: `src/include/access/htup_details.h:719`

```c
/*
 * GETSTRUCT - given a HeapTuple pointer, return address of the user data
 */
static inline void *
GETSTRUCT(const HeapTupleData *tuple);
```

### `LockDatabaseObject`

Definition path: `src/backend/storage/lmgr/lmgr.c:1008`

```c
/*
 *		LockDatabaseObject
 *
 * Obtain a lock on a general object of the current database.  Don't use
 * this for shared objects (such as tablespaces).  It's unwise to apply it
 * to relations, also, since a lock taken this way will NOT conflict with
 * locks taken via LockRelation and friends.
 */
void
LockDatabaseObject(Oid classid, Oid objid, uint16 objsubid,
				   LOCKMODE lockmode);
```

### `ObjectIdGetDatum`

Definition path: `src/include/postgres.h:252`

```c
/*
 * ObjectIdGetDatum
 *		Returns datum representation for an object identifier.
 */
static inline Datum
ObjectIdGetDatum(Oid X);
```

### `RelationGetFKeyList`

Definition path: `src/backend/utils/cache/relcache.c:4722`

```c
/*
 * RelationGetFKeyList -- get a list of foreign key info for the relation
 *
 * Returns a list of ForeignKeyCacheInfo structs, one per FK constraining
 * the given relation.  This data is a direct copy of relevant fields from
 * pg_constraint.  The list items are in no particular order.
 *
 * CAUTION: the returned list is part of the relcache's data, and could
 * vanish in a relcache entry reset.  Callers must inspect or copy it
 * before doing anything that might trigger a cache flush, such as
 * system catalog accesses.  copyObject() can be used if desired.
 * (We define it this way because current callers want to filter and
 * modify the list entries anyway, so copying would be a waste of time.)
 */
List *
RelationGetFKeyList(Relation relation);
```

### `RelationGetIndexExpressions`

Definition path: `src/backend/utils/cache/relcache.c:5088`

```c
/*
 * RelationGetIndexExpressions -- get the index expressions for an index
 *
 * We cache the result of transforming pg_index.indexprs into a node tree.
 * If the rel is not an index or has no expressional columns, we return NIL.
 * Otherwise, the returned tree is copied into the caller's memory context.
 * (We don't want to return a pointer to the relcache copy, since it could
 * disappear due to relcache invalidation.)
 */
List *
RelationGetIndexExpressions(Relation relation);
```

### `RelationGetIndexList`

Definition path: `src/backend/utils/cache/relcache.c:4827`

```c
/*
 * RelationGetIndexList -- get a list of OIDs of indexes on this relation
 *
 * The index list is created only if someone requests it.  We scan pg_index
 * to find relevant indexes, and add the list to the relcache entry so that
 * we won't have to compute it again.  Note that shared cache inval of a
 * relcache entry will delete the old list and set rd_indexvalid to false,
 * so that we must recompute the index list on next request.  This handles
 * creation or deletion of an index.
 *
 * Indexes that are marked not indislive are omitted from the returned list.
 * Such indexes are expected to be dropped momentarily, and should not be
 * touched at all by any caller of this function.
 *
 * The returned list is guaranteed to be sorted in order by OID.  This is
 * needed by the executor, since for index types that we obtain exclusive
 * locks on when updating the index, all backends must lock the indexes in
 * the same order or we will get deadlocks (see ExecOpenIndices()).  Any
 * consistent ordering would do, but ordering by OID is easy.
 *
 * Since shared cache inval causes the relcache's copy of the list to go away,
 * we return a copy of the list palloc'd in the caller's context.  The caller
 * may list_free() the returned list after scanning it. This is necessary
 * since the caller will typically be doing syscache lookups on the relevant
 * indexes, and syscache lookup could cause SI messages to be processed!
 *
 * In exactly the same way, we update rd_pkindex, which is the OID of the
 * relation's primary key index if any, else InvalidOid; and rd_replidindex,
 * which is the pg_class OID of an index to be used as the relation's
 * replication identity index, or InvalidOid if there is no such index.
 */
List *
RelationGetIndexList(Relation relation);
```

### `RelationGetIndexPredicate`

Definition path: `src/backend/utils/cache/relcache.c:5201`

```c
/*
 * RelationGetIndexPredicate -- get the index predicate for an index
 *
 * We cache the result of transforming pg_index.indpred into an implicit-AND
 * node tree (suitable for use in planning).
 * If the rel is not an index or has no predicate, we return NIL.
 * Otherwise, the returned tree is copied into the caller's memory context.
 * (We don't want to return a pointer to the relcache copy, since it could
 * disappear due to relcache invalidation.)
 */
List *
RelationGetIndexPredicate(Relation relation);
```

### `ReleaseSysCache`

Definition path: `src/backend/utils/cache/syscache.c:265`

```c
/*
 * ReleaseSysCache
 *		Release previously grabbed reference count on a tuple
 */
void
ReleaseSysCache(HeapTuple tuple);
```

### `SearchSysCache1`

Definition path: `src/backend/utils/cache/syscache.c:221`

```c
HeapTuple
SearchSysCache1(SysCacheIdentifier cacheId,
				Datum key1);
```

### `TupleDescAttr`

Definition path: `src/include/access/tupdesc.h:178`

```c
/* Accessor for the i'th FormData_pg_attribute element of tupdesc. */
static inline FormData_pg_attribute *
TupleDescAttr(TupleDesc tupdesc, int i);
```

### `applyRelabelType`

Definition path: `src/backend/nodes/nodeFuncs.c:641`

```c
/*
 * applyRelabelType
 *		Add a RelabelType node if needed to make the expression expose
 *		the specified type, typmod, and collation.
 *
 * This is primarily intended to be used during planning.  Therefore, it must
 * maintain the post-eval_const_expressions invariants that there are not
 * adjacent RelabelTypes, and that the tree is fully const-folded (hence,
 * we mustn't return a RelabelType atop a Const).  If we do find a Const,
 * we'll modify it in-place if "overwrite_ok" is true; that should only be
 * passed as true if caller knows the Const is newly generated.
 */
Node *
applyRelabelType(Node *arg, Oid rtype, int32 rtypmod, Oid rcollid,
				 CoercionForm rformat, int rlocation, bool overwrite_ok);
```

### `check_functions_in_node`

Definition path: `src/backend/nodes/nodeFuncs.c:1917`

```c
/*
 *	check_functions_in_node -
 *	  apply checker() to each function OID contained in given expression node
 *
 * Returns true if the checker() function does; for nodes representing more
 * than one function call, returns true if the checker() function does so
 * for any of those functions.  Returns false if node does not invoke any
 * SQL-visible function.  Caller must not pass node == NULL.
 *
 * This function examines only the given node; it does not recurse into any
 * sub-expressions.  Callers typically prefer to keep control of the recursion
 * for themselves, in case additional checks should be made, or because they
 * have special rules about which parts of the tree need to be visited.
 *
 * Note: we ignore MinMaxExpr, SQLValueFunction, XmlExpr, CoerceToDomain,
 * and NextValueExpr nodes, because they do not contain SQL function OIDs.
 * However, they can invoke SQL-visible functions, so callers should take
 * thought about how to treat them.
 */
bool
check_functions_in_node(Node *node, check_function_callback checker,
						void *context);
```

### `contain_subplans`

Definition path: `src/backend/optimizer/util/clauses.c:343`

```c
/*
 * contain_subplans
 *	  Recursively search for subplan nodes within a clause.
 *
 * If we see a SubLink node, we will return true.  This is only possible if
 * the expression tree hasn't yet been transformed by subselect.c.  We do not
 * know whether the node will produce a true subplan or just an initplan,
 * but we make the conservative assumption that it will be a subplan.
 *
 * Returns true if any subplan found.
 */
bool
contain_subplans(Node *clause);
```

### `contain_vars_of_level`

Definition path: `src/backend/optimizer/util/var.c:444`

```c
/*
 * contain_vars_of_level
 *	  Recursively scan a clause to discover whether it contains any Var nodes
 *	  of the specified query level.
 *
 *	  Returns true if any such Var found.
 *
 * Will recurse into sublinks.  Also, may be invoked directly on a Query.
 */
bool
contain_vars_of_level(Node *node, int levelsup);
```

### `contain_volatile_functions`

Definition path: `src/backend/optimizer/util/clauses.c:551`

```c
/*
 * contain_volatile_functions
 *	  Recursively search for volatile functions within a clause.
 *
 * Returns true if any volatile function (or operator implemented by a
 * volatile function) is found. This test prevents, for example,
 * invalid conversions of volatile expressions into indexscan quals.
 *
 * This will give the right answer only for clauses that have been put
 * through expression preprocessing.  Callers outside the planner typically
 * should use contain_volatile_functions_after_planning() instead, for the
 * reasons given there.
 *
 * We will recursively look into Query nodes (i.e., SubLink sub-selects)
 * but not into SubPlans.  This is a bit odd, but intentional.  If we are
 * looking at a SubLink, we are probably deciding whether a query tree
 * transformation is safe, and a contained sub-select should affect that;
 * for example, duplicating a sub-select containing a volatile function
 * would be bad.  However, once we've got to the stage of having SubPlans,
 * subsequent planning need not consider volatility within those, since
 * the executor won't change its evaluation rules for a SubPlan based on
 * volatility.
 *
 * For some node types, for example, RestrictInfo and PathTarget, we cache
 * whether we found any volatile functions or not and reuse that value in any
 * future checks for that node.  All of the logic for determining if the
 * cached value should be set to VOLATILITY_NOVOLATILE or VOLATILITY_VOLATILE
 * belongs in this function.  Any code which makes changes to these nodes
 * which could change the outcome this function must set the cached value back
 * to VOLATILITY_UNKNOWN.  That allows this function to redetermine the
 * correct value during the next call, should we need to redetermine if the
 * node contains any volatile functions again in the future.
 */
bool
contain_volatile_functions(Node *clause);
```

### `equal`

Definition path: `src/backend/nodes/equalfuncs.c:223`

```c
/*
 * equal
 *	  returns whether two nodes are equal
 */
bool
equal(const void *a, const void *b);
```

### `errcode`

Definition path: `src/backend/utils/error/elog.c:875`

```c
/*
 * errcode --- add SQLSTATE error code to the current error
 *
 * The code is expected to be represented as per MAKE_SQLSTATE().
 */
int
errcode(int sqlerrcode);
```

### `errmsg`

Definition path: `src/backend/utils/error/elog.c:1094`

```c
/*
 * errmsg --- add a primary error message text to the current error
 *
 * In addition to the usual %-escapes recognized by printf, "%m" in
 * fmt is replaced by the error message for the caller's value of errno.
 *
 * Note: no newline is needed at the end of the fmt string, since
 * ereport will provide one for the output methods that need it.
 */
int
errmsg(const char *fmt,...);
```

### `exprCollation`

Definition path: `src/backend/nodes/nodeFuncs.c:826`

```c
/*
 *	exprCollation -
 *	  returns the Oid of the collation of the expression's result.
 *
 * Note: expression nodes that can invoke functions generally have an
 * "inputcollid" field, which is what the function should use as collation.
 * That is the resolved common collation of the node's inputs.  It is often
 * but not always the same as the result collation; in particular, if the
 * function produces a non-collatable result type from collatable inputs
 * or vice versa, the two are different.
 */
Oid
exprCollation(const Node *expr);
```

### `exprType`

Definition path: `src/backend/nodes/nodeFuncs.c:42`

```c
/*
 *	exprType -
 *	  returns the Oid of the type of the expression's result.
 */
Oid
exprType(const Node *expr);
```

### `exprTypmod`

Definition path: `src/backend/nodes/nodeFuncs.c:304`

```c
/*
 *	exprTypmod -
 *	  returns the type-specific modifier of the expression's result type,
 *	  if it can be determined.  In many cases, it can't and we return -1.
 */
int32
exprTypmod(const Node *expr);
```

### `expression_returns_set`

Definition path: `src/backend/nodes/nodeFuncs.c:768`

```c
/*
 * expression_returns_set
 *	  Test whether an expression returns a set result.
 *
 * Because we use expression_tree_walker(), this can also be applied to
 * whole targetlists; it'll produce true if any one of the tlist items
 * returns a set.
 */
bool
expression_returns_set(Node *clause);
```

### `findNotNullConstraintAttnum`

Definition path: `src/backend/catalog/pg_constraint.c:592`

```c
/*
 * Find and return a copy of the pg_constraint tuple that implements a
 * (possibly not valid) not-null constraint for the given column of the
 * given relation.  If no such constraint exists, return NULL.
 *
 * XXX This would be easier if we had pg_attribute.notnullconstr with the OID
 * of the constraint that implements the not-null constraint for that column.
 * I'm not sure it's worth the catalog bloat and de-normalization, however.
 */
HeapTuple
findNotNullConstraintAttnum(Oid relid, AttrNumber attnum);
```

### `func_strict`

Definition path: `src/backend/utils/cache/lsyscache.c:1954`

```c
/*
 * func_strict
 *		Given procedure id, return the function's proisstrict flag.
 */
bool
func_strict(Oid funcid);
```

### `func_volatile`

Definition path: `src/backend/utils/cache/lsyscache.c:1973`

```c
/*
 * func_volatile
 *		Given procedure id, return the function's provolatile flag.
 */
char
func_volatile(Oid funcid);
```

### `getBaseTypeAndTypmod`

Definition path: `src/backend/utils/cache/lsyscache.c:2733`

```c
/*
 * getBaseTypeAndTypmod
 *		If the given type is a domain, return its base type and typmod;
 *		otherwise return the type's own OID, and leave *typmod unchanged.
 *
 * Note that the "applied typmod" should be -1 for every domain level
 * above the bottommost; therefore, if the passed-in typid is indeed
 * a domain, *typmod should be -1.
 */
Oid
getBaseTypeAndTypmod(Oid typid, int32 *typmod);
```

### `get_collation_isdeterministic`

Definition path: `src/backend/utils/cache/lsyscache.c:1173`

```c
bool
get_collation_isdeterministic(Oid colloid);
```

### `get_func_retset`

Definition path: `src/backend/utils/cache/lsyscache.c:1935`

```c
/*
 * get_func_retset
 *		Given procedure id, return the function's proretset flag.
 */
bool
get_func_retset(Oid funcid);
```

### `get_index_constraint`

Definition path: `src/backend/catalog/pg_depend.c:1061`

```c
/*
 * get_index_constraint
 *		Given the OID of an index, return the OID of the owning unique,
 *		primary-key, or exclusion constraint, or InvalidOid if there
 *		is no owning constraint.
 */
Oid
get_index_constraint(Oid indexId);
```

### `get_op_rettype`

Definition path: `src/backend/utils/cache/lsyscache.c:1526`

```c
/*
 * get_op_rettype
 *		Given operator oid, return the operator's result type.
 */
Oid
get_op_rettype(Oid opno);
```

### `get_opcode`

Definition path: `src/backend/utils/cache/lsyscache.c:1478`

```c
/*
 * get_opcode
 *
 *		Returns the regproc id of the routine used to implement an
 *		operator given the operator oid.
 */
RegProcedure
get_opcode(Oid opno);
```

### `get_opfamily_member_for_cmptype`

Definition path: `src/backend/utils/cache/lsyscache.c:199`

```c
/*
 * get_opfamily_member_for_cmptype
 *		Get the OID of the operator that implements the specified comparison
 *		type with the specified datatypes for the specified opfamily.
 *
 * Returns InvalidOid if there is no mapping for the comparison type or no
 * pg_amop entry for the given keys.
 */
Oid
get_opfamily_member_for_cmptype(Oid opfamily, Oid lefttype, Oid righttype,
								CompareType cmptype);
```

### `get_sortgroupref_tle`

Definition path: `src/backend/optimizer/util/tlist.c:354`

```c
/*
 * get_sortgroupref_tle
 *		Find the targetlist entry matching the given SortGroupRef index,
 *		and return it.
 */
TargetEntry *
get_sortgroupref_tle(Index sortref, List *targetList);
```

### `get_view_query`

Definition path: `src/backend/rewrite/rewriteHandler.c:2575`

```c
/*
 * get_view_query - get the Query from a view's _RETURN rule.
 *
 * Caller should have verified that the relation is a view, and therefore
 * we should find an ON SELECT action.
 *
 * Note that the pointer returned is into the relcache and therefore must
 * be treated as read-only to the caller and not modified or scribbled on.
 */
Query *
get_view_query(Relation view);
```

### `has_subclass`

Definition path: `src/backend/catalog/pg_inherits.c:356`

```c
/*
 * has_subclass - does this relation have any children?
 *
 * In the current implementation, has_subclass returns whether a
 * particular class *might* have a subclass. It will not return the
 * correct result if a class had a subclass which was later dropped.
 * This is because relhassubclass in pg_class is not updated immediately
 * when a subclass is dropped, primarily because of concurrency concerns.
 *
 * Currently has_subclass is only used as an efficiency hack to skip
 * unnecessary inheritance searches, so this is OK.  Note that ANALYZE
 * on a childless table will clean up the obsolete relhassubclass flag.
 *
 * Although this doesn't actually touch pg_inherits, it seems reasonable
 * to keep it here since it's normally used with the other routines here.
 */
bool
has_subclass(Oid relationId);
```

### `heap_freetuple`

Definition path: `src/backend/access/common/heaptuple.c:1372`

```c
/*
 * heap_freetuple
 */
void
heap_freetuple(HeapTuple htup);
```

### `index_close`

Definition path: `src/backend/access/index/indexam.c:178`

```c
/* ----------------
 *		index_close - close an index relation
 *
 *		If lockmode is not "NoLock", we then release the specified lock.
 *
 *		Note that it is often sensible to hold a lock beyond index_close;
 *		in that case, the lock is released automatically at xact end.
 * ----------------
 */
void
index_close(Relation relation, LOCKMODE lockmode);
```

### `index_open`

Definition path: `src/backend/access/index/indexam.c:134`

```c
/* ----------------
 *		index_open - open an index relation by relation OID
 *
 *		If lockmode is not "NoLock", the specified kind of lock is
 *		obtained on the index.  (Generally, NoLock should only be
 *		used if the caller knows it has some appropriate lock on the
 *		index already.)
 *
 *		An error is raised if the index does not exist.
 *
 *		This is a convenience routine adapted for indexscan use.
 *		Some callers may prefer to use relation_open directly.
 * ----------------
 */
Relation
index_open(Oid relationId, LOCKMODE lockmode);
```

### `lappend`

Definition path: `src/backend/nodes/list.c:339`

```c
/*
 * Append a pointer to the list. A pointer to the modified list is
 * returned. Note that this function may or may not destructively
 * modify the list; callers should always use this function's return
 * value, rather than continuing to use the pointer passed as the
 * first argument.
 */
List *
lappend(List *list, void *datum);
```

### `lappend_int`

Definition path: `src/backend/nodes/list.c:357`

```c
/*
 * Append an integer to the specified list. See lappend()
 */
List *
lappend_int(List *list, int datum);
```

### `lappend_oid`

Definition path: `src/backend/nodes/list.c:375`

```c
/*
 * Append an OID to the specified list. See lappend()
 */
List *
lappend_oid(List *list, Oid datum);
```

### `list_append_unique_int`

Definition path: `src/backend/nodes/list.c:1368`

```c
/*
 * This variant of list_append_unique() operates upon lists of integers.
 */
List *
list_append_unique_int(List *list, int datum);
```

### `list_concat`

Definition path: `src/backend/nodes/list.c:561`

```c
/*
 * Concatenate list2 to the end of list1, and return list1.
 *
 * This is equivalent to lappend'ing each element of list2, in order, to list1.
 * list1 is destructively changed, list2 is not.  (However, in the case of
 * pointer lists, list1 and list2 will point to the same structures.)
 *
 * Callers should be sure to use the return value as the new pointer to the
 * concatenated list: the 'list1' input pointer may or may not be the same
 * as the returned pointer.
 *
 * Note that this takes at least time proportional to the length of list2.
 * It'd typically be the case that we have to enlarge list1's storage,
 * probably adding time proportional to the length of list1.
 */
List *
list_concat(List *list1, const List *list2);
```

### `list_copy`

Definition path: `src/backend/nodes/list.c:1573`

```c
/*
 * Return a shallow copy of the specified list.
 */
List *
list_copy(const List *oldlist);
```

### `list_head`

Definition path: `src/include/nodes/pg_list.h:128`

```c
/* Fetch address of list's first cell; NULL if empty list */
static inline ListCell *
list_head(const List *l);
```

### `list_length`

Definition path: `src/include/nodes/pg_list.h:152`

```c
/* Fetch list's length */
static inline int
list_length(const List *l);
```

### `list_member_int`

Definition path: `src/backend/nodes/list.c:702`

```c
/*
 * Return true iff the integer 'datum' is a member of the list.
 */
bool
list_member_int(const List *list, int datum);
```

### `list_nth`

Definition path: `src/include/nodes/pg_list.h:331`

```c
/*
 * Return the pointer value contained in the n'th element of the
 * specified list. (List elements begin at 0.)
 */
static inline void *
list_nth(const List *list, int n);
```

### `list_nth_int`

Definition path: `src/include/nodes/pg_list.h:342`

```c
/*
 * Return the integer value contained in the n'th element of the
 * specified list.
 */
static inline int
list_nth_int(const List *list, int n);
```

### `lnext`

Definition path: `src/include/nodes/pg_list.h:375`

```c
/*
 * Get the address of the next cell after "c" within list "l", or NULL if none.
 */
static inline ListCell *
lnext(const List *l, const ListCell *c);
```

### `makeBoolExpr`

Definition path: `src/backend/nodes/makefuncs.c:420`

```c
/*
 * makeBoolExpr -
 *	  creates a BoolExpr node
 */
Expr *
makeBoolExpr(BoolExprType boolop, List *args, int location);
```

### `makeVar`

Definition path: `src/backend/nodes/makefuncs.c:66`

```c
/*
 * makeVar -
 *	  creates a Var node
 */
Var *
makeVar(int varno,
		AttrNumber varattno,
		Oid vartype,
		int32 vartypmod,
		Oid varcollid,
		Index varlevelsup);
```

### `make_ands_implicit`

Definition path: `src/backend/nodes/makefuncs.c:810`

```c
List *
make_ands_implicit(Expr *clause);
```

### `markNullableIfNeeded`

Definition path: `src/backend/parser/parse_relation.c:1046`

```c
/*
 * markNullableIfNeeded
 *		If the RTE referenced by the Var is nullable by outer join(s)
 *		at this point in the query, set var->varnullingrels to show that.
 */
void
markNullableIfNeeded(ParseState *pstate, Var *var);
```

### `op_input_types`

Definition path: `src/backend/utils/cache/lsyscache.c:1551`

```c
/*
 * op_input_types
 *
 *		Returns the left and right input datatypes for an operator
 *		(InvalidOid if not relevant).
 */
void
op_input_types(Oid opno, Oid *lefttype, Oid *righttype);
```

### `palloc0`

Definition path: `src/backend/utils/mmgr/mcxt.c:1417; src/common/fe_memutils.c:121`

Source excerpt: `src/backend/utils/mmgr/mcxt.c:1417`

```c
void *
palloc0(Size size);
```

Source excerpt: `src/common/fe_memutils.c:121`

```c
void *
palloc0(Size size);
```

### `parser_errposition`

Definition path: `src/backend/parser/parse_node.c:106`

```c
/*
 * parser_errposition
 *		Report a parse-analysis-time cursor position, if possible.
 *
 * This is expected to be used within an ereport() call.  The return value
 * is a dummy (always 0, in fact).
 *
 * The locations stored in raw parsetrees are byte offsets into the source
 * string.  We have to convert them to 1-based character indexes for reporting
 * to clients.  (We do things this way to avoid unnecessary overhead in the
 * normal non-error case: computing character indexes would be much more
 * expensive than storing token offsets.)
 */
int
parser_errposition(ParseState *pstate, int location);
```

### `pfree`

Definition path: `src/backend/utils/mmgr/mcxt.c:1616; src/common/fe_memutils.c:133`

Source excerpt: `src/backend/utils/mmgr/mcxt.c:1616`

```c
/*
 * pfree
 *		Release an allocated chunk.
 */
void
pfree(void *pointer);
```

Source excerpt: `src/common/fe_memutils.c:133`

```c
void
pfree(void *pointer);
```

### `relation_close`

Definition path: `src/backend/access/common/relation.c:206`

```c
/* ----------------
 *		relation_close - close any relation
 *
 *		If lockmode is not "NoLock", we then release the specified lock.
 *
 *		Note that it is often sensible to hold a lock beyond relation_close;
 *		in that case, the lock is released automatically at xact end.
 * ----------------
 */
void
relation_close(Relation relation, LOCKMODE lockmode);
```

### `relation_open`

Definition path: `src/backend/access/common/relation.c:48`

```c
/* ----------------
 *		relation_open - open any relation by relation OID
 *
 *		If lockmode is not "NoLock", the specified kind of lock is
 *		obtained on the relation.  (Generally, NoLock should only be
 *		used if the caller knows it has some appropriate lock on the
 *		relation already.)
 *
 *		An error is raised if the relation does not exist.
 *
 *		NB: a "relation" is anything with a pg_class entry.  The caller is
 *		expected to check whether the relkind is something it can handle.
 * ----------------
 */
Relation
relation_open(Oid relationId, LOCKMODE lockmode);
```

### `set_opfuncid`

Definition path: `src/backend/nodes/nodeFuncs.c:1879`

```c
/*
 * set_opfuncid
 *		Set the opfuncid (procedure OID) in an OpExpr node,
 *		if it hasn't been set already.
 *
 * Because of struct equivalence, this can also be used for
 * DistinctExpr and NullIfExpr nodes.
 */
void
set_opfuncid(OpExpr *opexpr);
```

## Structs

### `BoolExpr`

C kind: struct typedef

Definition path: `src/include/nodes/primnodes.h:973`

```c
typedef struct BoolExpr
{
	pg_node_attr(custom_read_write)

	Expr		xpr;
	BoolExprType boolop;
	List	   *args;			/* arguments to this expression */
	ParseLoc	location;		/* token location, or -1 if unknown */
} BoolExpr;
```

### `CoerceViaIO`

C kind: struct typedef

Definition path: `src/include/nodes/primnodes.h:1245`

```c
/* ----------------
 * CoerceViaIO
 *
 * CoerceViaIO represents a type coercion between two types whose textual
 * representations are compatible, implemented by invoking the source type's
 * typoutput function then the destination type's typinput function.
 * ----------------
 */
typedef struct CoerceViaIO
{
	Expr		xpr;
	Expr	   *arg;			/* input expression */
	Oid			resulttype;		/* output type of coercion */
	/* output typmod is not stored, but is presumed -1 */
	/* OID of collation, or InvalidOid if none */
	Oid			resultcollid pg_node_attr(query_jumble_ignore);
	/* how to display this node */
	CoercionForm coerceformat pg_node_attr(query_jumble_ignore);
	ParseLoc	location;		/* token location, or -1 if unknown */
} CoerceViaIO;
```

### `CollateExpr`

C kind: struct typedef

Definition path: `src/include/nodes/primnodes.h:1317`

```c
/*----------
 * CollateExpr - COLLATE
 *
 * The planner replaces CollateExpr with RelabelType during expression
 * preprocessing, so execution never sees a CollateExpr.
 *----------
 */
typedef struct CollateExpr
{
	Expr		xpr;
	Expr	   *arg;			/* input expression */
	Oid			collOid;		/* collation's OID */
	ParseLoc	location;		/* token location, or -1 if unknown */
} CollateExpr;
```

### `CommonTableExpr`

C kind: struct typedef

Definition path: `src/include/nodes/parsenodes.h:1873`

```c
typedef struct CommonTableExpr
{
	NodeTag		type;

	/*
	 * Query name (never qualified).  The string name is included in the query
	 * jumbling because RTE_CTE RTEs need it.
	 */
	char	   *ctename;
	/* optional list of column names */
	List	   *aliascolnames pg_node_attr(query_jumble_ignore);
	CTEMaterialize ctematerialized; /* is this an optimization fence? */
	/* SelectStmt/InsertStmt/etc before parse analysis, Query afterwards: */
	Node	   *ctequery;		/* the CTE's subquery */
	CTESearchClause *search_clause pg_node_attr(query_jumble_ignore);
	CTECycleClause *cycle_clause pg_node_attr(query_jumble_ignore);
	ParseLoc	location;		/* token location, or -1 if unknown */
	/* These fields are set during parse analysis: */
	/* is this CTE actually recursive? */
	bool		cterecursive pg_node_attr(query_jumble_ignore);

	/*
	 * Number of RTEs referencing this CTE (excluding internal
	 * self-references), irrelevant for query jumbling.
	 */
	int			cterefcount pg_node_attr(query_jumble_ignore);
	/* list of output column names */
	List	   *ctecolnames pg_node_attr(query_jumble_ignore);
	/* OID list of output column type OIDs */
	List	   *ctecoltypes pg_node_attr(query_jumble_ignore);
	/* integer list of output column typmods */
	List	   *ctecoltypmods pg_node_attr(query_jumble_ignore);
	/* OID list of column collation OIDs */
	List	   *ctecolcollations pg_node_attr(query_jumble_ignore);
} CommonTableExpr;
```

### `Const`

C kind: struct typedef

Definition path: `src/include/nodes/primnodes.h:324`

```c
/*
 * Const
 *
 * Note: for varlena data types, we make a rule that a Const node's value
 * must be in non-extended form (4-byte header, no compression or external
 * references).  This ensures that the Const node is self-contained and makes
 * it more likely that equal() will see logically identical values as equal.
 *
 * Only the constant type OID is relevant for the query jumbling.
 */
typedef struct Const
{
	pg_node_attr(custom_copy_equal, custom_read_write)

	Expr		xpr;
	/* pg_type OID of the constant's datatype */
	Oid			consttype;
	/* typmod value, if any */
	int32		consttypmod pg_node_attr(query_jumble_ignore);
	/* OID of collation, or InvalidOid if none */
	Oid			constcollid pg_node_attr(query_jumble_ignore);
	/* typlen of the constant's datatype */
	int			constlen pg_node_attr(query_jumble_ignore);
	/* the constant's value */
	Datum		constvalue pg_node_attr(query_jumble_ignore);
	/* whether the constant is null (if true, constvalue is undefined) */
	bool		constisnull pg_node_attr(query_jumble_ignore);

	/*
	 * Whether this datatype is passed by value.  If true, then all the
	 * information is stored in the Datum.  If false, then the Datum contains
	 * a pointer to the information.
	 */
	bool		constbyval pg_node_attr(query_jumble_ignore);

	/*
	 * token location, or -1 if unknown.  All constants are tracked as
	 * locations in query jumbling, to be marked as parameters.
	 */
	ParseLoc	location pg_node_attr(query_jumble_location);
} Const;
```

### `Expr`

C kind: struct typedef

Definition path: `src/include/nodes/primnodes.h:189`

```c
/*
 * Expr - generic superclass for executable-expression nodes
 *
 * All node types that are used in executable expression trees should derive
 * from Expr (that is, have Expr as their first field).  Since Expr only
 * contains NodeTag, this is a formality, but it is an easy form of
 * documentation.  See also the ExprState node types in execnodes.h.
 */
typedef struct Expr
{
	pg_node_attr(abstract)

	NodeTag		type;
} Expr;
```

### `ForeignKeyCacheInfo`

C kind: struct typedef

Definition path: `src/include/utils/rel.h:273`

```c
/*
 * ForeignKeyCacheInfo
 *		Information the relcache can cache about foreign key constraints
 *
 * This is basically just an image of relevant columns from pg_constraint.
 * We make it a subclass of Node so that copyObject() can be used on a list
 * of these, but we also ensure it is a "flat" object without substructure,
 * so that list_free_deep() is sufficient to free such a list.
 * The per-FK-column arrays can be fixed-size because we allow at most
 * INDEX_MAX_KEYS columns in a foreign key constraint.
 *
 * Currently, we mostly cache fields of interest to the planner, but the set
 * of fields has already grown the constraint OID for other uses.
 */
typedef struct ForeignKeyCacheInfo
{
	pg_node_attr(no_equal, no_read, no_query_jumble)

	NodeTag		type;
	/* oid of the constraint itself */
	Oid			conoid;
	/* relation constrained by the foreign key */
	Oid			conrelid;
	/* relation referenced by the foreign key */
	Oid			confrelid;
	/* number of columns in the foreign key */
	int			nkeys;

	/* Is enforced ? */
	bool		conenforced;

	/*
	 * these arrays each have nkeys valid entries:
	 */
	/* cols in referencing table */
	AttrNumber	conkey[INDEX_MAX_KEYS] pg_node_attr(array_size(nkeys));
	/* cols in referenced table */
	AttrNumber	confkey[INDEX_MAX_KEYS] pg_node_attr(array_size(nkeys));
	/* PK = FK operator OIDs */
	Oid			conpfeqop[INDEX_MAX_KEYS] pg_node_attr(array_size(nkeys));
} ForeignKeyCacheInfo;
```

### `Form_pg_attribute`

C kind: struct typedef

Definition path: `src/include/catalog/pg_attribute.h:39`

Source excerpt: `src/include/catalog/pg_attribute.h:37`

```c
/* ----------------
 *		pg_attribute definition.  cpp turns this into
 *		typedef struct FormData_pg_attribute
 *
 *		If you change the following, make sure you change the structs for
 *		system attributes in catalog/heap.c also.
 *		You may need to change catalog/genbki.pl as well.
 * ----------------
 */
BEGIN_CATALOG_STRUCT

CATALOG(pg_attribute,1249,AttributeRelationId) BKI_BOOTSTRAP BKI_ROWTYPE_OID(75,AttributeRelation_Rowtype_Id) BKI_SCHEMA_MACRO
{
	Oid			attrelid BKI_LOOKUP(pg_class);	/* OID of relation containing
												 * this attribute */
	NameData	attname;		/* name of attribute */

	/*
	 * atttypid is the OID of the instance in Catalog Class pg_type that
	 * defines the data type of this attribute (e.g. int4).  Information in
	 * that instance is redundant with the attlen, attbyval, and attalign
	 * attributes of this instance, so they had better match or Postgres will
	 * fail.  In an entry for a dropped column, this field is set to zero
	 * since the pg_type entry may no longer exist; but we rely on attlen,
	 * attbyval, and attalign to still tell us how large the values in the
	 * table are.
	 */
	Oid			atttypid BKI_LOOKUP_OPT(pg_type);

	/*
	 * attlen is a copy of the typlen field from pg_type for this attribute.
	 * See atttypid comments above.
	 */
	int16		attlen;

	/*
	 * attnum is the "attribute number" for the attribute:	A value that
	 * uniquely identifies this attribute within its class. For user
	 * attributes, Attribute numbers are greater than 0 and not greater than
	 * the number of attributes in the class. I.e. if the Class pg_class says
	 * that Class XYZ has 10 attributes, then the user attribute numbers in
	 * Class pg_attribute must be 1-10.
	 *
	 * System attributes have attribute numbers less than 0 that are unique
	 * within the class, but not constrained to any particular range.
	 *
	 * Note that (attnum - 1) is often used as the index to an array.
	 */
	int16		attnum;

	/*
	 * atttypmod records type-specific data supplied at table creation time
	 * (for example, the max length of a varchar field).  It is passed to
	 * type-specific input and output functions as the third argument. The
	 * value will generally be -1 for types that do not need typmod.
	 */
	int32		atttypmod BKI_DEFAULT(-1);

	/*
	 * attndims is the declared number of dimensions, if an array type,
	 * otherwise zero.
	 */
	int16		attndims;

	/*
	 * attbyval is a copy of the typbyval field from pg_type for this
	 * attribute.  See atttypid comments above.
	 */
	bool		attbyval;

	/*
	 * attalign is a copy of the typalign field from pg_type for this
	 * attribute.  See atttypid comments above.
	 */
	char		attalign;

	/*----------
	 * attstorage tells for VARLENA attributes, what the heap access
	 * methods can do to it if a given tuple doesn't fit into a page.
	 * Possible values are as for pg_type.typstorage (see TYPSTORAGE macros).
	 *----------
	 */
	char		attstorage;

	/*
	 * attcompression sets the current compression method of the attribute.
	 * Typically this is InvalidCompressionMethod ('\0') to specify use of the
	 * current default setting (see default_toast_compression).  Otherwise,
	 * 'p' selects pglz compression, while 'l' selects LZ4 compression.
	 * However, this field is ignored whenever attstorage does not allow
	 * compression.
	 */
	char		attcompression BKI_DEFAULT('\0');

	/*
	 * Whether a (possibly invalid) not-null constraint exists for the column
	 */
	bool		attnotnull;

	/* Has DEFAULT value or not */
	bool		atthasdef BKI_DEFAULT(f);

	/* Has a missing value or not */
	bool		atthasmissing BKI_DEFAULT(f);

	/* One of the ATTRIBUTE_IDENTITY_* constants below, or '\0' */
	char		attidentity BKI_DEFAULT('\0');

	/* One of the ATTRIBUTE_GENERATED_* constants below, or '\0' */
	char		attgenerated BKI_DEFAULT('\0');

	/* Is dropped (ie, logically invisible) or not */
	bool		attisdropped BKI_DEFAULT(f);

	/*
	 * This flag specifies whether this column has ever had a local
	 * definition.  It is set for normal non-inherited columns, but also for
	 * columns that are inherited from parents if also explicitly listed in
	 * CREATE TABLE INHERITS.  It is also set when inheritance is removed from
	 * a table with ALTER TABLE NO INHERIT.  If the flag is set, the column is
	 * not dropped by a parent's DROP COLUMN even if this causes the column's
	 * attinhcount to become zero.
	 */
	bool		attislocal BKI_DEFAULT(t);

	/* Number of times inherited from direct parent relation(s) */
	int16		attinhcount BKI_DEFAULT(0);

	/* attribute's collation, if any */
	Oid			attcollation BKI_LOOKUP_OPT(pg_collation);

#ifdef CATALOG_VARLEN			/* variable-length/nullable fields start here */
	/* NOTE: The following fields are not present in tuple descriptors. */

	/*
	 * attstattarget is the target number of statistics datapoints to collect
	 * during VACUUM ANALYZE of this column.  A zero here indicates that we do
	 * not wish to collect any stats about this column. A null value here
	 * indicates that no value has been explicitly set for this column, so
	 * ANALYZE should use the default setting.
	 *
	 * int16 is sufficient for the current max value (MAX_STATISTICS_TARGET).
	 */
	int16		attstattarget BKI_DEFAULT(_null_) BKI_FORCE_NULL;

	/* Column-level access permissions */
	aclitem		attacl[1] BKI_DEFAULT(_null_);

	/* Column-level options */
	text		attoptions[1] BKI_DEFAULT(_null_);

	/* Column-level FDW options */
	text		attfdwoptions[1] BKI_DEFAULT(_null_);

	/*
	 * Missing value for added columns. This is a one element array which lets
	 * us store a value of the attribute type here.
	 */
	anyarray	attmissingval BKI_DEFAULT(_null_);
#endif
} FormData_pg_attribute;

END_CATALOG_STRUCT
```

Source excerpt: `src/include/catalog/pg_attribute.h:206`

```c
/* ----------------
 *		Form_pg_attribute corresponds to a pointer to a tuple with
 *		the format of pg_attribute relation.
 * ----------------
 */
typedef FormData_pg_attribute *Form_pg_attribute;
```

### `Form_pg_constraint`

C kind: struct typedef

Definition path: `src/include/catalog/pg_constraint.h:33`

Source excerpt: `src/include/catalog/pg_constraint.h:31`

```c
/* ----------------
 *		pg_constraint definition.  cpp turns this into
 *		typedef struct FormData_pg_constraint
 * ----------------
 */
BEGIN_CATALOG_STRUCT

CATALOG(pg_constraint,2606,ConstraintRelationId)
{
	Oid			oid;			/* oid */

	/*
	 * conname + connamespace is deliberately not unique; we allow, for
	 * example, the same name to be used for constraints of different
	 * relations.  This is partly for backwards compatibility with past
	 * Postgres practice, and partly because we don't want to have to obtain a
	 * global lock to generate a globally unique name for a nameless
	 * constraint.  We associate a namespace with constraint names only for
	 * SQL-spec compatibility.
	 *
	 * However, we do require conname to be unique among the constraints of a
	 * single relation or domain.  This is enforced by a unique index on
	 * conrelid + contypid + conname.
	 */
	NameData	conname;		/* name of this constraint */
	Oid			connamespace BKI_LOOKUP(pg_namespace);	/* OID of namespace
														 * containing constraint */
	char		contype;		/* constraint type; see codes below */
	bool		condeferrable;	/* deferrable constraint? */
	bool		condeferred;	/* deferred by default? */
	bool		conenforced;	/* enforced constraint? */
	bool		convalidated;	/* constraint has been validated? */

	/*
	 * conrelid and conkey are only meaningful if the constraint applies to a
	 * specific relation (this excludes domain constraints and assertions).
	 * Otherwise conrelid is 0 and conkey is NULL.
	 */
	Oid			conrelid BKI_LOOKUP_OPT(pg_class);	/* relation this
													 * constraint constrains */

	/*
	 * contypid links to the pg_type row for a domain if this is a domain
	 * constraint.  Otherwise it's 0.
	 *
	 * For SQL-style global ASSERTIONs, both conrelid and contypid would be
	 * zero. This is not presently supported, however.
	 */
	Oid			contypid BKI_LOOKUP_OPT(pg_type);	/* domain this constraint
													 * constrains */

	/*
	 * conindid links to the index supporting the constraint, if any;
	 * otherwise it's 0.  This is used for unique, primary-key, and exclusion
	 * constraints, and less obviously for foreign-key constraints (where the
	 * index is a unique index on the referenced relation's referenced
	 * columns).  Notice that the index is on conrelid in the first case but
	 * confrelid in the second.
	 */
	Oid			conindid BKI_LOOKUP_OPT(pg_class);	/* index supporting this
													 * constraint */

	/*
	 * If this constraint is on a partition inherited from a partitioned
	 * table, this is the OID of the corresponding constraint in the parent.
	 */
	Oid			conparentid BKI_LOOKUP_OPT(pg_constraint);

	/*
	 * These fields, plus confkey, are only meaningful for a foreign-key
	 * constraint.  Otherwise confrelid is 0 and the char fields are spaces.
	 */
	Oid			confrelid BKI_LOOKUP_OPT(pg_class); /* relation referenced by
													 * foreign key */
	char		confupdtype;	/* foreign key's ON UPDATE action */
	char		confdeltype;	/* foreign key's ON DELETE action */
	char		confmatchtype;	/* foreign key's match type */

	/* Has a local definition (hence, do not drop when coninhcount is 0) */
	bool		conislocal;

	/* Number of times inherited from direct parent relation(s) */
	int16		coninhcount;

	/* Has a local definition and cannot be inherited */
	bool		connoinherit;

	/*
	 * For primary keys, unique constraints, and foreign keys, signifies the
	 * last column uses overlaps instead of equals.
	 */
	bool		conperiod;

#ifdef CATALOG_VARLEN			/* variable-length fields start here */

	/*
	 * Columns of conrelid that the constraint applies to, if known (this is
	 * NULL for trigger constraints)
	 */
	int16		conkey[1];

	/*
	 * If a foreign key, the referenced columns of confrelid
	 */
	int16		confkey[1];

	/*
	 * If a foreign key, the OIDs of the PK = FK equality/overlap operators
	 * for each column of the constraint
	 */
	Oid			conpfeqop[1] BKI_LOOKUP(pg_operator);

	/*
	 * If a foreign key, the OIDs of the PK = PK equality/overlap operators
	 * for each column of the constraint (i.e., equality for the referenced
	 * columns)
	 */
	Oid			conppeqop[1] BKI_LOOKUP(pg_operator);

	/*
	 * If a foreign key, the OIDs of the FK = FK equality/overlap operators
	 * for each column of the constraint (i.e., equality for the referencing
	 * columns)
	 */
	Oid			conffeqop[1] BKI_LOOKUP(pg_operator);

	/*
	 * If a foreign key with an ON DELETE SET NULL/DEFAULT action, the subset
	 * of conkey to updated.  If null, all columns are updated.
	 */
	int16		confdelsetcols[1];

	/*
	 * If an exclusion constraint, the OIDs of the exclusion operators for
	 * each column of the constraint.  Also set for unique constraints/primary
	 * keys using WITHOUT OVERLAPS.
	 */
	Oid			conexclop[1] BKI_LOOKUP(pg_operator);

	/*
	 * If a check constraint, nodeToString representation of expression
	 */
	pg_node_tree conbin;
#endif
} FormData_pg_constraint;

END_CATALOG_STRUCT
```

Source excerpt: `src/include/catalog/pg_constraint.h:179`

```c
/* ----------------
 *		Form_pg_constraint corresponds to a pointer to a tuple with
 *		the format of pg_constraint relation.
 * ----------------
 */
typedef FormData_pg_constraint *Form_pg_constraint;
```

### `Form_pg_index`

C kind: struct typedef

Definition path: `src/include/catalog/pg_index.h:31`

Source excerpt: `src/include/catalog/pg_index.h:29`

```c
/* ----------------
 *		pg_index definition.  cpp turns this into
 *		typedef struct FormData_pg_index.
 * ----------------
 */
BEGIN_CATALOG_STRUCT

CATALOG(pg_index,2610,IndexRelationId) BKI_SCHEMA_MACRO
{
	Oid			indexrelid BKI_LOOKUP(pg_class);	/* OID of the index */
	Oid			indrelid BKI_LOOKUP(pg_class);	/* OID of the relation it
												 * indexes */
	int16		indnatts;		/* total number of columns in index */
	int16		indnkeyatts;	/* number of key columns in index */
	bool		indisunique;	/* is this a unique index? */
	bool		indnullsnotdistinct;	/* null treatment in unique index */
	bool		indisprimary;	/* is this index for primary key? */
	bool		indisexclusion; /* is this index for exclusion constraint? */
	bool		indimmediate;	/* is uniqueness enforced immediately? */
	bool		indisclustered; /* is this the index last clustered by? */
	bool		indisvalid;		/* is this index valid for use by queries? */
	bool		indcheckxmin;	/* must we wait for xmin to be old? */
	bool		indisready;		/* is this index ready for inserts? */
	bool		indislive;		/* is this index alive at all? */
	bool		indisreplident; /* is this index the identity for replication? */

	/* variable-length fields start here, but we allow direct access to indkey */
	int2vector	indkey BKI_FORCE_NOT_NULL;	/* column numbers of indexed cols,
											 * or 0 */

#ifdef CATALOG_VARLEN
	oidvector	indcollation BKI_LOOKUP_OPT(pg_collation) BKI_FORCE_NOT_NULL;	/* collation identifiers */
	oidvector	indclass BKI_LOOKUP(pg_opclass) BKI_FORCE_NOT_NULL; /* opclass identifiers */
	int2vector	indoption BKI_FORCE_NOT_NULL;	/* per-column flags
												 * (AM-specific meanings) */
	pg_node_tree indexprs;		/* expression trees for index attributes that
								 * are not simple column references; one for
								 * each zero entry in indkey[] */
	pg_node_tree indpred;		/* expression tree for predicate, if a partial
								 * index; else NULL */
#endif
} FormData_pg_index;

END_CATALOG_STRUCT
```

Source excerpt: `src/include/catalog/pg_index.h:74`

```c
/* ----------------
 *		Form_pg_index corresponds to a pointer to a tuple with
 *		the format of pg_index relation.
 * ----------------
 */
typedef FormData_pg_index *Form_pg_index;
```

### `FromExpr`

C kind: struct typedef

Definition path: `src/include/nodes/primnodes.h:2400`

```c
/*----------
 * FromExpr - represents a FROM ... WHERE ... construct
 *
 * This is both more flexible than a JoinExpr (it can have any number of
 * children, including zero) and less so --- we don't need to deal with
 * aliases and so on.  The output column set is implicitly just the union
 * of the outputs of the children.
 *----------
 */
typedef struct FromExpr
{
	NodeTag		type;
	List	   *fromlist;		/* List of join subtrees */
	Node	   *quals;			/* qualifiers on join, if any */
} FromExpr;
```

### `FuncExpr`

C kind: struct typedef

Definition path: `src/include/nodes/primnodes.h:785`

```c
/*
 * FuncExpr - expression node for a function call
 *
 * Collation information is irrelevant for the query jumbling, only the
 * arguments and the function OID matter.
 */
typedef struct FuncExpr
{
	Expr		xpr;
	/* PG_PROC OID of the function */
	Oid			funcid;
	/* PG_TYPE OID of result value */
	Oid			funcresulttype pg_node_attr(query_jumble_ignore);
	/* true if function returns set */
	bool		funcretset pg_node_attr(query_jumble_ignore);

	/*
	 * true if variadic arguments have been combined into an array last
	 * argument
	 */
	bool		funcvariadic pg_node_attr(query_jumble_ignore);
	/* how to display this function call */
	CoercionForm funcformat pg_node_attr(query_jumble_ignore);
	/* OID of collation of result */
	Oid			funccollid pg_node_attr(query_jumble_ignore);
	/* OID of collation that function should use */
	Oid			inputcollid pg_node_attr(query_jumble_ignore);
	/* arguments to the function */
	List	   *args;
	/* token location, or -1 if unknown */
	ParseLoc	location;
} FuncExpr;
```

### `HeapTuple`

C kind: struct typedef

Definition path: `src/include/access/htup.h:62`

```c
/*
 * HeapTupleData is an in-memory data structure that points to a tuple.
 *
 * There are several ways in which this data structure is used:
 *
 * * Pointer to a tuple in a disk buffer: t_data points directly into the
 *	 buffer (which the code had better be holding a pin on, but this is not
 *	 reflected in HeapTupleData itself).
 *
 * * Pointer to nothing: t_data is NULL.  This is used as a failure indication
 *	 in some functions.
 *
 * * Part of a palloc'd tuple: the HeapTupleData itself and the tuple
 *	 form a single palloc'd chunk.  t_data points to the memory location
 *	 immediately following the HeapTupleData struct (at offset HEAPTUPLESIZE).
 *	 This is the output format of heap_form_tuple and related routines.
 *
 * * Separately allocated tuple: t_data points to a palloc'd chunk that
 *	 is not adjacent to the HeapTupleData.  (This case is deprecated since
 *	 it's difficult to tell apart from case #1.  It should be used only in
 *	 limited contexts where the code knows that case #1 will never apply.)
 *
 * * Separately allocated minimal tuple: t_data points MINIMAL_TUPLE_OFFSET
 *	 bytes before the start of a MinimalTuple.  As with the previous case,
 *	 this can't be told apart from case #1 by inspection; code setting up
 *	 or destroying this representation has to know what it's doing.
 *
 * t_len should always be valid, except in the pointer-to-nothing case.
 * t_self and t_tableOid should be valid if the HeapTupleData points to
 * a disk buffer, or if it represents a copy of a tuple on disk.  They
 * should be explicitly set invalid in manufactured tuples.
 */
typedef struct HeapTupleData
{
	uint32		t_len;			/* length of *t_data */
	ItemPointerData t_self;		/* SelfItemPointer */
	Oid			t_tableOid;		/* table the tuple came from */
#define FIELDNO_HEAPTUPLEDATA_DATA 3
	HeapTupleHeader t_data;		/* -> tuple header and data */
} HeapTupleData;

typedef HeapTupleData *HeapTuple;
```

### `JoinExpr`

C kind: struct typedef

Definition path: `src/include/nodes/primnodes.h:2368`

```c
/*----------
 * JoinExpr - for SQL JOIN expressions
 *
 * isNatural, usingClause, keyJoin, and quals are interdependent.  The user can
 * write only one of NATURAL, USING(), FOR KEY, or ON() (this is enforced by
 * the grammar).
 * If he writes NATURAL then parse analysis generates the equivalent USING()
 * list, and from that fills in "quals" with the right equality comparisons.
 * If he writes USING() then "quals" is filled with equality comparisons.
 * If he writes FOR KEY then "keyJoin" is set initially, and parse analysis
 * replaces it while filling "quals" with equality comparisons.
 * If he writes ON() then only "quals" is set.  Note that NATURAL/USING
 * are not equivalent to ON() since they also affect the output column list.
 *
 * alias is an Alias node representing the AS alias-clause attached to the
 * join expression, or NULL if no clause.  NB: presence or absence of the
 * alias has a critical impact on semantics, because a join with an alias
 * restricts visibility of the tables/columns inside it.
 *
 * join_using_alias is an Alias node representing the join correlation
 * name that SQL:2016 and later allow to be attached to JOIN/USING.
 * Its column alias list includes only the common column names from USING,
 * and it does not restrict visibility of the join's input tables.
 *
 * During parse analysis, an RTE is created for the Join, and its index
 * is filled into rtindex.  This RTE is present mainly so that Vars can
 * be created that refer to the outputs of the join.  The planner sometimes
 * generates JoinExprs internally; these can have rtindex = 0 if there are
 * no join alias variables referencing such joins.
 *----------
 */
typedef struct JoinExpr
{
	NodeTag		type;
	JoinType	jointype;		/* type of join */
	bool		isNatural;		/* Natural join? Will need to shape table */
	Node	   *larg;			/* left subtree */
	Node	   *rarg;			/* right subtree */
	/* USING clause, if any (list of String) */
	List	   *usingClause pg_node_attr(query_jumble_ignore);
	/* alias attached to USING clause, if any */
	Alias	   *join_using_alias pg_node_attr(query_jumble_ignore);
	/* KEY clause, if any */
	Node	   *keyJoin;
	/* FILTER clause (transformed); for deparsing only, actual expr is in quals */
	Node	   *joinFilter pg_node_attr(query_jumble_ignore);
	/* qualifiers on join, if any */
	Node	   *quals;
	/* user-written alias clause, if any */
	Alias	   *alias pg_node_attr(query_jumble_ignore);
	/* RT index assigned for join, or 0 */
	int			rtindex;
} JoinExpr;
```

### `KeyJoinClause`

C kind: struct typedef

Definition path: `src/include/nodes/parsenodes.h:4673`

```c
typedef struct KeyJoinClause
{
	NodeTag		type;
	List	   *localCols;
	KeyJoinDirection direction;
	char	   *refAlias;
	List	   *refCols;
	Node	   *filter;			/* raw FILTER (WHERE ...) expr, or NULL */
	ParseLoc	location;		/* token location, or -1 if unknown */
} KeyJoinClause;
```

### `KeyJoinFact`

C kind: struct typedef

Definition path: `src/include/nodes/parsenodes.h:1176`

```c
typedef struct KeyJoinFact
{
	NodeTag		type;
	KeyJoinFactKind kind;
	/* KJF_NOT_NULL: surface column attnum */
	AttrNumber	attnum;
	/* KJF_UNIQUE / KJF_FOREIGN_KEY / KJF_ROW_COVERAGE: */
	List	   *keyPositions;	/* of KeyJoinKeyPosition */
	Oid			relid;			/* base relation OID, or InvalidOid */
	List	   *baseAttnums;	/* per-position base attnum */
	/* KJF_FOREIGN_KEY only: */
	Oid			referencedRelid;
	List	   *referencedAttnums;
	Oid			constraint;		/* pg_constraint OID */
	/* KJF_FOREIGN_KEY and KJF_ROW_COVERAGE: */
	/* transient canonical filters; may contain parser-private Params */
	List	   *filterConjuncts;
	/* All kinds: */
	List	   *dependencies;	/* of KeyJoinProofDependency */
} KeyJoinFact;
```

### `KeyJoinKeyPosition`

C kind: struct typedef

Definition path: `src/include/nodes/parsenodes.h:1145`

```c
typedef struct KeyJoinKeyPosition
{
	NodeTag		type;
	List	   *attnums;
	/* strict exposed proof identity */
	Oid			typeOid;
	int32		typmod;
	Oid			collationOid;
	/* transient equality operator input identity, base type for domains */
	Oid			eqTypeOid;
	int32		eqTypmod;
	Oid			eqOperator;
} KeyJoinKeyPosition;
```

### `KeyJoinNode`

C kind: struct typedef

Definition path: `src/include/nodes/parsenodes.h:4684`

```c
typedef struct KeyJoinNode
{
	NodeTag		type;
	KeyJoinDirection direction;
	Index		referencingVarno;
	Index		referencedVarno;
	List	   *referencingAttnums;
	List	   *referencedAttnums;
	Index		refAliasVarno;
	List	   *refAliasAttnums;
	Oid			constraint;
	List	   *notNullConstraints;
	List	   *proofDependencies;
} KeyJoinNode;
```

### `KeyJoinProofDependency`

C kind: struct typedef

Definition path: `src/include/nodes/parsenodes.h:1137`

```c
typedef struct KeyJoinProofDependency
{
	NodeTag		type;
	Oid			classId;
	Oid			objectId;
	int32		objectSubId;
} KeyJoinProofDependency;
```

### `KeyJoinSurfaceFacts`

C kind: struct typedef

Definition path: `src/include/nodes/parsenodes.h:1197`

```c
typedef struct KeyJoinSurfaceFacts
{
	NodeTag		type;
	/* transient parser proof facts, never read from stored query trees */
	List	   *facts;			/* of KeyJoinFact */
} KeyJoinSurfaceFacts;
```

### `List`

C kind: struct typedef

Definition path: `src/include/nodes/pg_list.h:53`

```c
typedef struct List
{
	NodeTag		type;			/* T_List, T_IntList, T_OidList, or T_XidList */
	int			length;			/* number of elements currently present */
	int			max_length;		/* allocated length of elements[] */
	ListCell   *elements;		/* re-allocatable array of cells */
	/* We may allocate some cells along with the List header: */
	ListCell	initial_elements[FLEXIBLE_ARRAY_MEMBER];
	/* If elements == initial_elements, it's not a separate allocation */
} List;
```

### `ListCell`

C kind: union typedef

Definition path: `src/include/nodes/pg_list.h:45`

```c
typedef union ListCell
{
	void	   *ptr_value;
	int			int_value;
	Oid			oid_value;
	TransactionId xid_value;
} ListCell;
```

### `Node`

C kind: struct typedef

Definition path: `src/include/nodes/nodes.h:134`

```c
/*
 * The first field of a node of any type is guaranteed to be the NodeTag.
 * Hence the type of any node can be gotten by casting it to Node. Declaring
 * a variable to be of Node * (instead of void *) can also facilitate
 * debugging.
 */
typedef struct Node
{
	NodeTag		type;
} Node;
```

### `OpExpr`

C kind: struct typedef

Definition path: `src/include/nodes/primnodes.h:852`

```c
/*
 * OpExpr - expression node for an operator invocation
 *
 * Semantically, this is essentially the same as a function call.
 *
 * Note that opfuncid is not necessarily filled in immediately on creation
 * of the node.  The planner makes sure it is valid before passing the node
 * tree to the executor, but during parsing/planning opfuncid can be 0.
 * Therefore, equal() will accept a zero value as being equal to other values.
 *
 * Internal state information and collation data is irrelevant for the query
 * jumbling.
 */
typedef struct OpExpr
{
	Expr		xpr;

	/* PG_OPERATOR OID of the operator */
	Oid			opno;

	/* PG_PROC OID of underlying function */
	Oid			opfuncid pg_node_attr(equal_ignore_if_zero, query_jumble_ignore);

	/* PG_TYPE OID of result value */
	Oid			opresulttype pg_node_attr(query_jumble_ignore);

	/* true if operator returns set */
	bool		opretset pg_node_attr(query_jumble_ignore);

	/* OID of collation of result */
	Oid			opcollid pg_node_attr(query_jumble_ignore);

	/* OID of collation that operator should use */
	Oid			inputcollid pg_node_attr(query_jumble_ignore);

	/* arguments to the operator (1 or 2) */
	List	   *args;

	/* token location, or -1 if unknown */
	ParseLoc	location;
} OpExpr;
```

### `Param`

C kind: struct typedef

Definition path: `src/include/nodes/primnodes.h:397`

```c
typedef struct Param
{
	pg_node_attr(custom_query_jumble)

	Expr		xpr;
	ParamKind	paramkind;		/* kind of parameter. See above */
	int			paramid;		/* numeric ID for parameter */
	Oid			paramtype;		/* pg_type OID of parameter's datatype */
	/* typmod value, if known */
	int32		paramtypmod;
	/* OID of collation, or InvalidOid if none */
	Oid			paramcollid;
	/* token location, or -1 if unknown */
	ParseLoc	location;
} Param;
```

### `ParseNamespaceColumn`

C kind: struct typedef

Definition path: `src/include/parser/parse_node.h:348`

```c
/*
 * Data about one column of a ParseNamespaceItem.
 *
 * We track the info needed to construct a Var referencing the column
 * (but only for user-defined columns; system column references and
 * whole-row references are handled separately).
 *
 * p_varno and p_varattno identify the semantic referent, which is a
 * base-relation column unless the reference is to a join USING column that
 * isn't semantically equivalent to either join input column (because it is a
 * FULL join or the input column requires a type coercion).  In those cases
 * p_varno and p_varattno refer to the JOIN RTE.
 *
 * p_varnosyn and p_varattnosyn are either identical to p_varno/p_varattno,
 * or they specify the column's position in an aliased JOIN RTE that hides
 * the semantic referent RTE's refname.  (That could be either the JOIN RTE
 * in which this ParseNamespaceColumn entry exists, or some lower join level.)
 *
 * If an RTE contains a dropped column, its ParseNamespaceColumn struct
 * is all-zeroes.  (Conventionally, test for p_varno == 0 to detect this.)
 */
struct ParseNamespaceColumn
{
	Index		p_varno;		/* rangetable index */
	AttrNumber	p_varattno;		/* attribute number of the column */
	Oid			p_vartype;		/* pg_type OID */
	int32		p_vartypmod;	/* type modifier value */
	Oid			p_varcollid;	/* OID of collation, or InvalidOid */
	VarReturningType p_varreturningtype;	/* for RETURNING OLD/NEW */
	Index		p_varnosyn;		/* rangetable index of syntactic referent */
	AttrNumber	p_varattnosyn;	/* attribute number of syntactic referent */
	bool		p_dontexpand;	/* not included in star expansion */
};
```

### `ParseNamespaceItem`

C kind: struct typedef

Definition path: `src/include/parser/parse_node.h:312`

```c
/*
 * An element of a namespace list.
 *
 * p_names contains the table name and column names exposed by this nsitem.
 * (Typically it's equal to p_rte->eref, but for a JOIN USING alias it's
 * equal to p_rte->join_using_alias.  Since the USING columns will be the
 * join's first N columns, the net effect is just that we expose only those
 * join columns via this nsitem.)
 *
 * p_rte and p_rtindex link to the underlying rangetable entry, and
 * p_perminfo to the entry in rteperminfos.
 *
 * The p_nscolumns array contains info showing how to construct Vars
 * referencing the names appearing in the p_names->colnames list.
 *
 * Namespace items with p_rel_visible set define which RTEs are accessible by
 * qualified names, while those with p_cols_visible set define which RTEs are
 * accessible by unqualified names.  These sets are different because a JOIN
 * without an alias does not hide the contained tables (so they must be
 * visible for qualified references) but it does hide their columns
 * (unqualified references to the columns refer to the JOIN, not the member
 * tables, so we must not complain that such a reference is ambiguous).
 * Conversely, a subquery without an alias does not hide the columns selected
 * by the subquery, but it does hide the auto-generated relation name (so the
 * subquery columns are visible for unqualified references only).  Various
 * special RTEs such as NEW/OLD for rules may also appear with only one flag
 * set.
 *
 * While processing the FROM clause, namespace items may appear with
 * p_lateral_only set, meaning they are visible only to LATERAL
 * subexpressions.  (The pstate's p_lateral_active flag tells whether we are
 * inside such a subexpression at the moment.)	If p_lateral_ok is not set,
 * it's an error to actually use such a namespace item.  One might think it
 * would be better to just exclude such items from visibility, but the wording
 * of SQL:2008 requires us to do it this way.  We also use p_lateral_ok to
 * forbid LATERAL references to an UPDATE/DELETE target table.
 *
 * While processing the RETURNING clause, special namespace items are added to
 * refer to the OLD and NEW state of the result relation.  These namespace
 * items have p_returning_type set appropriately, for use when creating Vars.
 * For convenience, this information is duplicated on each namespace column.
 *
 * At no time should a namespace list contain two entries that conflict
 * according to the rules in checkNameSpaceConflicts; but note that those
 * are more complicated than "must have different alias names", so in practice
 * code searching a namespace list has to check for ambiguous references.
 */
struct ParseNamespaceItem
{
	Alias	   *p_names;		/* Table and column names */
	RangeTblEntry *p_rte;		/* The relation's rangetable entry */
	int			p_rtindex;		/* The relation's index in the rangetable */
	RTEPermissionInfo *p_perminfo;	/* The relation's rteperminfos entry */
	/* array of same length as p_names->colnames: */
	ParseNamespaceColumn *p_nscolumns;	/* per-column data */
	bool		p_rel_visible;	/* Relation name is visible? */
	bool		p_cols_visible; /* Column names visible as unqualified refs? */
	bool		p_lateral_only; /* Is only visible to LATERAL expressions? */
	bool		p_lateral_ok;	/* If so, does join type allow use? */
	VarReturningType p_returning_type;	/* Is OLD/NEW for use in RETURNING? */
};
```

### `ParseState`

C kind: struct typedef

Definition path: `src/include/parser/parse_node.h:211`

```c
/*
 * State information used during parse analysis
 *
 * parentParseState: NULL in a top-level ParseState.  When parsing a subquery,
 * links to current parse state of outer query.
 *
 * p_sourcetext: source string that generated the raw parsetree being
 * analyzed, or NULL if not available.  (The string is used only to
 * generate cursor positions in error messages: we need it to convert
 * byte-wise locations in parse structures to character-wise cursor
 * positions.)
 *
 * p_rtable: list of RTEs that will become the rangetable of the query.
 * Note that neither relname nor refname of these entries are necessarily
 * unique; searching the rtable by name is a bad idea.
 *
 * p_rteperminfos: list of RTEPermissionInfo containing an entry corresponding
 * to each RTE_RELATION entry in p_rtable.
 *
 * p_joinexprs: list of JoinExpr nodes associated with p_rtable entries.
 * This is one-for-one with p_rtable, but contains NULLs for non-join
 * RTEs, and may be shorter than p_rtable if the last RTE(s) aren't joins.
 *
 * p_nullingrels: list of Bitmapsets associated with p_rtable entries, each
 * containing the set of outer-join RTE indexes that can null that relation
 * at the current point in the parse tree.  This is one-for-one with p_rtable,
 * but may be shorter than p_rtable, in which case the missing entries are
 * implicitly empty (NULL).  That rule allows us to save work when the query
 * contains no outer joins.
 *
 * p_joinlist: list of join items (RangeTblRef and JoinExpr nodes) that
 * will become the fromlist of the query's top-level FromExpr node.
 *
 * p_namespace: list of ParseNamespaceItems that represents the current
 * namespace for table and column lookup.  (The RTEs listed here may be just
 * a subset of the whole rtable.  See ParseNamespaceItem comments below.)
 *
 * p_lateral_active: true if we are currently parsing a LATERAL subexpression
 * of this parse level.  This makes p_lateral_only namespace items visible,
 * whereas they are not visible when p_lateral_active is FALSE.
 *
 * p_ctenamespace: list of CommonTableExprs (WITH items) that are visible
 * at the moment.  This is entirely different from p_namespace because a CTE
 * is not an RTE, rather "visibility" means you could make an RTE from it.
 *
 * p_future_ctes: list of CommonTableExprs (WITH items) that are not yet
 * visible due to scope rules.  This is used to help improve error messages.
 *
 * p_parent_cte: CommonTableExpr that immediately contains the current query,
 * if any.
 *
 * p_target_relation: target relation, if query is INSERT/UPDATE/DELETE/MERGE
 *
 * p_target_nsitem: target relation's ParseNamespaceItem.
 *
 * p_grouping_nsitem: the ParseNamespaceItem that represents the grouping step.
 *
 * p_windowdefs: list of WindowDefs representing WINDOW and OVER clauses.
 * We collect these while transforming expressions and then transform them
 * afterwards (so that any resjunk tlist items needed for the sort/group
 * clauses end up at the end of the query tlist).  A WindowDef's location in
 * this list, counting from 1, is the winref number to use to reference it.
 *
 * p_expr_kind: kind of expression we're currently parsing, as per enum above;
 * EXPR_KIND_NONE when not in an expression.
 *
 * p_next_resno: next TargetEntry.resno to assign, starting from 1.
 *
 * p_multiassign_exprs: partially-processed MultiAssignRef source expressions.
 *
 * p_locking_clause: query's FOR UPDATE/FOR SHARE clause, if any.
 *
 * p_locked_from_parent: true if parent query level applies FOR UPDATE/SHARE
 * to this subquery as a whole.
 *
 * p_resolve_unknowns: resolve unknown-type SELECT output columns as type TEXT
 * (this is true by default).
 *
 * p_graph_table_pstate: Namespace for the GRAPH_TABLE reference being
 * transformed, if any.
 *
 * p_hasAggs, p_hasWindowFuncs, etc: true if we've found any of the indicated
 * constructs in the query.
 *
 * p_last_srf: the set-returning FuncExpr or OpExpr most recently found in
 * the query, or NULL if none.
 *
 * p_pre_columnref_hook, etc: optional parser hook functions for modifying the
 * interpretation of ColumnRefs and ParamRefs.
 *
 * p_ref_hook_state: passthrough state for the parser hook functions.
 */
struct ParseState
{
	ParseState *parentParseState;	/* stack link */
	const char *p_sourcetext;	/* source text, or NULL if not available */
	List	   *p_rtable;		/* range table so far */
	List	   *p_rteperminfos; /* list of RTEPermissionInfo nodes for each
								 * RTE_RELATION entry in rtable */
	List	   *p_joinexprs;	/* JoinExprs for RTE_JOIN p_rtable entries */
	List	   *p_nullingrels;	/* Bitmapsets showing nulling outer joins */
	List	   *p_joinlist;		/* join items so far (will become FromExpr
								 * node's fromlist) */
	List	   *p_namespace;	/* currently-referenceable RTEs (List of
								 * ParseNamespaceItem) */
	bool		p_lateral_active;	/* p_lateral_only items visible? */
	List	   *p_ctenamespace; /* current namespace for common table exprs */
	List	   *p_future_ctes;	/* common table exprs not yet in namespace */
	CommonTableExpr *p_parent_cte;	/* this query's containing CTE */
	Relation	p_target_relation;	/* INSERT/UPDATE/DELETE/MERGE target rel */
	ParseNamespaceItem *p_target_nsitem;	/* target rel's NSItem, or NULL */
	ParseNamespaceItem *p_grouping_nsitem;	/* NSItem for grouping, or NULL */
	List	   *p_windowdefs;	/* raw representations of window clauses */
	ParseExprKind p_expr_kind;	/* what kind of expression we're parsing */
	int			p_next_resno;	/* next targetlist resno to assign */
	List	   *p_multiassign_exprs;	/* junk tlist entries for multiassign */
	List	   *p_locking_clause;	/* raw FOR UPDATE/FOR SHARE info */
	bool		p_locked_from_parent;	/* parent has marked this subquery
										 * with FOR UPDATE/FOR SHARE */
	bool		p_resolve_unknowns; /* resolve unknown-type SELECT outputs as
									 * type text */

	QueryEnvironment *p_queryEnv;	/* curr env, incl refs to enclosing env */
	GraphTableParseState *p_graph_table_pstate; /* Current graph table
												 * namespace, if any */

	/* Flags telling about things found in the query: */
	bool		p_hasAggs;
	bool		p_hasWindowFuncs;
	bool		p_hasTargetSRFs;
	bool		p_hasSubLinks;
	bool		p_hasModifyingCTE;

	Node	   *p_last_srf;		/* most recent set-returning func/op found */

	/*
	 * Optional hook functions for parser callbacks.  These are null unless
	 * set up by the caller of make_parsestate.
	 */
	PreParseColumnRefHook p_pre_columnref_hook;
	PostParseColumnRefHook p_post_columnref_hook;
	ParseParamRefHook p_paramref_hook;
	CoerceParamHook p_coerce_param_hook;
	void	   *p_ref_hook_state;	/* common passthrough link for above */
};
```

### `Query`

C kind: struct typedef

Definition path: `src/include/nodes/parsenodes.h:117`

```c
/*
 * Query -
 *	  Parse analysis turns all statements into a Query tree
 *	  for further processing by the rewriter and planner.
 *
 *	  Utility statements (i.e. non-optimizable statements) have the
 *	  utilityStmt field set, and the rest of the Query is mostly dummy.
 *
 *	  Planning converts a Query tree into a Plan tree headed by a PlannedStmt
 *	  node --- the Query structure is not used by the executor.
 *
 *	  All the fields ignored for the query jumbling are not semantically
 *	  significant (such as alias names), as is ignored anything that can
 *	  be deduced from child nodes (else we'd just be double-hashing that
 *	  piece of information).
 */
typedef struct Query
{
	NodeTag		type;

	CmdType		commandType;	/* select|insert|update|delete|merge|utility */

	/* where did I come from? */
	QuerySource querySource pg_node_attr(query_jumble_ignore);

	/*
	 * query identifier (can be set by plugins); ignored for equal, as it
	 * might not be set; also not stored.  This is the result of the query
	 * jumble, hence ignored.
	 *
	 * We store this as a signed value as this is the form it's displayed to
	 * users in places such as EXPLAIN and pg_stat_statements.  Primarily this
	 * is done due to lack of an SQL type to represent the full range of
	 * uint64.
	 */
	int64		queryId pg_node_attr(equal_ignore, query_jumble_ignore, read_write_ignore, read_as(0));

	/* do I set the command result tag? */
	bool		canSetTag pg_node_attr(query_jumble_ignore);

	Node	   *utilityStmt;	/* non-null if commandType == CMD_UTILITY */

	/*
	 * rtable index of target relation for INSERT/UPDATE/DELETE/MERGE; 0 for
	 * SELECT.  This is ignored in the query jumble as unrelated to the
	 * compilation of the query ID.
	 */
	int			resultRelation pg_node_attr(query_jumble_ignore);

	/* FOR PORTION OF clause for UPDATE/DELETE */
	ForPortionOfExpr *forPortionOf;

	/* has aggregates in tlist or havingQual */
	bool		hasAggs pg_node_attr(query_jumble_ignore);
	/* has window functions in tlist */
	bool		hasWindowFuncs pg_node_attr(query_jumble_ignore);
	/* has set-returning functions in tlist */
	bool		hasTargetSRFs pg_node_attr(query_jumble_ignore);
	/* has subquery SubLink */
	bool		hasSubLinks pg_node_attr(query_jumble_ignore);
	/* distinctClause is from DISTINCT ON */
	bool		hasDistinctOn pg_node_attr(query_jumble_ignore);
	/* WITH RECURSIVE was specified */
	bool		hasRecursive pg_node_attr(query_jumble_ignore);
	/* has INSERT/UPDATE/DELETE/MERGE in WITH */
	bool		hasModifyingCTE pg_node_attr(query_jumble_ignore);
	/* FOR [KEY] UPDATE/SHARE was specified */
	bool		hasForUpdate pg_node_attr(query_jumble_ignore);
	/* rewriter has applied some RLS policy */
	bool		hasRowSecurity pg_node_attr(query_jumble_ignore);
	/* parser has added an RTE_GROUP RTE */
	bool		hasGroupRTE pg_node_attr(query_jumble_ignore);
	/* is a RETURN statement */
	bool		isReturn pg_node_attr(query_jumble_ignore);

	List	   *cteList;		/* WITH list (of CommonTableExpr's) */

	List	   *rtable;			/* list of range table entries */

	/*
	 * list of RTEPermissionInfo nodes for the rtable entries having
	 * perminfoindex > 0
	 */
	List	   *rteperminfos pg_node_attr(query_jumble_ignore);
	FromExpr   *jointree;		/* table join tree (FROM and WHERE clauses);
								 * also USING clause for MERGE */

	List	   *mergeActionList;	/* list of actions for MERGE (only) */

	/*
	 * rtable index of target relation for MERGE to pull data. Initially, this
	 * is the same as resultRelation, but after query rewriting, if the target
	 * relation is a trigger-updatable view, this is the index of the expanded
	 * view subquery, whereas resultRelation is the index of the target view.
	 */
	int			mergeTargetRelation pg_node_attr(query_jumble_ignore);

	/* join condition between source and target for MERGE */
	Node	   *mergeJoinCondition;

	List	   *targetList;		/* target list (of TargetEntry) */

	/* OVERRIDING clause */
	OverridingKind override pg_node_attr(query_jumble_ignore);

	OnConflictExpr *onConflict; /* ON CONFLICT DO NOTHING/SELECT/UPDATE */

	/*
	 * The following three fields describe the contents of the RETURNING list
	 * for INSERT/UPDATE/DELETE/MERGE. returningOldAlias and returningNewAlias
	 * are the alias names for OLD and NEW, which may be user-supplied values,
	 * the defaults "old" and "new", or NULL (if the default "old"/"new" is
	 * already in use as the alias for some other relation).
	 */
	char	   *returningOldAlias pg_node_attr(query_jumble_ignore);
	char	   *returningNewAlias pg_node_attr(query_jumble_ignore);
	List	   *returningList;	/* return-values list (of TargetEntry) */

	List	   *groupClause;	/* a list of SortGroupClause's */
	bool		groupDistinct;	/* was GROUP BY DISTINCT used? */
	bool		groupByAll;		/* was GROUP BY ALL used? */

	List	   *groupingSets;	/* a list of GroupingSet's if present */

	Node	   *havingQual;		/* qualifications applied to groups */

	List	   *windowClause;	/* a list of WindowClause's */

	List	   *distinctClause; /* a list of SortGroupClause's */

	List	   *sortClause;		/* a list of SortGroupClause's */

	Node	   *limitOffset;	/* # of result tuples to skip (int8 expr) */
	Node	   *limitCount;		/* # of result tuples to return (int8 expr) */
	LimitOption limitOption;	/* limit type */

	List	   *rowMarks;		/* a list of RowMarkClause's */

	Node	   *setOperations;	/* set-operation tree if this is top level of
								 * a UNION/INTERSECT/EXCEPT query */

	/*
	 * A list of pg_constraint OIDs that the query depends on to be
	 * semantically valid
	 */
	List	   *constraintDeps pg_node_attr(query_jumble_ignore);

	/* a list of WithCheckOption's (added during rewrite) */
	List	   *withCheckOptions pg_node_attr(query_jumble_ignore);

	/*
	 * The following two fields identify the portion of the source text string
	 * containing this query.  They are typically only populated in top-level
	 * Queries, not in sub-queries.  When not set, they might both be zero, or
	 * both be -1 meaning "unknown".
	 */
	/* start location, or -1 if unknown */
	ParseLoc	stmt_location;
	/* length in bytes; 0 means "rest of string" */
	ParseLoc	stmt_len pg_node_attr(query_jumble_ignore);
} Query;
```

### `RangeTblEntry`

C kind: struct typedef

Definition path: `src/include/nodes/parsenodes.h:1204`

```c
typedef struct RangeTblEntry
{
	pg_node_attr(custom_read_write)

	NodeTag		type;

	/*
	 * Fields valid in all RTEs:
	 *
	 * put alias + eref first to make dump more legible
	 */
	/* user-written alias clause, if any */
	Alias	   *alias pg_node_attr(query_jumble_ignore);

	/*
	 * Expanded reference names.  This uses a custom query jumble function so
	 * that the table name is included in the computation, but not its list of
	 * columns.
	 */
	Alias	   *eref pg_node_attr(custom_query_jumble);

	RTEKind		rtekind;		/* see above */

	/*
	 * Fields valid for a plain relation RTE (else zero):
	 *
	 * inh is true for relation references that should be expanded to include
	 * inheritance children, if the rel has any.  In the parser, this will
	 * only be true for RTE_RELATION entries.  The planner also uses this
	 * field to mark RTE_SUBQUERY entries that contain UNION ALL queries that
	 * it has flattened into pulled-up subqueries (creating a structure much
	 * like the effects of inheritance).
	 *
	 * rellockmode is really LOCKMODE, but it's declared int to avoid having
	 * to include lock-related headers here.  It must be RowExclusiveLock if
	 * the RTE is an INSERT/UPDATE/DELETE/MERGE target, else RowShareLock if
	 * the RTE is a SELECT FOR UPDATE/FOR SHARE target, else AccessShareLock.
	 *
	 * Note: in some cases, rule expansion may result in RTEs that are marked
	 * with RowExclusiveLock even though they are not the target of the
	 * current query; this happens if a DO ALSO rule simply scans the original
	 * target table.  We leave such RTEs with their original lockmode so as to
	 * avoid getting an additional, lesser lock.
	 *
	 * perminfoindex is 1-based index of the RTEPermissionInfo belonging to
	 * this RTE in the containing struct's list of same; 0 if permissions need
	 * not be checked for this RTE.
	 *
	 * As a special case, relid, relkind, rellockmode, and perminfoindex can
	 * also be set (nonzero) in an RTE_SUBQUERY RTE.  This occurs when we
	 * convert an RTE_RELATION RTE naming a view into an RTE_SUBQUERY
	 * containing the view's query.  We still need to perform run-time locking
	 * and permission checks on the view, even though it's not directly used
	 * in the query anymore, and the most expedient way to do that is to
	 * retain these fields from the old state of the RTE.
	 *
	 * As a special case, RTE_NAMEDTUPLESTORE can also set relid to indicate
	 * that the tuple format of the tuplestore is the same as the referenced
	 * relation.  This allows plans referencing AFTER trigger transition
	 * tables to be invalidated if the underlying table is altered.
	 */
	/* OID of the relation */
	Oid			relid pg_node_attr(query_jumble_ignore);
	/* inheritance requested? */
	bool		inh;
	/* relation kind (see pg_class.relkind) */
	char		relkind pg_node_attr(query_jumble_ignore);
	/* lock level that query requires on the rel */
	int			rellockmode pg_node_attr(query_jumble_ignore);
	/* index of RTEPermissionInfo entry, or 0 */
	Index		perminfoindex pg_node_attr(query_jumble_ignore);
	/* sampling info, or NULL */
	struct TableSampleClause *tablesample;

	/*
	 * Fields valid for a subquery RTE (else NULL):
	 */
	/* the sub-query */
	Query	   *subquery;
	/* is from security_barrier view? */
	bool		security_barrier pg_node_attr(query_jumble_ignore);

	/*
	 * Fields valid for a join RTE (else NULL/zero):
	 *
	 * joinaliasvars is a list of (usually) Vars corresponding to the columns
	 * of the join result.  An alias Var referencing column K of the join
	 * result can be replaced by the K'th element of joinaliasvars --- but to
	 * simplify the task of reverse-listing aliases correctly, we do not do
	 * that until planning time.  In detail: an element of joinaliasvars can
	 * be a Var of one of the join's input relations, or such a Var with an
	 * implicit coercion to the join's output column type, or a COALESCE
	 * expression containing the two input column Vars (possibly coerced).
	 * Elements beyond the first joinmergedcols entries are always just Vars,
	 * and are never referenced from elsewhere in the query (that is, join
	 * alias Vars are generated only for merged columns).  We keep these
	 * entries only because they're needed in expandRTE() and similar code.
	 *
	 * Vars appearing within joinaliasvars are marked with varnullingrels sets
	 * that describe the nulling effects of this join and lower ones.  This is
	 * essential for FULL JOIN cases, because the COALESCE expression only
	 * describes the semantics correctly if its inputs have been nulled by the
	 * join.  For other cases, it allows expandRTE() to generate a valid
	 * representation of the join's output without consulting additional
	 * parser state.
	 *
	 * Within a Query loaded from a stored rule, it is possible for non-merged
	 * joinaliasvars items to be null pointers, which are placeholders for
	 * (necessarily unreferenced) columns dropped since the rule was made.
	 * Also, once planning begins, joinaliasvars items can be almost anything,
	 * as a result of subquery-flattening substitutions.
	 *
	 * joinleftcols is an integer list of physical column numbers of the left
	 * join input rel that are included in the join; likewise joinrighttcols
	 * for the right join input rel.  (Which rels those are can be determined
	 * from the associated JoinExpr.)  If the join is USING/NATURAL, then the
	 * first joinmergedcols entries in each list identify the merged columns.
	 * The merged columns come first in the join output, then remaining
	 * columns of the left input, then remaining columns of the right.
	 *
	 * Note that input columns could have been dropped after creation of a
	 * stored rule, if they are not referenced in the query (in particular,
	 * merged columns could not be dropped); this is not accounted for in
	 * joinleftcols/joinrighttcols.
	 */
	JoinType	jointype;
	/* number of merged (JOIN USING) columns */
	int			joinmergedcols pg_node_attr(query_jumble_ignore);
	/* list of alias-var expansions */
	List	   *joinaliasvars pg_node_attr(query_jumble_ignore);
	/* left-side input column numbers */
	List	   *joinleftcols pg_node_attr(query_jumble_ignore);
	/* right-side input column numbers */
	List	   *joinrightcols pg_node_attr(query_jumble_ignore);

	/*
	 * join_using_alias is an alias clause attached directly to JOIN/USING. It
	 * is different from the alias field (above) in that it does not hide the
	 * range variables of the tables being joined.
	 */
	Alias	   *join_using_alias pg_node_attr(query_jumble_ignore);

	/*
	 * Fields valid for a function RTE (else NIL/zero):
	 *
	 * When funcordinality is true, the eref->colnames list includes an alias
	 * for the ordinality column.  The ordinality column is otherwise
	 * implicit, and must be accounted for "by hand" in places such as
	 * expandRTE().
	 */
	/* list of RangeTblFunction nodes */
	List	   *functions;
	/* is this called WITH ORDINALITY? */
	bool		funcordinality;

	/*
	 * Fields valid for a TableFunc RTE (else NULL):
	 */
	TableFunc  *tablefunc;

	/*
	 * Fields valid for a graph table RTE (else NULL):
	 */
	GraphPattern *graph_pattern;
	List	   *graph_table_columns;

	/*
	 * Fields valid for a values RTE (else NIL):
	 */
	/* list of expression lists */
	List	   *values_lists;

	/*
	 * Fields valid for a CTE RTE (else NULL/zero):
	 */
	/* name of the WITH list item */
	char	   *ctename;
	/* number of query levels up */
	Index		ctelevelsup;
	/* is this a recursive self-reference? */
	bool		self_reference pg_node_attr(query_jumble_ignore);

	/*
	 * Fields valid for CTE, VALUES, ENR, and TableFunc RTEs (else NIL):
	 *
	 * We need these for CTE RTEs so that the types of self-referential
	 * columns are well-defined.  For VALUES RTEs, storing these explicitly
	 * saves having to re-determine the info by scanning the values_lists. For
	 * ENRs, we store the types explicitly here (we could get the information
	 * from the catalogs if 'relid' was supplied, but we'd still need these
	 * for TupleDesc-based ENRs, so we might as well always store the type
	 * info here).  For TableFuncs, these fields are redundant with data in
	 * the TableFunc node, but keeping them here allows some code sharing with
	 * the other cases.
	 *
	 * For ENRs only, we have to consider the possibility of dropped columns.
	 * A dropped column is included in these lists, but it will have zeroes in
	 * all three lists (as well as an empty-string entry in eref).  Testing
	 * for zero coltype is the standard way to detect a dropped column.
	 */
	/* OID list of column type OIDs */
	List	   *coltypes pg_node_attr(query_jumble_ignore);
	/* integer list of column typmods */
	List	   *coltypmods pg_node_attr(query_jumble_ignore);
	/* OID list of column collation OIDs */
	List	   *colcollations pg_node_attr(query_jumble_ignore);

	/*
	 * Fields valid for ENR RTEs (else NULL/zero):
	 */
	/* name of ephemeral named relation */
	char	   *enrname;
	/* estimated or actual from caller */
	Cardinality enrtuples pg_node_attr(query_jumble_ignore);

	/*
	 * Fields valid for a GROUP RTE (else NIL):
	 */
	/* list of grouping expressions */
	List	   *groupexprs;

	/*
	 * Fields valid in all RTEs:
	 */
	/* was LATERAL specified? */
	bool		lateral pg_node_attr(query_jumble_ignore);
	/* present in FROM clause? */
	bool		inFromCl pg_node_attr(query_jumble_ignore);
	/* security barrier quals to apply, if any */
	List	   *securityQuals pg_node_attr(query_jumble_ignore);
	/* transient cached proof facts for FOR KEY joins */
	bool		keyJoinFactsComputed pg_node_attr(equal_ignore, query_jumble_ignore, read_write_ignore, read_as(false));
	KeyJoinSurfaceFacts *keyJoinFacts pg_node_attr(equal_ignore, query_jumble_ignore, read_write_ignore, read_as(NULL));
} RangeTblEntry;
```

### `RangeTblFunction`

C kind: struct typedef

Definition path: `src/include/nodes/parsenodes.h:1503`

```c
/*
 * RangeTblFunction -
 *	  RangeTblEntry subsidiary data for one function in a FUNCTION RTE.
 *
 * If the function had a column definition list (required for an
 * otherwise-unspecified RECORD result), funccolnames lists the names given
 * in the definition list, funccoltypes lists their declared column types,
 * funccoltypmods lists their typmods, funccolcollations their collations.
 * Otherwise, those fields are NIL.
 *
 * Notice we don't attempt to store info about the results of functions
 * returning named composite types, because those can change from time to
 * time.  We do however remember how many columns we thought the type had
 * (including dropped columns!), so that we can successfully ignore any
 * columns added after the query was parsed.
 *
 * The query jumbling only needs to track the function expression.
 */
typedef struct RangeTblFunction
{
	NodeTag		type;

	Node	   *funcexpr;		/* expression tree for func call */
	/* number of columns it contributes to RTE */
	int			funccolcount pg_node_attr(query_jumble_ignore);
	/* These fields record the contents of a column definition list, if any: */
	/* column names (list of String) */
	List	   *funccolnames pg_node_attr(query_jumble_ignore);
	/* OID list of column type OIDs */
	List	   *funccoltypes pg_node_attr(query_jumble_ignore);
	/* integer list of column typmods */
	List	   *funccoltypmods pg_node_attr(query_jumble_ignore);
	/* OID list of column collation OIDs */
	List	   *funccolcollations pg_node_attr(query_jumble_ignore);

	/* This is set during planning for use by the executor: */
	/* PARAM_EXEC Param IDs affecting this func */
	Bitmapset  *funcparams pg_node_attr(query_jumble_ignore);
} RangeTblFunction;
```

### `RangeTblRef`

C kind: struct typedef

Definition path: `src/include/nodes/primnodes.h:2325`

```c
/*
 * RangeTblRef - reference to an entry in the query's rangetable
 *
 * We could use direct pointers to the RT entries and skip having these
 * nodes, but multiple pointers to the same node in a querytree cause
 * lots of headaches, so it seems better to store an index into the RT.
 */
typedef struct RangeTblRef
{
	NodeTag		type;
	int			rtindex;
} RangeTblRef;
```

### `RelabelType`

C kind: struct typedef

Definition path: `src/include/nodes/primnodes.h:1222`

```c
/* ----------------
 * RelabelType
 *
 * RelabelType represents a "dummy" type coercion between two binary-
 * compatible datatypes, such as reinterpreting the result of an OID
 * expression as an int4.  It is a no-op at runtime; we only need it
 * to provide a place to store the correct type to be attributed to
 * the expression result during type resolution.  (We can't get away
 * with just overwriting the type field of the input expression node,
 * so we need a separate node to show the coercion's result type.)
 * ----------------
 */
typedef struct RelabelType
{
	Expr		xpr;
	Expr	   *arg;			/* input expression */
	Oid			resulttype;		/* output type of coercion expression */
	/* output typmod (usually -1) */
	int32		resulttypmod pg_node_attr(query_jumble_ignore);
	/* OID of collation, or InvalidOid if none */
	Oid			resultcollid pg_node_attr(query_jumble_ignore);
	/* how to display this node */
	CoercionForm relabelformat pg_node_attr(query_jumble_ignore);
	ParseLoc	location;		/* token location, or -1 if unknown */
} RelabelType;
```

### `Relation`

C kind: struct typedef

Definition path: `src/include/utils/rel.h:55`

```c
/*
 * Here are the contents of a relation cache entry.
 */
typedef struct RelationData
{
	RelFileLocator rd_locator;	/* relation physical identifier */
	SMgrRelation rd_smgr;		/* cached file handle, or NULL */
	int			rd_refcnt;		/* reference count */
	ProcNumber	rd_backend;		/* owning backend's proc number, if temp rel */
	bool		rd_islocaltemp; /* rel is a temp rel of this session */
	bool		rd_isnailed;	/* rel is nailed in cache */
	bool		rd_isvalid;		/* relcache entry is valid */
	bool		rd_indexvalid;	/* is rd_indexlist valid? (also rd_pkindex and
								 * rd_replidindex) */
	bool		rd_statvalid;	/* is rd_statlist valid? */

	/*----------
	 * rd_createSubid is the ID of the highest subtransaction the rel has
	 * survived into or zero if the rel or its storage was created before the
	 * current top transaction.  (IndexStmt.oldNumber leads to the case of a new
	 * rel with an old rd_locator.)  rd_firstRelfilelocatorSubid is the ID of the
	 * highest subtransaction an rd_locator change has survived into or zero if
	 * rd_locator matches the value it had at the start of the current top
	 * transaction.  (Rolling back the subtransaction that
	 * rd_firstRelfilelocatorSubid denotes would restore rd_locator to the value it
	 * had at the start of the current top transaction.  Rolling back any
	 * lower subtransaction would not.)  Their accuracy is critical to
	 * RelationNeedsWAL().
	 *
	 * rd_newRelfilelocatorSubid is the ID of the highest subtransaction the
	 * most-recent relfilenumber change has survived into or zero if not changed
	 * in the current transaction (or we have forgotten changing it).  This
	 * field is accurate when non-zero, but it can be zero when a relation has
	 * multiple new relfilenumbers within a single transaction, with one of them
	 * occurring in a subsequently aborted subtransaction, e.g.
	 *		BEGIN;
	 *		TRUNCATE t;
	 *		SAVEPOINT save;
	 *		TRUNCATE t;
	 *		ROLLBACK TO save;
	 *		-- rd_newRelfilelocatorSubid is now forgotten
	 *
	 * If every rd_*Subid field is zero, they are read-only outside
	 * relcache.c.  Files that trigger rd_locator changes by updating
	 * pg_class.reltablespace and/or pg_class.relfilenode call
	 * RelationAssumeNewRelfilelocator() to update rd_*Subid.
	 *
	 * rd_droppedSubid is the ID of the highest subtransaction that a drop of
	 * the rel has survived into.  In entries visible outside relcache.c, this
	 * is always zero.
	 */
	SubTransactionId rd_createSubid;	/* rel was created in current xact */
	SubTransactionId rd_newRelfilelocatorSubid; /* highest subxact changing
												 * rd_locator to current value */
	SubTransactionId rd_firstRelfilelocatorSubid;	/* highest subxact
													 * changing rd_locator to
													 * any value */
	SubTransactionId rd_droppedSubid;	/* dropped with another Subid set */

	Form_pg_class rd_rel;		/* RELATION tuple */
	TupleDesc	rd_att;			/* tuple descriptor */
	Oid			rd_id;			/* relation's object id */
	LockInfoData rd_lockInfo;	/* lock mgr's info for locking relation */
	RuleLock   *rd_rules;		/* rewrite rules */
	MemoryContext rd_rulescxt;	/* private memory cxt for rd_rules, if any */
	TriggerDesc *trigdesc;		/* Trigger info, or NULL if rel has none */
	/* use "struct" here to avoid needing to include rowsecurity.h: */
	struct RowSecurityDesc *rd_rsdesc;	/* row security policies, or NULL */

	/* data managed by RelationGetFKeyList: */
	List	   *rd_fkeylist;	/* list of ForeignKeyCacheInfo (see below) */
	bool		rd_fkeyvalid;	/* true if list has been computed */

	/* data managed by RelationGetPartitionKey: */
	PartitionKey rd_partkey;	/* partition key, or NULL */
	MemoryContext rd_partkeycxt;	/* private context for rd_partkey, if any */

	/* data managed by RelationGetPartitionDesc: */
	PartitionDesc rd_partdesc;	/* partition descriptor, or NULL */
	MemoryContext rd_pdcxt;		/* private context for rd_partdesc, if any */

	/* Same as above, for partdescs that omit detached partitions */
	PartitionDesc rd_partdesc_nodetached;	/* partdesc w/o detached parts */
	MemoryContext rd_pddcxt;	/* for rd_partdesc_nodetached, if any */

	/*
	 * pg_inherits.xmin of the partition that was excluded in
	 * rd_partdesc_nodetached.  This informs a future user of that partdesc:
	 * if this value is not in progress for the active snapshot, then the
	 * partdesc can be used, otherwise they have to build a new one.  (This
	 * matches what find_inheritance_children_extended would do).
	 */
	TransactionId rd_partdesc_nodetached_xmin;

	/* data managed by RelationGetPartitionQual: */
	List	   *rd_partcheck;	/* partition CHECK quals */
	bool		rd_partcheckvalid;	/* true if list has been computed */
	MemoryContext rd_partcheckcxt;	/* private cxt for rd_partcheck, if any */

	/* data managed by RelationGetIndexList: */
	List	   *rd_indexlist;	/* list of OIDs of indexes on relation */
	Oid			rd_pkindex;		/* OID of (deferrable?) primary key, if any */
	bool		rd_ispkdeferrable;	/* is rd_pkindex a deferrable PK? */
	Oid			rd_replidindex; /* OID of replica identity index, if any */

	/* data managed by RelationGetStatExtList: */
	List	   *rd_statlist;	/* list of OIDs of extended stats */

	/* data managed by RelationGetIndexAttrBitmap: */
	bool		rd_attrsvalid;	/* are bitmaps of attrs valid? */
	Bitmapset  *rd_keyattr;		/* cols that can be ref'd by foreign keys */
	Bitmapset  *rd_pkattr;		/* cols included in primary key */
	Bitmapset  *rd_idattr;		/* included in replica identity index */
	Bitmapset  *rd_hotblockingattr; /* cols blocking HOT update */
	Bitmapset  *rd_summarizedattr;	/* cols indexed by summarizing indexes */

	PublicationDesc *rd_pubdesc;	/* publication descriptor, or NULL */

	/*
	 * rd_options is set whenever rd_rel is loaded into the relcache entry.
	 * Note that you can NOT look into rd_rel for this data.  NULL means "use
	 * defaults".
	 */
	bytea	   *rd_options;		/* parsed pg_class.reloptions */

	/*
	 * Oid of the handler for this relation. For an index this is a function
	 * returning IndexAmRoutine, for table like relations a function returning
	 * TableAmRoutine.  This is stored separately from rd_indam, rd_tableam as
	 * its lookup requires syscache access, but during relcache bootstrap we
	 * need to be able to initialize rd_tableam without syscache lookups.
	 */
	Oid			rd_amhandler;	/* OID of index AM's handler function */

	/*
	 * Table access method.
	 */
	const struct TableAmRoutine *rd_tableam;

	/* These are non-NULL only for an index relation: */
	Form_pg_index rd_index;		/* pg_index tuple describing this index */
	/* use "struct" here to avoid needing to include htup.h: */
	struct HeapTupleData *rd_indextuple;	/* all of pg_index tuple */

	/*
	 * index access support info (used only for an index relation)
	 *
	 * Note: only default support procs for each opclass are cached, namely
	 * those with lefttype and righttype equal to the opclass's opcintype. The
	 * arrays are indexed by support function number, which is a sufficient
	 * identifier given that restriction.
	 */
	MemoryContext rd_indexcxt;	/* private memory cxt for this stuff */
	/* use "struct" here to avoid needing to include amapi.h: */
	const struct IndexAmRoutine *rd_indam;	/* index AM's API struct */
	Oid		   *rd_opfamily;	/* OIDs of op families for each index col */
	Oid		   *rd_opcintype;	/* OIDs of opclass declared input data types */
	RegProcedure *rd_support;	/* OIDs of support procedures */
	struct FmgrInfo *rd_supportinfo;	/* lookup info for support procedures */
	int16	   *rd_indoption;	/* per-column AM-specific flags */
	List	   *rd_indexprs;	/* index expression trees, if any */
	List	   *rd_indpred;		/* index predicate tree, if any */
	Oid		   *rd_exclops;		/* OIDs of exclusion operators, if any */
	Oid		   *rd_exclprocs;	/* OIDs of exclusion ops' procs, if any */
	uint16	   *rd_exclstrats;	/* exclusion ops' strategy numbers, if any */
	Oid		   *rd_indcollation;	/* OIDs of index collations */
	bytea	  **rd_opcoptions;	/* parsed opclass-specific options */

	/*
	 * rd_amcache is available for index and table AMs to cache private data
	 * about the relation.  This must be just a cache since it may get reset
	 * at any time (in particular, it will get reset by a relcache inval
	 * message for the relation).  If used, it must point to a single memory
	 * chunk palloc'd in CacheMemoryContext, or in rd_indexcxt for an index
	 * relation.  A relcache reset will include freeing that chunk and setting
	 * rd_amcache = NULL.
	 */
	void	   *rd_amcache;		/* available for use by index/table AM */

	/*
	 * foreign-table support
	 *
	 * rd_fdwroutine must point to a single memory chunk palloc'd in
	 * CacheMemoryContext.  It will be freed and reset to NULL on a relcache
	 * reset.
	 */

	/* use "struct" here to avoid needing to include fdwapi.h: */
	struct FdwRoutine *rd_fdwroutine;	/* cached function pointers, or NULL */

	/*
	 * Hack for CLUSTER, rewriting ALTER TABLE, etc: when writing a new
	 * version of a table, we need to make any toast pointers inserted into it
	 * have the existing toast table's OID, not the OID of the transient toast
	 * table.  If rd_toastoid isn't InvalidOid, it is the OID to place in
	 * toast pointers inserted into this rel.  (Note it's set on the new
	 * version of the main heap, not the toast table itself.)  This also
	 * causes toast_save_datum() to try to preserve toast value OIDs.
	 */
	Oid			rd_toastoid;	/* Real TOAST table's OID, or InvalidOid */

	bool		pgstat_enabled; /* should relation stats be counted */
	/* use "struct" here to avoid needing to include pgstat.h: */
	struct PgStat_TableStatus *pgstat_info; /* statistics collection area */
} RelationData;
```

### `SQLValueFunction`

C kind: struct typedef

Definition path: `src/include/nodes/primnodes.h:1586`

```c
typedef struct SQLValueFunction
{
	Expr		xpr;
	SQLValueFunctionOp op;		/* which function this is */

	/*
	 * Result type/typmod.  Type is fully determined by "op", so no need to
	 * include this Oid in the query jumbling.
	 */
	Oid			type pg_node_attr(query_jumble_ignore);
	int32		typmod;
	ParseLoc	location;		/* token location, or -1 if unknown */
} SQLValueFunction;
```

### `SortGroupClause`

C kind: struct typedef

Definition path: `src/include/nodes/parsenodes.h:1623`

```c
/*
 * SortGroupClause -
 *		representation of ORDER BY, GROUP BY, PARTITION BY,
 *		DISTINCT, DISTINCT ON items
 *
 * You might think that ORDER BY is only interested in defining ordering,
 * and GROUP/DISTINCT are only interested in defining equality.  However,
 * one way to implement grouping is to sort and then apply a "uniq"-like
 * filter.  So it's also interesting to keep track of possible sort operators
 * for GROUP/DISTINCT, and in particular to try to sort for the grouping
 * in a way that will also yield a requested ORDER BY ordering.  So we need
 * to be able to compare ORDER BY and GROUP/DISTINCT lists, which motivates
 * the decision to give them the same representation.
 *
 * tleSortGroupRef must match ressortgroupref of exactly one entry of the
 *		query's targetlist; that is the expression to be sorted or grouped by.
 * eqop is the OID of the equality operator.
 * sortop is the OID of the ordering operator (a "<" or ">" operator),
 *		or InvalidOid if not available.
 * nulls_first means about what you'd expect.  If sortop is InvalidOid
 *		then nulls_first is meaningless and should be set to false.
 * hashable is true if eqop is hashable (note this condition also depends
 *		on the datatype of the input expression).
 *
 * In an ORDER BY item, all fields must be valid.  (The eqop isn't essential
 * here, but it's cheap to get it along with the sortop, and requiring it
 * to be valid eases comparisons to grouping items.)  Note that this isn't
 * actually enough information to determine an ordering: if the sortop is
 * collation-sensitive, a collation OID is needed too.  We don't store the
 * collation in SortGroupClause because it's not available at the time the
 * parser builds the SortGroupClause; instead, consult the exposed collation
 * of the referenced targetlist expression to find out what it is.
 *
 * In a grouping item, eqop must be valid.  If the eqop is a btree equality
 * operator, then sortop should be set to a compatible ordering operator.
 * We prefer to set eqop/sortop/nulls_first to match any ORDER BY item that
 * the query presents for the same tlist item.  If there is none, we just
 * use the default ordering op for the datatype.
 *
 * If the tlist item's type has a hash opclass but no btree opclass, then
 * we will set eqop to the hash equality operator, sortop to InvalidOid,
 * and nulls_first to false.  A grouping item of this kind can only be
 * implemented by hashing, and of course it'll never match an ORDER BY item.
 *
 * The hashable flag is provided since we generally have the requisite
 * information readily available when the SortGroupClause is constructed,
 * and it's relatively expensive to get it again later.  Note there is no
 * need for a "sortable" flag since OidIsValid(sortop) serves the purpose.
 *
 * A query might have both ORDER BY and DISTINCT (or DISTINCT ON) clauses.
 * In SELECT DISTINCT, the distinctClause list is as long or longer than the
 * sortClause list, while in SELECT DISTINCT ON it's typically shorter.
 * The two lists must match up to the end of the shorter one --- the parser
 * rearranges the distinctClause if necessary to make this true.  (This
 * restriction ensures that only one sort step is needed to both satisfy the
 * ORDER BY and set up for the Unique step.  This is semantically necessary
 * for DISTINCT ON, and presents no real drawback for DISTINCT.)
 */
typedef struct SortGroupClause
{
	NodeTag		type;
	Index		tleSortGroupRef;	/* reference into targetlist */
	Oid			eqop;			/* the equality operator ('=' op) */
	Oid			sortop;			/* the ordering operator ('<' op), or 0 */
	bool		reverse_sort;	/* is sortop a "greater than" operator? */
	bool		nulls_first;	/* do NULLs come before normal values? */
	/* can eqop be implemented by hashing? */
	bool		hashable pg_node_attr(query_jumble_ignore);
} SortGroupClause;
```

### `TargetEntry`

C kind: struct typedef

Definition path: `src/include/nodes/primnodes.h:2268`

```c
/*--------------------
 * TargetEntry -
 *	   a target entry (used in query target lists)
 *
 * Strictly speaking, a TargetEntry isn't an expression node (since it can't
 * be evaluated by ExecEvalExpr).  But we treat it as one anyway, since in
 * very many places it's convenient to process a whole query targetlist as a
 * single expression tree.
 *
 * In a SELECT's targetlist, resno should always be equal to the item's
 * ordinal position (counting from 1).  However, in an INSERT or UPDATE
 * targetlist, resno represents the attribute number of the destination
 * column for the item; so there may be missing or out-of-order resnos.
 * It is even legal to have duplicated resnos; consider
 *		UPDATE table SET arraycol[1] = ..., arraycol[2] = ..., ...
 * In an INSERT, the rewriter and planner will normalize the tlist by
 * reordering it into physical column order and filling in default values
 * for any columns not assigned values by the original query.  In an UPDATE,
 * after the rewriter merges multiple assignments for the same column, the
 * planner extracts the target-column numbers into a separate "update_colnos"
 * list, and then renumbers the tlist elements serially.  Thus, tlist resnos
 * match ordinal position in all tlists seen by the executor; but it is wrong
 * to assume that before planning has happened.
 *
 * resname is required to represent the correct column name in non-resjunk
 * entries of top-level SELECT targetlists, since it will be used as the
 * column title sent to the frontend.  In most other contexts it is only
 * a debugging aid, and may be wrong or even NULL.  (In particular, it may
 * be wrong in a tlist from a stored rule, if the referenced column has been
 * renamed by ALTER TABLE since the rule was made.  Also, the planner tends
 * to store NULL rather than look up a valid name for tlist entries in
 * non-toplevel plan nodes.)  In resjunk entries, resname should be either
 * a specific system-generated name (such as "ctid") or NULL; anything else
 * risks confusing ExecGetJunkAttribute!
 *
 * ressortgroupref is used in the representation of ORDER BY, GROUP BY, and
 * DISTINCT items.  Targetlist entries with ressortgroupref=0 are not
 * sort/group items.  If ressortgroupref>0, then this item is an ORDER BY,
 * GROUP BY, and/or DISTINCT target value.  No two entries in a targetlist
 * may have the same nonzero ressortgroupref --- but there is no particular
 * meaning to the nonzero values, except as tags.  (For example, one must
 * not assume that lower ressortgroupref means a more significant sort key.)
 * The order of the associated SortGroupClause lists determine the semantics.
 *
 * resorigtbl/resorigcol identify the source of the column, if it is a
 * simple reference to a column of a base table (or view).  If it is not
 * a simple reference, these fields are zeroes.
 *
 * If resjunk is true then the column is a working column (such as a sort key)
 * that should be removed from the final output of the query.  Resjunk columns
 * must have resnos that cannot duplicate any regular column's resno.  Also
 * note that there are places that assume resjunk columns come after non-junk
 * columns.
 *--------------------
 */
typedef struct TargetEntry
{
	Expr		xpr;
	/* expression to evaluate */
	Expr	   *expr;
	/* attribute number (see notes above) */
	AttrNumber	resno;
	/* name of the column (could be NULL) */
	char	   *resname pg_node_attr(query_jumble_ignore);
	/* nonzero if referenced by a sort/group clause */
	Index		ressortgroupref;
	/* OID of column's source table */
	Oid			resorigtbl pg_node_attr(query_jumble_ignore);
	/* column's number in source table */
	AttrNumber	resorigcol pg_node_attr(query_jumble_ignore);
	/* set to true to eliminate the attribute from final target list */
	bool		resjunk pg_node_attr(query_jumble_ignore);
} TargetEntry;
```

### `TupleDesc`

C kind: struct typedef

Definition path: `src/include/access/tupdesc.h:148`

```c
/*
 * This struct is passed around within the backend to describe the structure
 * of tuples.  For tuples coming from on-disk relations, the information is
 * collected from the pg_attribute, pg_attrdef, and pg_constraint catalogs.
 * Transient row types (such as the result of a join query) have anonymous
 * TupleDesc structs that generally omit any constraint info; therefore the
 * structure is designed to let the constraints be omitted efficiently.
 *
 * Note that only user attributes, not system attributes, are mentioned in
 * TupleDesc.
 *
 * If the tupdesc is known to correspond to a named rowtype (such as a table's
 * rowtype) then tdtypeid identifies that type and tdtypmod is -1.  Otherwise
 * tdtypeid is RECORDOID, and tdtypmod can be either -1 for a fully anonymous
 * row type, or a value >= 0 to allow the rowtype to be looked up in the
 * typcache.c type cache.
 *
 * Note that tdtypeid is never the OID of a domain over composite, even if
 * we are dealing with values that are known (at some higher level) to be of
 * a domain-over-composite type.  This is because tdtypeid/tdtypmod need to
 * match up with the type labeling of composite Datums, and those are never
 * explicitly marked as being of a domain type, either.
 *
 * Tuple descriptors that live in caches (relcache or typcache, at present)
 * are reference-counted: they can be deleted when their reference count goes
 * to zero.  Tuple descriptors created by the executor need no reference
 * counting, however: they are simply created in the appropriate memory
 * context and go away when the context is freed.  We set the tdrefcount
 * field of such a descriptor to -1, while reference-counted descriptors
 * always have tdrefcount >= 0.
 *
 * Beyond the compact_attrs variable length array, the TupleDesc stores an
 * array of FormData_pg_attribute.  The TupleDescAttr() function, as defined
 * below, takes care of calculating the address of the elements of the
 * FormData_pg_attribute array.
 *
 * The array of CompactAttribute is effectively an abbreviated version of the
 * array of FormData_pg_attribute.  Because CompactAttribute is significantly
 * smaller than FormData_pg_attribute, code, especially performance-critical
 * code, should prioritize using the fields from the CompactAttribute over the
 * equivalent fields in FormData_pg_attribute.
 *
 * Any code making changes manually to and fields in the FormData_pg_attribute
 * array must subsequently call populate_compact_attribute() to flush the
 * changes out to the corresponding 'compact_attrs' element.
 *
 * firstNonCachedOffsetAttr stores the index into the compact_attrs array for
 * the first attribute that we don't have a known attcacheoff for.
 *
 * firstNonGuaranteedAttr stores the index to into the compact_attrs array for
 * the first attribute that is either NULLable, missing, or !attbyval.  This
 * can be used in locations as a guarantee that attributes before this will
 * always exist in tuples.  The !attbyval part isn't required for this, but
 * including this allows various tuple deforming routines to forego any checks
 * for !attbyval.
 *
 * Once a TupleDesc has been populated, before it is used for any purpose,
 * TupleDescFinalize() must be called on it.
 */
typedef struct TupleDescData
{
	int			natts;			/* number of attributes in the tuple */
	Oid			tdtypeid;		/* composite type ID for tuple type */
	int32		tdtypmod;		/* typmod for tuple type */
	int			tdrefcount;		/* reference count, or -1 if not counting */
	int			firstNonCachedOffsetAttr;	/* index of first compact_attrs
											 * element without an attcacheoff */
	int			firstNonGuaranteedAttr; /* index of the first nullable,
										 * missing, dropped, or !attbyval
										 * compact_attrs element. */
	TupleConstr *constr;		/* constraints, or NULL if none */
	/* compact_attrs[N] is the compact metadata of Attribute Number N+1 */
	CompactAttribute compact_attrs[FLEXIBLE_ARRAY_MEMBER];
}			TupleDescData;
typedef struct TupleDescData *TupleDesc;
```

### `Var`

C kind: struct typedef

Definition path: `src/include/nodes/primnodes.h:262`

```c
typedef struct Var
{
	Expr		xpr;

	/*
	 * index of this var's relation in the range table, or
	 * INNER_VAR/OUTER_VAR/etc
	 */
	int			varno;

	/*
	 * attribute number of this var, or zero for all attrs ("whole-row Var")
	 */
	AttrNumber	varattno;

	/* pg_type OID for the type of this var */
	Oid			vartype pg_node_attr(query_jumble_ignore);
	/* pg_attribute typmod value */
	int32		vartypmod pg_node_attr(query_jumble_ignore);
	/* OID of collation, or InvalidOid if none */
	Oid			varcollid pg_node_attr(query_jumble_ignore);

	/*
	 * RT indexes of outer joins that can replace the Var's value with null.
	 * We can omit varnullingrels in the query jumble, because it's fully
	 * determined by varno/varlevelsup plus the Var's query location.
	 */
	Bitmapset  *varnullingrels pg_node_attr(query_jumble_ignore);

	/*
	 * for subquery variables referencing outer relations; 0 in a normal var,
	 * >0 means N levels up
	 */
	Index		varlevelsup;

	/* returning type of this var (see above) */
	VarReturningType varreturningtype;

	/*
	 * varnosyn/varattnosyn are ignored for equality, because Vars with
	 * different syntactic identifiers are semantically the same as long as
	 * their varno/varattno match.
	 */
	/* syntactic relation index (0 if unknown) */
	Index		varnosyn pg_node_attr(equal_ignore, query_jumble_ignore);
	/* syntactic attribute number */
	AttrNumber	varattnosyn pg_node_attr(equal_ignore, query_jumble_ignore);

	/* token location, or -1 if unknown */
	ParseLoc	location;
} Var;
```
