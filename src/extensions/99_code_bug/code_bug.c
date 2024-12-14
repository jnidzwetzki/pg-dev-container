#include "postgres.h"

#include "nodes/bitmapset.h"
#include "utils/builtins.h"

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(count_via_bms);

Datum count_via_bms(PG_FUNCTION_ARGS);

Datum count_via_bms(PG_FUNCTION_ARGS)
{
    Bitmapset *set;
    int i;
    int result;
    int32 max_value;

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
        Assert(bms_is_member(i, set));
    }

    ereport(DEBUG2,
            (errcode(ERRCODE_SUCCESSFUL_COMPLETION),
             errmsg("count_via_bms performed \"%d\" iterations", i)));

    result = bms_num_members(set);

    bms_free(set);

    PG_RETURN_INT32((int32)result - 1);
}
