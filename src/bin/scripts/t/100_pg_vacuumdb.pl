
# Copyright (c) 2021-2024, PostgreSQL Global Development Group

use strict;
use warnings FATAL => 'all';

use PostgreSQL::Test::Cluster;
use PostgreSQL::Test::Utils;
use Test::More;

program_help_ok('pg_vacuumdb');
program_version_ok('pg_vacuumdb');
program_options_handling_ok('pg_vacuumdb');

my $node = PostgreSQL::Test::Cluster->new('main');
$node->init;
$node->start;

$node->issues_sql_like(
	[ 'pg_vacuumdb', 'postgres' ],
	qr/statement: VACUUM.*;/,
	'SQL VACUUM run');
$node->issues_sql_like(
	[ 'pg_vacuumdb', '-f', 'postgres' ],
	qr/statement: VACUUM \(SKIP_DATABASE_STATS, FULL\).*;/,
	'pg_vacuumdb -f');
$node->issues_sql_like(
	[ 'pg_vacuumdb', '-F', 'postgres' ],
	qr/statement: VACUUM \(SKIP_DATABASE_STATS, FREEZE\).*;/,
	'pg_vacuumdb -F');
$node->issues_sql_like(
	[ 'pg_vacuumdb', '-zj2', 'postgres' ],
	qr/statement: VACUUM \(SKIP_DATABASE_STATS, ANALYZE\).*;/,
	'pg_vacuumdb -zj2');
$node->issues_sql_like(
	[ 'pg_vacuumdb', '-Z', 'postgres' ],
	qr/statement: ANALYZE.*;/,
	'pg_vacuumdb -Z');
$node->issues_sql_like(
	[ 'pg_vacuumdb', '--disable-page-skipping', 'postgres' ],
	qr/statement: VACUUM \(DISABLE_PAGE_SKIPPING, SKIP_DATABASE_STATS\).*;/,
	'pg_vacuumdb --disable-page-skipping');
$node->issues_sql_like(
	[ 'pg_vacuumdb', '--skip-locked', 'postgres' ],
	qr/statement: VACUUM \(SKIP_DATABASE_STATS, SKIP_LOCKED\).*;/,
	'pg_vacuumdb --skip-locked');
$node->issues_sql_like(
	[ 'pg_vacuumdb', '--skip-locked', '--analyze-only', 'postgres' ],
	qr/statement: ANALYZE \(SKIP_LOCKED\).*;/,
	'pg_vacuumdb --skip-locked --analyze-only');
$node->command_fails(
	[ 'pg_vacuumdb', '--analyze-only', '--disable-page-skipping', 'postgres' ],
	'--analyze-only and --disable-page-skipping specified together');
$node->issues_sql_like(
	[ 'pg_vacuumdb', '--no-index-cleanup', 'postgres' ],
	qr/statement: VACUUM \(INDEX_CLEANUP FALSE, SKIP_DATABASE_STATS\).*;/,
	'pg_vacuumdb --no-index-cleanup');
$node->command_fails(
	[ 'pg_vacuumdb', '--analyze-only', '--no-index-cleanup', 'postgres' ],
	'--analyze-only and --no-index-cleanup specified together');
$node->issues_sql_like(
	[ 'pg_vacuumdb', '--no-truncate', 'postgres' ],
	qr/statement: VACUUM \(TRUNCATE FALSE, SKIP_DATABASE_STATS\).*;/,
	'pg_vacuumdb --no-truncate');
$node->command_fails(
	[ 'pg_vacuumdb', '--analyze-only', '--no-truncate', 'postgres' ],
	'--analyze-only and --no-truncate specified together');
$node->issues_sql_like(
	[ 'pg_vacuumdb', '--no-process-main', 'postgres' ],
	qr/statement: VACUUM \(PROCESS_MAIN FALSE, SKIP_DATABASE_STATS\).*;/,
	'pg_vacuumdb --no-process-main');
$node->command_fails(
	[ 'pg_vacuumdb', '--analyze-only', '--no-process-main', 'postgres' ],
	'--analyze-only and --no-process-main specified together');
$node->issues_sql_like(
	[ 'pg_vacuumdb', '--no-process-toast', 'postgres' ],
	qr/statement: VACUUM \(PROCESS_TOAST FALSE, SKIP_DATABASE_STATS\).*;/,
	'pg_vacuumdb --no-process-toast');
$node->command_fails(
	[ 'pg_vacuumdb', '--analyze-only', '--no-process-toast', 'postgres' ],
	'--analyze-only and --no-process-toast specified together');
$node->issues_sql_like(
	[ 'pg_vacuumdb', '-P', 2, 'postgres' ],
	qr/statement: VACUUM \(SKIP_DATABASE_STATS, PARALLEL 2\).*;/,
	'pg_vacuumdb -P 2');
$node->issues_sql_like(
	[ 'pg_vacuumdb', '-P', 0, 'postgres' ],
	qr/statement: VACUUM \(SKIP_DATABASE_STATS, PARALLEL 0\).*;/,
	'pg_vacuumdb -P 0');
$node->command_ok([qw(pg_vacuumdb -Z --table=pg_am dbname=template1)],
	'pg_vacuumdb with connection string');

$node->command_fails(
	[qw(pg_vacuumdb -Zt pg_am;ABORT postgres)],
	'trailing command in "-t", without COLUMNS');

# Unwanted; better if it failed.
$node->command_ok(
	[qw(pg_vacuumdb -Zt pg_am(amname);ABORT postgres)],
	'trailing command in "-t", with COLUMNS');

$node->safe_psql(
	'postgres', q|
  CREATE TABLE "need""q(uot" (")x" text);
  CREATE TABLE vactable (a int, b int);
  CREATE VIEW vacview AS SELECT 1 as a;

  CREATE FUNCTION f0(int) RETURNS int LANGUAGE SQL AS 'SELECT $1 * $1';
  CREATE FUNCTION f1(int) RETURNS int LANGUAGE SQL AS 'SELECT f0($1)';
  CREATE TABLE funcidx (x int);
  INSERT INTO funcidx VALUES (0),(1),(2),(3);
  CREATE SCHEMA "Foo";
  CREATE TABLE "Foo".bar(id int);
  CREATE SCHEMA "Bar";
  CREATE TABLE "Bar".baz(id int);
|);
$node->command_ok([qw|pg_vacuumdb -Z --table="need""q(uot"(")x") postgres|],
	'column list');

$node->command_fails(
	[ 'pg_vacuumdb', '--analyze', '--table', 'vactable(c)', 'postgres' ],
	'incorrect column name with ANALYZE');
$node->command_fails([ 'pg_vacuumdb', '-P', -1, 'postgres' ],
	'negative parallel degree');
$node->issues_sql_like(
	[ 'pg_vacuumdb', '--analyze', '--table', 'vactable(a, b)', 'postgres' ],
	qr/statement: VACUUM \(SKIP_DATABASE_STATS, ANALYZE\) public.vactable\(a, b\);/,
	'pg_vacuumdb --analyze with complete column list');
$node->issues_sql_like(
	[ 'pg_vacuumdb', '--analyze-only', '--table', 'vactable(b)', 'postgres' ],
	qr/statement: ANALYZE public.vactable\(b\);/,
	'pg_vacuumdb --analyze-only with partial column list');
$node->command_checks_all(
	[ 'pg_vacuumdb', '--analyze', '--table', 'vacview', 'postgres' ],
	0,
	[qr/^.*vacuuming database "postgres"/],
	[qr/^WARNING.*cannot vacuum non-tables or special system tables/s],
	'pg_vacuumdb with view');
$node->command_fails(
	[ 'pg_vacuumdb', '--table', 'vactable', '--min-mxid-age', '0', 'postgres' ],
	'pg_vacuumdb --min-mxid-age with incorrect value');
$node->command_fails(
	[ 'pg_vacuumdb', '--table', 'vactable', '--min-xid-age', '0', 'postgres' ],
	'pg_vacuumdb --min-xid-age with incorrect value');
$node->issues_sql_like(
	[
		'pg_vacuumdb', '--table', 'vactable', '--min-mxid-age',
		'2147483000', 'postgres'
	],
	qr/GREATEST.*relminmxid.*2147483000/,
	'pg_vacuumdb --table --min-mxid-age');
$node->issues_sql_like(
	[ 'pg_vacuumdb', '--min-xid-age', '2147483001', 'postgres' ],
	qr/GREATEST.*relfrozenxid.*2147483001/,
	'pg_vacuumdb --table --min-xid-age');
$node->issues_sql_like(
	[ 'pg_vacuumdb', '--schema', '"Foo"', 'postgres' ],
	qr/VACUUM \(SKIP_DATABASE_STATS\) "Foo".bar/,
	'pg_vacuumdb --schema');
$node->issues_sql_like(
	[ 'pg_vacuumdb', '--schema', '"Foo"', '--schema', '"Bar"', 'postgres' ],
	qr/VACUUM\ \(SKIP_DATABASE_STATS\)\ "Foo".bar
		.*VACUUM\ \(SKIP_DATABASE_STATS\)\ "Bar".baz
	/sx,
	'pg_vacuumdb multiple --schema switches');
$node->issues_sql_like(
	[ 'pg_vacuumdb', '--exclude-schema', '"Foo"', 'postgres' ],
	qr/^(?!.*VACUUM \(SKIP_DATABASE_STATS\) "Foo".bar).*$/s,
	'pg_vacuumdb --exclude-schema');
$node->issues_sql_like(
	[
		'pg_vacuumdb', '--exclude-schema', '"Foo"', '--exclude-schema',
		'"Bar"', 'postgres'
	],
	qr/^(?!.*VACUUM\ \(SKIP_DATABASE_STATS\)\ "Foo".bar
	| VACUUM\ \(SKIP_DATABASE_STATS\)\ "Bar".baz).*$/sx,
	'pg_vacuumdb multiple --exclude-schema switches');
$node->command_fails_like(
	[ 'pg_vacuumdb', '-N', 'pg_catalog', '-t', 'pg_class', 'postgres', ],
	qr/cannot vacuum specific table\(s\) and exclude schema\(s\) at the same time/,
	'cannot use options -N and -t at the same time');
$node->command_fails_like(
	[ 'pg_vacuumdb', '-n', 'pg_catalog', '-t', 'pg_class', 'postgres' ],
	qr/cannot vacuum all tables in schema\(s\) and specific table\(s\) at the same time/,
	'cannot use options -n and -t at the same time');
$node->command_fails_like(
	[ 'pg_vacuumdb', '-n', 'pg_catalog', '-N', '"Foo"', 'postgres' ],
	qr/cannot vacuum all tables in schema\(s\) and exclude schema\(s\) at the same time/,
	'cannot use options -n and -N at the same time');
$node->issues_sql_like(
	[ 'pg_vacuumdb', '-a', '-N', 'pg_catalog' ],
	qr/(?:(?!VACUUM \(SKIP_DATABASE_STATS\) pg_catalog.pg_class).)*/,
	'pg_vacuumdb -a -N');
$node->issues_sql_like(
	[ 'pg_vacuumdb', '-a', '-n', 'pg_catalog' ],
	qr/VACUUM \(SKIP_DATABASE_STATS\) pg_catalog.pg_class/,
	'pg_vacuumdb -a -n');
$node->issues_sql_like(
	[ 'pg_vacuumdb', '-a', '-t', 'pg_class' ],
	qr/VACUUM \(SKIP_DATABASE_STATS\) pg_catalog.pg_class/,
	'pg_vacuumdb -a -t');
$node->command_fails_like(
	[ 'pg_vacuumdb', '-a', '-d', 'postgres' ],
	qr/cannot vacuum all databases and a specific one at the same time/,
	'cannot use options -a and -d at the same time');
$node->command_fails_like(
	[ 'pg_vacuumdb', '-a', 'postgres' ],
	qr/cannot vacuum all databases and a specific one at the same time/,
	'cannot use option -a and a dbname as argument at the same time');

done_testing();
