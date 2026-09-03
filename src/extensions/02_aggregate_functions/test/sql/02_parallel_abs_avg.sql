-- Test the parallel code path of the abs_avg aggregate.
SET max_parallel_workers_per_gather = 2;
SET parallel_setup_cost = 0;
SET parallel_tuple_cost = 0;
SET min_parallel_table_scan_size = 0;

CREATE TABLE parallel_test AS SELECT i FROM generate_series(-100000, 100000) i;
ANALYZE parallel_test;

-- Ensure the aggregate is executed in parallel
EXPLAIN (COSTS OFF) SELECT abs_avg(i) FROM parallel_test;

-- The parallel result has to match the serial one and PostgreSQL's avg()
SELECT abs_avg(i) FROM parallel_test;
SELECT avg(abs(i::BIGINT))::FLOAT FROM parallel_test;

-- All workers process zero rows here, so their transition value is NULL.
EXPLAIN (COSTS OFF) SELECT abs_avg(i) FROM parallel_test WHERE i > (SELECT 1000000);
SELECT abs_avg(i) FROM parallel_test WHERE i > (SELECT 1000000);

SET max_parallel_workers_per_gather = 0;
SELECT abs_avg(i) FROM parallel_test;

DROP TABLE parallel_test;

RESET max_parallel_workers_per_gather;
RESET parallel_setup_cost;
RESET parallel_tuple_cost;
RESET min_parallel_table_scan_size;
