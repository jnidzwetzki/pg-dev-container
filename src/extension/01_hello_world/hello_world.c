#include "postgres.h"
#include "varatt.h"
#include "fmgr.h"

#include "utils/builtins.h"

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(hello_world);

Datum hello_world(PG_FUNCTION_ARGS);

Datum hello_world(PG_FUNCTION_ARGS)
{
    const char *greeting = "Hello ";
    char *username = NULL;
    char *result = NULL;
    int greetlen;

    // Get argument and convert from PG text datatype to C char
    username = text_to_cstring(PG_GETARG_TEXT_PP(0));

    // Calculate result length
    greetlen = strlen(greeting) + strlen(username) + 1;

    // Request memory
    result = (char *)palloc(greetlen);

    // Build result
    strcpy(result, greeting);
    strcat(result, username);

    // Return result and convert it into a PG text datatype
    PG_RETURN_TEXT_P(cstring_to_text(result));
}
