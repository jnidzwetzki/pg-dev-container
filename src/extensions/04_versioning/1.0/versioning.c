#include "postgres.h"
#include "fmgr.h"

#include "utils/builtins.h"

PG_MODULE_MAGIC;


PG_FUNCTION_INFO_V1(hello_function);

Datum hello_function(PG_FUNCTION_ARGS)
{
    const char *greeting = "Hello from version 1.0";
    PG_RETURN_TEXT_P(cstring_to_text(greeting));
}
