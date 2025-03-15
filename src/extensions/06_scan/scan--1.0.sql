CREATE FUNCTION full_table_scan(REGCLASS) RETURNS VOID
  AS 'MODULE_PATHNAME', 'full_table_scan'
  LANGUAGE C STRICT IMMUTABLE;

CREATE FUNCTION table_scan_with_scankeys(REGCLASS) RETURNS VOID
  AS 'MODULE_PATHNAME', 'table_scan_with_scankeys'
  LANGUAGE C STRICT IMMUTABLE;

CREATE FUNCTION table_scan_with_index(tablename REGCLASS, indexname REGCLASS) RETURNS VOID
  AS 'MODULE_PATHNAME', 'table_scan_with_index'
  LANGUAGE C STRICT IMMUTABLE;

CREATE FUNCTION table_scan_and_sort_attribute(tablename REGCLASS, attrname TEXT) RETURNS VOID
  AS 'MODULE_PATHNAME', 'table_scan_and_sort_attribute'
  LANGUAGE C STRICT IMMUTABLE;

CREATE FUNCTION get_attribute_type (tablename REGCLASS, attrname TEXT) RETURNS OID
  AS 'MODULE_PATHNAME', 'get_attribute_type'
  LANGUAGE C STRICT IMMUTABLE;
