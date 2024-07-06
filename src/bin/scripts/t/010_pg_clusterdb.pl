
# Copyright (c) 2021-2024, PostgreSQL Global Development Group

use strict;
use warnings FATAL => 'all';

use PostgreSQL::Test::Cluster;
use PostgreSQL::Test::Utils;
use Test::More;

program_help_ok('pg_clusterdb');
program_version_ok('pg_clusterdb');
program_options_handling_ok('pg_clusterdb');

my $node = PostgreSQL::Test::Cluster->new('main');
$node->init;
$node->start;

$node->issues_sql_like(
	['pg_clusterdb'],
	qr/statement: CLUSTER;/,
	'SQL CLUSTER run');

$node->command_fails([ 'pg_clusterdb', '-t', 'nonexistent' ],
	'fails with nonexistent table');

$node->safe_psql('postgres',
	'CREATE TABLE test1 (a int); CREATE INDEX test1x ON test1 (a); CLUSTER test1 USING test1x'
);
$node->issues_sql_like(
	[ 'pg_clusterdb', '-t', 'test1' ],
	qr/statement: CLUSTER public\.test1;/,
	'cluster specific table');

$node->command_ok([qw(pg_clusterdb --echo --verbose dbname=template1)],
	'pg_clusterdb with connection string');

done_testing();
