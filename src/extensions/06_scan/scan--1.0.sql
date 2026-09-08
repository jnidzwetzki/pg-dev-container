-- The scan functions report the contents of the relation they are given. Their 
-- result depends on the table data, and the planner constant-folds an
-- IMMUTABLE function that has only constant arguments, which would run the 
-- scan at plan time and cache the outcome in prepared plans.
CREATE FUNCTION full_table_scan(REGCLASS) RETURNS VOID
  AS 'MODULE_PATHNAME', 'full_table_scan'
  LANGUAGE C STRICT VOLATILE;

CREATE FUNCTION table_scan_with_scankeys(REGCLASS) RETURNS VOID
  AS 'MODULE_PATHNAME', 'table_scan_with_scankeys'
  LANGUAGE C STRICT VOLATILE;

CREATE FUNCTION table_scan_with_index(tablename REGCLASS, indexname REGCLASS) RETURNS VOID
  AS 'MODULE_PATHNAME', 'table_scan_with_index'
  LANGUAGE C STRICT VOLATILE;

CREATE FUNCTION table_scan_and_sort_attribute(tablename REGCLASS, attrname TEXT) RETURNS VOID
  AS 'MODULE_PATHNAME', 'table_scan_and_sort_attribute'
  LANGUAGE C STRICT VOLATILE;

-- A pure catalog lookup without side effects. The result can change between
-- statements (the column could be dropped), so STABLE.
CREATE FUNCTION get_attribute_type (tablename REGCLASS, attrname TEXT) RETURNS OID
  AS 'MODULE_PATHNAME', 'get_attribute_type'
  LANGUAGE C STRICT STABLE;
