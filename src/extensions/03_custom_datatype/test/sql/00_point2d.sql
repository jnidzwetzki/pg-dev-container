-- Working type creation
SELECT '(1,1)'::point2d;
SELECT '(0,0)'::point2d;
SELECT '(-1.5,2.5)'::point2d;
SELECT NULL::point2d;

-- Error cases
SELECT '1,1'::point2d;
SELECT '(1,1'::point2d;
SELECT '1,1)'::point2d;
SELECT '(1,1,1)'::point2d;

-- Distance calculation
SELECT point2d_distance('(1,1)'::point2d, '(1,1)'::point2d);
SELECT point2d_distance('(1,1)'::point2d, '(0,0)'::point2d);
SELECT point2d_distance('(1,1)'::point2d, '(-1.5,2.5)'::point2d);
SELECT point2d_distance('(0,0)'::point2d, '(-1.5,2.5)'::point2d);
SELECT point2d_distance('(0,0)'::point2d, NULL::point2d);

-- Use the datatype in a table
CREATE TABLE point2d_table (id serial, point point2d);
INSERT INTO point2d_table (point) VALUES ('(1,1)'::point2d), ('(0,0)'::point2d), ('(-1.5,2.5)'::point2d), ('(10, 10)'::point2d);

SELECT * FROM point2d_table;

-- Find all points that are more than 2.5 units apart
SELECT tbl1.point AS p1, tbl2.point AS p2, point2d_distance(tbl1.point, tbl2.point) AS distance
      FROM point2d_table tbl1, point2d_table tbl2 
      WHERE tbl1.id < tbl2.id AND point2d_distance(tbl1.point, tbl2.point) > 2.5;

DROP TABLE point2d_table;