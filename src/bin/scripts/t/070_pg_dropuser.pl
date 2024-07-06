
# Copyright (c) 2021-2024, PostgreSQL Global Development Group

use strict;
use warnings FATAL => 'all';

use PostgreSQL::Test::Cluster;
use PostgreSQL::Test::Utils;
use Test::More;

program_help_ok('pg_dropuser');
program_version_ok('pg_dropuser');
program_options_handling_ok('pg_dropuser');

my $node = PostgreSQL::Test::Cluster->new('main');
$node->init;
$node->start;

$node->safe_psql('postgres', 'CREATE ROLE regress_foobar1');
$node->issues_sql_like(
	[ 'pg_dropuser', 'regress_foobar1' ],
	qr/statement: DROP ROLE regress_foobar1/,
	'SQL DROP ROLE run');

$node->command_fails([ 'pg_dropuser', 'regress_nonexistent' ],
	'fails with nonexistent user');

done_testing();
