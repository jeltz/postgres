--
-- EACH ELEMENT FOREIGN KEY CONSTRAINTS
--

CREATE TABLE pk_table (a int PRIMARY KEY, b text);
CREATE TABLE fk_table (a int[], b int);

-- OK - Insert test data into pk_table
INSERT INTO pk_table VALUES (1, 'Test1');
INSERT INTO pk_table VALUES (2, 'Test2');
INSERT INTO pk_table VALUES (3, 'Test3');
INSERT INTO pk_table VALUES (4, 'Test4');
INSERT INTO pk_table VALUES (5, 'Test5');

-- OK - Check alter table
ALTER TABLE fk_table ADD CONSTRAINT fk_name FOREIGN KEY (EACH ELEMENT OF a) REFERENCES pk_table;
ALTER TABLE fk_table DROP CONSTRAINT fk_name;

-- OK - Check alter table with rows
INSERT INTO fk_table VALUES ('{1}', 1);
ALTER TABLE fk_table ADD CONSTRAINT fk_name FOREIGN KEY (EACH ELEMENT OF a) REFERENCES pk_table;
ALTER TABLE fk_table DROP CONSTRAINT fk_name;

-- FAIL - Check alter table with failing rows
INSERT INTO fk_table VALUES ('{10,1}', 2);
ALTER TABLE fk_table ADD CONSTRAINT fk_name FOREIGN KEY (EACH ELEMENT OF a) REFERENCES pk_table;
DROP TABLE fk_table;

-- OK - Check create table
CREATE TABLE fk_table (a int[], FOREIGN KEY (EACH ELEMENT OF a) REFERENCES pk_table, b int);

-- OK - Check create table with multi dimensional column
CREATE TABLE fk_table_multi_dim (a int[][], FOREIGN KEY (EACH ELEMENT OF a) REFERENCES pk_table, b int);

-- OK - Check create table with not null
CREATE TABLE fk_table_not_null (a int[] NOT NULL, FOREIGN KEY (EACH ELEMENT OF a) REFERENCES pk_table, b int);

-- OK - Insert successful rows
INSERT INTO fk_table VALUES ('{1}', 3);
INSERT INTO fk_table VALUES ('{2}', 4);
INSERT INTO fk_table VALUES ('{1}', 5);
INSERT INTO fk_table VALUES ('{3}', 6);
INSERT INTO fk_table VALUES ('{1}', 7);
INSERT INTO fk_table VALUES ('{4,5}', 8);
INSERT INTO fk_table VALUES ('{4,4}', 9);
INSERT INTO fk_table VALUES (NULL, 10);
INSERT INTO fk_table VALUES ('{}', 11);
INSERT INTO fk_table VALUES ('{1,NULL}', 12);
INSERT INTO fk_table VALUES ('{NULL}', 13);
INSERT INTO fk_table_multi_dim VALUES ('{{4,5},{1,2},{1,3}}', 14);
INSERT INTO fk_table_multi_dim VALUES ('{{4,5},{NULL,2},{NULL,3}}', 15);

-- FAIL - Insert failed rows
INSERT INTO fk_table VALUES ('{6}', 16);
INSERT INTO fk_table VALUES ('{4,6}', 17);
INSERT INTO fk_table VALUES ('{6,NULL}', 18);
INSERT INTO fk_table VALUES ('{6,NULL,4,NULL}', 19);
INSERT INTO fk_table_multi_dim VALUES ('{{1,2},{6,NULL}}', 20);
INSERT INTO fk_table_not_null VALUES (NULL, 21);

-- OK - Check fk_table
SELECT * FROM fk_table ORDER BY a,b;

-- FAIL - Delete a row from pk_table (must fail due to ON DELETE NO ACTION)
DELETE FROM pk_table WHERE a=1;

-- FAIL - Update a row from pk_table (must fail due to ON UPDATE NO ACTION)
UPDATE pk_table SET a=7 WHERE a=1;

-- OK - Check UPDATE on fk_table
UPDATE fk_table SET a='{1}' WHERE b=4;

-- Check fk_table for update of matched row
SELECT * FROM fk_table ORDER BY a,b;

DROP TABLE fk_table;
DROP TABLE fk_table_not_null;
DROP TABLE fk_table_multi_dim;

-- Allowed references with actions (NO ACTION, RESTRICT)
CREATE TABLE fk_table (a int[], FOREIGN KEY (EACH ELEMENT OF a) REFERENCES pk_table ON DELETE NO ACTION ON UPDATE NO ACTION, b int);
DROP TABLE fk_table;
CREATE TABLE fk_table (a int[], FOREIGN KEY (EACH ELEMENT OF a) REFERENCES pk_table ON DELETE NO ACTION ON UPDATE RESTRICT, b int);
DROP TABLE fk_table;
CREATE TABLE fk_table (a int[], FOREIGN KEY (EACH ELEMENT OF a) REFERENCES pk_table ON DELETE RESTRICT ON UPDATE NO ACTION, b int);
DROP TABLE fk_table;
CREATE TABLE fk_table (a int[], FOREIGN KEY (EACH ELEMENT OF a) REFERENCES pk_table ON DELETE RESTRICT ON UPDATE RESTRICT, b int);
DROP TABLE fk_table;

-- FAIL - Not allowed references (SET NULL, SET DEFAULT, CASCADE)
CREATE TABLE fk_table (a int[], FOREIGN KEY (EACH ELEMENT OF a) REFERENCES pk_table ON DELETE NO ACTION ON UPDATE SET NULL, b int);
DROP TABLE IF EXISTS fk_table;
CREATE TABLE fk_table (a int[], FOREIGN KEY (EACH ELEMENT OF a) REFERENCES pk_table ON DELETE NO ACTION ON UPDATE SET DEFAULT, b int);
DROP TABLE IF EXISTS fk_table;
CREATE TABLE fk_table (a int[], FOREIGN KEY (EACH ELEMENT OF a) REFERENCES pk_table ON DELETE NO ACTION ON UPDATE CASCADE, b int);
DROP TABLE IF EXISTS fk_table;
CREATE TABLE fk_table (a int[], FOREIGN KEY (EACH ELEMENT OF a) REFERENCES pk_table ON DELETE RESTRICT ON UPDATE SET NULL, b int);
DROP TABLE IF EXISTS fk_table;
CREATE TABLE fk_table (a int[], FOREIGN KEY (EACH ELEMENT OF a) REFERENCES pk_table ON DELETE RESTRICT ON UPDATE SET DEFAULT, b int);
DROP TABLE IF EXISTS fk_table;
CREATE TABLE fk_table (a int[], FOREIGN KEY (EACH ELEMENT OF a) REFERENCES pk_table ON DELETE RESTRICT ON UPDATE CASCADE, b int);
DROP TABLE IF EXISTS fk_table;
CREATE TABLE fk_table (a int[], FOREIGN KEY (EACH ELEMENT OF a) REFERENCES pk_table ON DELETE SET NULL ON UPDATE NO ACTION, b int);
DROP TABLE IF EXISTS fk_table;
CREATE TABLE fk_table (a int[], FOREIGN KEY (EACH ELEMENT OF a) REFERENCES pk_table ON DELETE SET NULL ON UPDATE RESTRICT, b int);
DROP TABLE IF EXISTS fk_table;
CREATE TABLE fk_table (a int[], FOREIGN KEY (EACH ELEMENT OF a) REFERENCES pk_table ON DELETE SET NULL ON UPDATE SET NULL, b int);
DROP TABLE IF EXISTS fk_table;
CREATE TABLE fk_table (a int[], FOREIGN KEY (EACH ELEMENT OF a) REFERENCES pk_table ON DELETE SET NULL ON UPDATE SET DEFAULT, b int);
DROP TABLE IF EXISTS fk_table;
CREATE TABLE fk_table (a int[], FOREIGN KEY (EACH ELEMENT OF a) REFERENCES pk_table ON DELETE SET NULL ON UPDATE CASCADE, b int);
DROP TABLE IF EXISTS fk_table;
CREATE TABLE fk_table (a int[], FOREIGN KEY (EACH ELEMENT OF a) REFERENCES pk_table ON DELETE CASCADE ON UPDATE NO ACTION, b int);
DROP TABLE IF EXISTS fk_table;
CREATE TABLE fk_table (a int[], FOREIGN KEY (EACH ELEMENT OF a) REFERENCES pk_table ON DELETE CASCADE ON UPDATE RESTRICT, b int);
DROP TABLE IF EXISTS fk_table;
CREATE TABLE fk_table (a int[], FOREIGN KEY (EACH ELEMENT OF a) REFERENCES pk_table ON DELETE CASCADE ON UPDATE SET NULL, b int);
DROP TABLE IF EXISTS fk_table;
CREATE TABLE fk_table (a int[], FOREIGN KEY (EACH ELEMENT OF a) REFERENCES pk_table ON DELETE CASCADE ON UPDATE SET DEFAULT, b int);
DROP TABLE IF EXISTS fk_table;
CREATE TABLE fk_table (a int[], FOREIGN KEY (EACH ELEMENT OF a) REFERENCES pk_table ON DELETE CASCADE ON UPDATE CASCADE, b int);
DROP TABLE IF EXISTS fk_table;
CREATE TABLE fk_table (a int[], FOREIGN KEY (EACH ELEMENT OF a) REFERENCES pk_table ON DELETE SET DEFAULT ON UPDATE NO ACTION, b int);
DROP TABLE IF EXISTS fk_table;
CREATE TABLE fk_table (a int[], FOREIGN KEY (EACH ELEMENT OF a) REFERENCES pk_table ON DELETE SET DEFAULT ON UPDATE RESTRICT, b int);
DROP TABLE IF EXISTS fk_table;
CREATE TABLE fk_table (a int[], FOREIGN KEY (EACH ELEMENT OF a) REFERENCES pk_table ON DELETE SET DEFAULT ON UPDATE SET NULL, b int);
DROP TABLE IF EXISTS fk_table;
CREATE TABLE fk_table (a int[], FOREIGN KEY (EACH ELEMENT OF a) REFERENCES pk_table ON DELETE SET DEFAULT ON UPDATE SET DEFAULT, b int);
DROP TABLE IF EXISTS fk_table;
CREATE TABLE fk_table (a int[], FOREIGN KEY (EACH ELEMENT OF a) REFERENCES pk_table ON DELETE SET DEFAULT ON UPDATE CASCADE, b int);
DROP TABLE IF EXISTS fk_table;

DROP TABLE pk_table;

-- Check reference on empty table
CREATE TABLE pk_table (a int PRIMARY KEY);
CREATE TABLE fk_table  (a int[], FOREIGN KEY (EACH ELEMENT OF a) REFERENCES pk_table);
INSERT INTO fk_table VALUES ('{}');
DROP TABLE fk_table;
DROP TABLE pk_table;

--
-- ??? Why is it meaningful to test same things with different type?
--
-- Repeat a similar test using CHAR(1) keys rather than int
CREATE TABLE pk_table (a CHAR(1) PRIMARY KEY, b text);

-- Populate the primary table
INSERT INTO pk_table VALUES ('A', 'Test A');
INSERT INTO pk_table VALUES ('B', 'Test B');
INSERT INTO pk_table VALUES ('C', 'Test C');

-- Create the refrencing table
CREATE TABLE fk_table (a char(1)[], FOREIGN KEY (EACH ELEMENT OF a) REFERENCES pk_table ON UPDATE RESTRICT ON DELETE RESTRICT, b int);

-- Insert valid rows into FK TABLE
INSERT INTO fk_table VALUES ('{"A"}', 1);
INSERT INTO fk_table VALUES ('{"B"}', 2);
INSERT INTO fk_table VALUES ('{"C"}', 3);
INSERT INTO fk_table VALUES ('{"A","B","C"}', 4);

-- Insert invalid rows into FK TABLE
INSERT INTO fk_table VALUES ('{"D"}', 5);
INSERT INTO fk_table VALUES ('{"A","B","D"}', 6);

-- Check fk_table
SELECT * FROM fk_table;

-- Delete a row from pk_table (must fail due to ON DELETE RESTRICT)
DELETE FROM pk_table WHERE a='A';

-- Check fk_table for removal of matched row
SELECT * FROM fk_table;

-- Update a row from pk_table (must fail due to ON UPDATE RESTRICT)
UPDATE pk_table SET a='D' WHERE a='B';

-- Check fk_table for update of matched row
SELECT * FROM fk_table;

-- Cleanup
DROP TABLE fk_table;
DROP TABLE pk_table;

-- Composite primary keys
CREATE TABLE pk_table (id1 CHAR(1), id2 CHAR(1), b text, PRIMARY KEY (id1, id2));

-- Populate the primary table
INSERT INTO pk_table VALUES ('A', 'A', 'Test A');
INSERT INTO pk_table VALUES ('A', 'B', 'Test B');
INSERT INTO pk_table VALUES ('B', 'C', 'Test B');

-- Create the refrencing table
CREATE TABLE fk_table (fid1 CHAR(1), fid2 CHAR(1)[], b text, FOREIGN KEY (fid1, EACH ELEMENT OF fid2) REFERENCES pk_table);

-- Insert valid rows into FK TABLE
INSERT INTO fk_table VALUES ('A', ARRAY['A','B'], '1');
INSERT INTO fk_table VALUES ('B', ARRAY['C'], '2');

-- Insert invalid rows into FK TABLE
INSERT INTO fk_table VALUES ('A', ARRAY['A','B', 'C'], '3');
INSERT INTO fk_table VALUES ('B', ARRAY['A'], '4');

-- Cleanup
DROP TABLE fk_table;
DROP TABLE pk_table;

-- Test Array Element Foreign Keys with composite type
CREATE TYPE invoiceid AS (year_part int, progressive_part int);
CREATE TABLE pk_table (id invoiceid PRIMARY KEY, b text);

-- Populate the primary table
INSERT INTO pk_table VALUES (ROW(2010, 99), 'Last invoice for 2010');
INSERT INTO pk_table VALUES (ROW(2011, 1), 'First invoice for 2011');
INSERT INTO pk_table VALUES (ROW(2011, 2), 'Second invoice for 2011');

-- Create the refrencing table
CREATE TABLE fk_table (id SERIAL PRIMARY KEY, invoice_ids invoiceid[], FOREIGN KEY (EACH ELEMENT OF invoice_ids) REFERENCES pk_table, b TEXT);

-- Insert valid rows into FK TABLE
INSERT INTO fk_table(invoice_ids, b) VALUES (ARRAY['(2010,99)']::invoiceid[], 'Product A');
INSERT INTO fk_table(invoice_ids, b) VALUES (ARRAY['(2011,1)','(2011,2)']::invoiceid[], 'Product B');
INSERT INTO fk_table(invoice_ids, b) VALUES (ARRAY['(2011,2)']::invoiceid[], 'Product C');

-- Insert invalid rows into FK TABLE
INSERT INTO fk_table(invoice_ids, b) VALUES (ARRAY['(2011,99)']::invoiceid[], 'Product A');
INSERT INTO fk_table(invoice_ids, b) VALUES (ARRAY['(2011,1)','(2010,1)']::invoiceid[], 'Product B');

-- Check fk_table
SELECT * FROM fk_table;

-- Delete a row from pk_table
DELETE FROM pk_table WHERE id=ROW(2010,99);

-- Check fk_table for removal of matched row
SELECT * FROM fk_table;

-- Update a row from pk_table
UPDATE pk_table SET id=ROW(2011,99) WHERE id=ROW(2011,1);

-- Check fk_table for update of matched row
SELECT * FROM fk_table;

-- Cleanup
DROP TABLE fk_table;
DROP TABLE pk_table;
DROP TYPE invoiceid;

-- Check for an array column referencing another array column (NOT ELEMENT FOREIGN KEY)
-- Create primary table with an array primary key
CREATE TABLE pk_table (id int[] PRIMARY KEY, b text);

-- Create the refrencing table
CREATE TABLE fk_table (id SERIAL PRIMARY KEY, fids int[] REFERENCES pk_table, b TEXT);

-- Populate the primary table
INSERT INTO pk_table VALUES ('{1,1}', 'A');
INSERT INTO pk_table VALUES ('{1,2}', 'B');

-- Insert valid rows into FK TABLE
INSERT INTO fk_table (fids, b) VALUES ('{1,1}', 'Product A');
INSERT INTO fk_table (fids, b) VALUES ('{1,2}', 'Product B');

-- Insert invalid rows into FK TABLE
INSERT INTO fk_table (fids, b) VALUES ('{0,1}', 'Product C');
INSERT INTO fk_table (fids, b) VALUES ('{2,1}', 'Product D');

-- Cleanup
DROP TABLE fk_table;
DROP TABLE pk_table;

-- ---------------------------------------
-- Multi-column "ELEMENT" foreign key tests
-- ---------------------------------------

-- Create pk_table with two-column primary key
CREATE TABLE pk_table (a int NOT NULL, b int NOT NULL, PRIMARY KEY (a, b));
-- Populate pk_table pairs
INSERT INTO pk_table
  SELECT a.t, a.t * b.t
  FROM
  (
    SELECT generate_series(1, 10) AS t
  ) a
  CROSS JOIN
  (
    SELECT generate_series(0, 10) AS t
  ) b;

-- Test with TABLE declaration of an element foreign key constraint (NO ACTION)
CREATE TABLE fk_table (
	a int PRIMARY KEY, b int[],
	FOREIGN KEY (a, EACH ELEMENT OF b) REFERENCES pk_table(a, b)
);
-- Insert facts
INSERT INTO fk_table VALUES (1, '{0,1,2,3,4,5}'); -- OK
INSERT INTO fk_table VALUES (2, '{0,2,4,6}'); -- OK
INSERT INTO fk_table VALUES (3, '{0,3,6,9,0,3,6,9,0,0,0,0,9,9}'); -- OK (multiple occurrences)
INSERT INTO fk_table VALUES (4, '{0,2,4}'); -- FAILS (2 is not present)
INSERT INTO fk_table VALUES (4, '{0,NULL,4}'); -- OK
INSERT INTO fk_table VALUES (5, '{0,NULL,5}'); -- OK
-- Try updates
UPDATE fk_table SET b = '{0,2,4,6}' WHERE a = 2; -- OK
UPDATE fk_table SET b = '{0,2,3,4,6}' WHERE a = 2; -- FAILS
UPDATE fk_table SET a = 20, b = '{0,2,3,4,6}' WHERE a = 2; -- FAILS (20 does not exist)
UPDATE fk_table SET b = '{0,4,8}' WHERE a = 4; -- OK
UPDATE fk_table SET b = '{0,5,NULL,10}' WHERE a = 5; -- OK
DROP TABLE fk_table;

-- Test with FOREIGN KEY after TABLE population
CREATE TABLE fk_table (
	a int PRIMARY KEY, b int[]
);
-- Insert facts
INSERT INTO fk_table VALUES (1, '{0,1,2,3,4,5}'); -- OK
INSERT INTO fk_table VALUES (2, '{0,2,4,6}'); -- OK
INSERT INTO fk_table VALUES (3, '{0,3,6,9,0,3,6,9,0,0,0,0,9,9}'); -- OK (multiple occurrences)
INSERT INTO fk_table VALUES (4, '{0,2,4}'); -- OK (2 is not present)
INSERT INTO fk_table VALUES (5, '{0,NULL,5}'); -- OK
-- Add foreign key (FAILS)
ALTER TABLE fk_table ADD FOREIGN KEY (a, EACH ELEMENT OF b) REFERENCES pk_table(a, b);
DROP TABLE fk_table;

-- Test with TABLE declaration of a two-dim ELEMENT foreign key constraint (FAILS)
CREATE TABLE fk_table (
	a int[] PRIMARY KEY, b int[],
	FOREIGN KEY (EACH ELEMENT OF a, EACH ELEMENT OF b) REFERENCES pk_table(a, b)
);

-- Test with two-dim ELEMENT foreign key after TABLE population
CREATE TABLE fk_table (
	a int[] PRIMARY KEY, b int[]
);
INSERT INTO fk_table VALUES ('{1}', '{0,1,2,3,4,5}'); -- OK
INSERT INTO fk_table VALUES ('{1,2}', '{0,2,4,6}'); -- OK
-- Add foreign key (FAILS)
ALTER TABLE fk_table ADD FOREIGN KEY (EACH ELEMENT OF a, EACH ELEMENT OF b) REFERENCES pk_table(a, b);
DROP TABLE fk_table;
DROP TABLE pk_table;

--
-- ??? In the test below, what is tested that is not already tested?
--
-- The only difference I can see is the order of the primary key columns...
--
--   int[], int
--
-- ...compared to...
--
--   int, int[]
--
-- ...on lines 289...307.
--
-- Why is this meaningful to test?
--
-- Check for potential name conflicts (with internal integrity checks)
CREATE TABLE x1(x1 int, x2 int, PRIMARY KEY(x1,x2));
INSERT INTO x1 VALUES
       (1,4),
       (1,5),
       (2,4),
       (2,5),
       (3,6),
       (3,7)
;
CREATE TABLE x2(x1 int[], x2 int, FOREIGN KEY(EACH ELEMENT OF x1, x2) REFERENCES x1);
INSERT INTO x2 VALUES ('{1,2}',4);
INSERT INTO x2 VALUES ('{1,3}',6); -- FAILS
DROP TABLE x2;
CREATE TABLE x2(x1 int[], x2 int);
INSERT INTO x2 VALUES ('{1,2}',4);
INSERT INTO x2 VALUES ('{1,3}',6);
ALTER TABLE x2 ADD CONSTRAINT fk_const FOREIGN KEY(EACH ELEMENT OF x1, x2) REFERENCES x1;  -- FAILS
DROP TABLE x2;
DROP TABLE x1;


-- ---------------------------------------
-- Multi-dimensional "ELEMENT" foreign key tests
-- ---------------------------------------

-- Create pk_table table with two-column primary key
CREATE TABLE pk_table (a int NOT NULL PRIMARY KEY,
	code text NOT NULL UNIQUE);
-- Populate pk_table table pairs
INSERT INTO pk_table SELECT t, 'pk_table-' || lpad(t::text, 2, '0')
	FROM (SELECT generate_series(1, 10)) a(t);

-- Test with TABLE declaration of an element foreign key constraint (NO ACTION)
CREATE TABLE fk_table (
	id int GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
	slots int[3][3], FOREIGN KEY (EACH ELEMENT OF slots) REFERENCES pk_table
);
INSERT INTO fk_table (slots) VALUES ('{{NULL, 1, NULL}, {NULL, NULL, 3}, {NULL, NULL, 6}}'); -- OK
INSERT INTO fk_table (slots) VALUES ('{{NULL, 1, NULL}, {NULL, NULL, 11}, {NULL, NULL, 6}}'); -- FAILS
INSERT INTO fk_table (slots) VALUES ('{{1, 2, 3}, {4, 5, 6}, {7, 8, 9}}'); -- OK
INSERT INTO fk_table (slots) VALUES ('{1, 2, 3, 4, 5, 6, 7, 8, 9}'); -- OK
UPDATE fk_table SET slots = '{{NULL, 1, NULL}, {NULL, NULL, 3}, {7, 8, 10}}' WHERE id = 1; -- OK
UPDATE fk_table SET slots = '{{100, 100, 100}, {NULL, NULL, 20}, {7, 8, 10}}' WHERE id = 1; -- FAILS
DROP TABLE fk_table;

-- Test with postponed foreign key
CREATE TABLE fk_table (
	id int GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
	slots int[3][3]
);
INSERT INTO fk_table (slots) VALUES ('{{NULL, 1, NULL}, {NULL, NULL, 3}, {NULL, NULL, 6}}'); -- OK
INSERT INTO fk_table (slots) VALUES ('{{NULL, 1, NULL}, {NULL, NULL, 11}, {NULL, NULL, 6}}'); -- OK
INSERT INTO fk_table (slots) VALUES ('{{1, 2, 3}, {4, 5, 6}, {7, 8, 9}}'); -- OK
INSERT INTO fk_table (slots) VALUES ('{1, 2, 3, 4, 5, 6, 7, 8, 9}'); -- OK
ALTER TABLE fk_table ADD FOREIGN KEY (EACH ELEMENT OF slots) REFERENCES pk_table; -- FAILS
DELETE FROM fk_table WHERE id = 2; -- REMOVE ISSUE
ALTER TABLE fk_table ADD FOREIGN KEY (EACH ELEMENT OF slots) REFERENCES pk_table; -- NOW OK
INSERT INTO fk_table (slots) VALUES ('{{NULL, 1, NULL}, {NULL, NULL, 11}, {NULL, NULL, 6}}'); -- FAILS
DROP TABLE fk_table;


-- Leave tables in the database
CREATE TABLE pk_table_violating (a int PRIMARY KEY, b text);
CREATE TABLE fk_tableFORELEMENTFK (a int[], FOREIGN KEY (EACH ELEMENT OF a) REFERENCES pk_table_violating, b int);

-- Check ALTER TABLE ALTER TYPE
ALTER TABLE fk_tableFORELEMENTFK ALTER a TYPE int[];

-- Check GIN index
-- Define pk_table_gin
CREATE TABLE pk_table_gin (a int PRIMARY KEY, b text);

-- Insert test data into pk_table_gin
INSERT INTO pk_table_gin VALUES (1, 'Test1');
INSERT INTO pk_table_gin VALUES (2, 'Test2');
INSERT INTO pk_table_gin VALUES (3, 'Test3');
INSERT INTO pk_table_gin VALUES (4, 'Test4');
INSERT INTO pk_table_gin VALUES (5, 'Test5');
INSERT INTO pk_table_gin VALUES (6, 'Test6');

-- Define fk_table_gin
CREATE TABLE fk_table_gin (a int[],
    b int PRIMARY KEY,
    FOREIGN KEY (EACH ELEMENT OF a) REFERENCES pk_table_gin
    ON DELETE NO ACTION ON UPDATE NO ACTION);

-- -- Create index on fk_table_gin
CREATE INDEX ON fk_table_gin USING gin (a array_ops);

-- Populate Table
INSERT INTO fk_table_gin VALUES ('{5}', 1);
INSERT INTO fk_table_gin VALUES ('{3,2}', 2);
INSERT INTO fk_table_gin VALUES ('{3,5,2,5}', 3);
INSERT INTO fk_table_gin VALUES ('{3,4,4}', 4);
INSERT INTO fk_table_gin VALUES ('{3,5,4,1,3}', 5);
INSERT INTO fk_table_gin VALUES ('{1}', 6);
INSERT INTO fk_table_gin VALUES ('{5,1}', 7);
INSERT INTO fk_table_gin VALUES ('{2,1,2,4,1}', 8);
INSERT INTO fk_table_gin VALUES ('{4,2}', 9);
INSERT INTO fk_table_gin VALUES ('{3,4,5,3}', 10);

-- Try UPDATE
UPDATE pk_table_gin SET a=7 WHERE a=6;

-- Try using the indexable operator
SELECT * FROM fk_table_gin WHERE a @>> 5;

-- Cleanup
DROP TABLE fk_table_gin;
DROP TABLE pk_table_gin;

-- ---------------------------------------
-- Invalid refrencing key tests
-- ---------------------------------------
CREATE TABLE pk_table_violating (a int PRIMARY KEY, b text);

-- Attempt fk constraint between int <-> int
CREATE TABLE fk_table_violating (a int, FOREIGN KEY (EACH ELEMENT OF a) REFERENCES pk_table_violating, b int);

-- Attempt fk constraint between int <-> char[]
CREATE TABLE fk_table_violating (a char[], FOREIGN KEY (EACH ELEMENT OF a) REFERENCES pk_table_violating, b int);

-- Attempt fk constraint between int <-> char
CREATE TABLE fk_table_violating (a char, FOREIGN KEY (EACH ELEMENT OF a) REFERENCES pk_table_violating, b int);

-- Attempt fk constraint between int <-> smallint[]
CREATE TABLE fk_table_violating (a smallint[], FOREIGN KEY (EACH ELEMENT OF a) REFERENCES pk_table_violating, b int);

-- Attempt fk constraint between int <-> bigint[]
CREATE TABLE fk_table_violating (a bigint[], FOREIGN KEY (EACH ELEMENT OF a) REFERENCES pk_table_violating, b int);

-- Attempt fk constraint between int <-> int2vector
CREATE TABLE fk_table_violating (a int2vector, FOREIGN KEY (EACH ELEMENT OF a) REFERENCES pk_table_violating, b int);
