-- KO - Null parameter is not supported
SELECT count_via_bms(NULL);

-- KO - Value out of INT range
SELECT count_via_bms(99999999999999999);

-- KO - Negative values are not supported
SELECT count_via_bms(-100);

SELECT count_via_bms(0);

SELECT count_via_bms(10);

SELECT count_via_bms(100);

-- Test debug statements
SET client_min_messages = DEBUG2;
SELECT count_via_bms(100);
RESET client_min_messages;
