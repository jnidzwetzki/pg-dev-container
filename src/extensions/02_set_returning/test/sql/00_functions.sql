SELECT create_integer_set(NULL);
SELECT create_integer_set(-1);

SELECT create_integer_set(5);
SELECT * FROM create_integer_set(5);
SELECT * FROM create_integer_set(5) LIMIT 2;


SELECT * FROM get_programming_languages();
SELECT name FROM get_programming_languages() WHERE year = 1995;