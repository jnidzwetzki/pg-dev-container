-- Direct function call
SELECT abs_avg(1234);
SELECT abs_avg(0);
SELECT abs_avg(-123);
SELECT abs_avg(NULL);

-- Test using generate series
SELECT abs_avg(i) FROM generate_series(1, 10) i;
SELECT abs_avg(i) FROM generate_series(1, 100) i;
SELECT abs_avg(i) FROM generate_series(-10, 100) i;
SELECT abs_avg(i) FROM generate_series(-100, 100) i;

-- Test using a table
CREATE TABLE test_table AS SELECT i FROM generate_series(1, 100) i;
SELECT abs_avg(i) FROM test_table;
DROP TABLE test_table;
