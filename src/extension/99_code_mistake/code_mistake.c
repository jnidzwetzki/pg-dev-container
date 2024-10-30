#include "postgres.h"

#include "nodes/bitmapset.h"
#include "utils/builtins.h"

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(code_mistake);

Datum code_mistake(PG_FUNCTION_ARGS);

Datum code_mistake(PG_FUNCTION_ARGS)
{
    Bitmapset *set;
    int i;
    int max_value;

    max_value = PG_GETARG_INT32(0);

    if (max_value < 0)
    {
        ereport(ERROR,
                (errcode(ERRCODE_DATA_EXCEPTION),
                 errmsg("max value must be greater than zero")));
    }

    set = bms_make_singleton(0);

    for (i = 1; i <= max_value; i++)
    {
        bms_add_member(set, i);

        if (!bms_is_member(i, set))
        {
            ereport(ERROR,
                    (errcode(ERRCODE_DATA_CORRUPTED),
                     errmsg("value \"%d\" is not part of the bitmap set", i)));
        }
    }

    ereport(INFO,
            (errmsg("all \"%d\" elements are contaied", bms_num_members(set))));

    bms_free(set);

    PG_RETURN_VOID();
}
