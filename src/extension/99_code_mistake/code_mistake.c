#include "postgres.h"

#include "nodes/bitmapset.h"
#include "utils/builtins.h"

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(code_mistake);

Datum code_mistake(PG_FUNCTION_ARGS);

Datum code_mistake(PG_FUNCTION_ARGS)
{
    Bitmapset *set = NULL;
    int i;

    set = bms_make_singleton(0);

    for (i = 1; i < 1024; i++)
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
            (errmsg("all exptected elements are contaied")));

    bms_free(set);

    PG_RETURN_VOID();
}
