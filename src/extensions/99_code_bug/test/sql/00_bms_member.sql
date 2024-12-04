-- KO - Null parameter is not supported
SELECT check_bms_membership(NULL);

-- KO - Value out of INT range
SELECT check_bms_membership(99999999999999999);

-- KO - Nagative values are not supported
SELECT check_bms_membership(-100);

SELECT check_bms_membership(0);

SELECT check_bms_membership(10);

SELECT check_bms_membership(100);

