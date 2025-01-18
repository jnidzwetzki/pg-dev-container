-- SUM aggregate
CREATE OR REPLACE FUNCTION _int32_sum_trans_fn(state BIGINT,
   val INT) RETURNS BIGINT
   AS 'MODULE_PATHNAME', 'int32_sum_trans'
   LANGUAGE C IMMUTABLE;

CREATE OR REPLACE AGGREGATE mysum(INT)
(
    sfunc = _int32_sum_trans_fn,
    stype = BIGINT
);

--- ABS_AVG aggregate
CREATE OR REPLACE FUNCTION _int32_abs_avg_trans_fn(state internal, val INT)
RETURNS internal
AS 'MODULE_PATHNAME', 'int32_abs_avg_trans'
LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION _int32_abs_avg_final_fn(state internal)
RETURNS FLOAT
AS 'MODULE_PATHNAME', 'int32_abs_avg_final'
LANGUAGE C IMMUTABLE;

CREATE OR REPLACE AGGREGATE abs_avg(INT)
(
    sfunc = _int32_abs_avg_trans_fn,
    stype = internal,
    finalfunc = _int32_abs_avg_final_fn
);

