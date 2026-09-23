CREATE FUNCTION create_integer_set(INT) RETURNS SETOF integer
  AS 'MODULE_PATHNAME'
  LANGUAGE C STRICT IMMUTABLE;

CREATE FUNCTION get_programming_languages()
  RETURNS TABLE (
      name text,
      inventor text,
      year integer
  )
  AS 'MODULE_PATHNAME', 'get_programming_languages'
  LANGUAGE C STRICT;