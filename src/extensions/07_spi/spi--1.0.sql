CREATE FUNCTION spi_join_catalog_data() RETURNS TABLE (
      relname text,
      nspname text
  )
  AS 'MODULE_PATHNAME', 'spi_join_catalog_data'
  LANGUAGE C STRICT IMMUTABLE;
