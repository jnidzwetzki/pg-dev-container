CREATE OR REPLACE FUNCTION hello_world_sql(name TEXT) RETURNS TEXT AS $$
BEGIN
    RETURN 'Hello ' || name;
END;
$$ LANGUAGE plpgsql;