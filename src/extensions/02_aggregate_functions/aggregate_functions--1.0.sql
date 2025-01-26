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
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION _int32_abs_combine_fn(state1 internal, state2 internal)
RETURNS internal
AS 'MODULE_PATHNAME', 'int32_abs_combine'
LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION _int32_abs_avg_serial_fn(state internal)
RETURNS BYTEA
AS 'MODULE_PATHNAME', 'int32_abs_avg_serialize'
LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION _int32_abs_avg_deserial_fn(sstate bytea, dummy internal)
RETURNS internal
AS 'MODULE_PATHNAME', 'int32_abs_avg_deserialize'
LANGUAGE C IMMUTABLE;

CREATE OR REPLACE AGGREGATE abs_avg(INT)
(
    sfunc = _int32_abs_avg_trans_fn,
    stype = internal,
    finalfunc = _int32_abs_avg_final_fn,
    parallel = safe,
    combinefunc = _int32_abs_combine_fn,
    serialfunc = _int32_abs_avg_serial_fn,
    deserialfunc = _int32_abs_avg_deserial_fn
);

