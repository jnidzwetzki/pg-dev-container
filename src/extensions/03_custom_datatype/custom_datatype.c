#include "postgres.h"
#include "fmgr.h"

#include <math.h>

#include "libpq/pqformat.h"

PG_MODULE_MAGIC;

/* Define the custom datatype or PostgreSQL that represents a point in 2 dimensional space. */
typedef struct Point2D
{
    double x;
    double y;
} Point2D;

PG_FUNCTION_INFO_V1(point2d_in);
PG_FUNCTION_INFO_V1(point2d_out);
PG_FUNCTION_INFO_V1(point2d_recv);
PG_FUNCTION_INFO_V1(point2d_send);

/* 
 * Define the custom datatype's input function.
 * This function takes a string and converts it to a Point.
 */
Datum point2d_in(PG_FUNCTION_ARGS)
{
    char *str = PG_GETARG_CSTRING(0);
    Point2D *result;
    double x, y;
    int n;
    int chars_read = 0;

    n = sscanf(str, " ( %lf , %lf )%n", &x, &y, &chars_read);

    if (n != 2 || str[chars_read] != '\0')
        ereport(ERROR, (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION), 
           errmsg("invalid input syntax for type Point: \"%s\"", str)));

    /* Allocate memory for the Point. */
    result = (Point2D *)palloc(sizeof(Point2D));
    result->x = x;
    result->y = y;

    PG_RETURN_POINTER(result);
}

/* 
 * Define the custom datatype's output function.
 * This function takes a Point and converts it to a string.
 */
Datum point2d_out(PG_FUNCTION_ARGS)
{
    Point2D *point = (Point2D *)PG_GETARG_POINTER(0);
    char *result;

    result = psprintf("(%g,%g)", point->x, point->y);

    PG_RETURN_CSTRING(result);
}

/* 
 * Define the custom datatype's receive function.
 * This function takes a binary representation of a Point and converts it to a Point.
 */
Datum point2d_recv(PG_FUNCTION_ARGS)
{
    StringInfo buf = (StringInfo)PG_GETARG_POINTER(0);
    Point2D *result;

    result = (Point2D *)palloc(sizeof(Point2D));
    result->x = pq_getmsgfloat8(buf);
    result->y = pq_getmsgfloat8(buf);

    PG_RETURN_POINTER(result);
}

/* 
 * Define the custom datatype's send function.
 * This function takes a Point and converts it to a binary representation.
 */
Datum point2d_send(PG_FUNCTION_ARGS)
{
    Point2D *point = (Point2D *)PG_GETARG_POINTER(0);
    StringInfoData buf;

    pq_begintypsend(&buf);
    pq_sendfloat8(&buf, point->x);
    pq_sendfloat8(&buf, point->y);

    PG_RETURN_BYTEA_P(pq_endtypsend(&buf));
}

/* Optional distance function */

/*
 * Define a distance function for the custom datatype.
 */
PG_FUNCTION_INFO_V1(point2d_distance);
Datum point2d_distance(PG_FUNCTION_ARGS)
{
    Point2D *a = (Point2D *)PG_GETARG_POINTER(0);
    Point2D *b = (Point2D *)PG_GETARG_POINTER(1);
    double result;

    result = sqrt(pow(a->x - b->x, 2) + pow(a->y - b->y, 2));

    PG_RETURN_FLOAT8(result);
}