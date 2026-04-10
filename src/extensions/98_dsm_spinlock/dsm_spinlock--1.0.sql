CREATE OR REPLACE FUNCTION grab_spinlock(INT)
    RETURNS TABLE(acquire_time interval, total_time interval)
    AS 'MODULE_PATHNAME', 'grab_spinlock'
    LANGUAGE C STRICT;