-- KO - Null parameter is not supported
SELECT code_mistake(NULL);

-- KO - Value out of INT range
SELECT code_mistake(99999999999999999);

-- KO - Nagative values are not supported
SELECT code_mistake(-100);

SELECT code_mistake(0);

SELECT code_mistake(10);

SELECT code_mistake(100);

