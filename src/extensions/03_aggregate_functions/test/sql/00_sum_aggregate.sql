-- Direct function call
SELECT mysum(1234);
SELECT mysum(0);
SELECT mysum(-123);
SELECT mysum(NULL);

-- Test using generate series
SELECT mysum(i) FROM generate_series(1, 10) i;
SELECT mysum(i) FROM generate_series(1, 100) i;

-- Test using a table
CREATE TABLE test_table AS SELECT i FROM generate_series(1, 100) i;
SELECT mysum(i) FROM test_table;
DROP TABLE test_table;
