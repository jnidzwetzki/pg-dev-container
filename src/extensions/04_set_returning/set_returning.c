#include "postgres.h"
#include "fmgr.h"
#include "funcapi.h"

#include "utils/builtins.h"


PG_MODULE_MAGIC;

typedef struct integer_set_ctx
{
    int32 elements;
} integer_set_ctx;

PG_FUNCTION_INFO_V1(create_integer_set);

Datum 
create_integer_set(PG_FUNCTION_ARGS)
{
    FuncCallContext  *funcctx;
    Datum             result;
    integer_set_ctx  *ctx;

    if(SRF_IS_FIRSTCALL())
    {
        int32             elements;
        MemoryContext     oldcontext;

        /* Process user provided value in first function invocation */
        elements = PG_GETARG_INT32(0);
        if(elements <= 0)
            ereport(ERROR, (errmsg("elements parameter must be greater than 0")));

        /* Init user function context in proper memory context */
        funcctx = SRF_FIRSTCALL_INIT();
        oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);
        
        ctx = palloc(sizeof(integer_set_ctx));
        ctx->elements = elements;

        funcctx->user_fctx = ctx;

        MemoryContextSwitchTo(oldcontext);
    }

    funcctx = SRF_PERCALL_SETUP();

    ctx = funcctx->user_fctx;

    if(funcctx->call_cntr < ctx->elements)
    {
        result = Int32GetDatum(funcctx->call_cntr);
        SRF_RETURN_NEXT(funcctx, result);
    }
    else
    {
        SRF_RETURN_DONE(funcctx);
    }
}

typedef struct programming_languages
{
    char *name;
    char *inventor;
    int year;
} programming_languages;

programming_languages languages[] = 
{
    {"C", "Dennis Ritchie", 1972},
    {"C++", "Bjarne Stroustrup", 1985},
    {"Python", "Guido van Rossum", 1991},
    {"Java", "James Gosling", 1995}
};

PG_FUNCTION_INFO_V1(get_programming_languages);

Datum
get_programming_languages(PG_FUNCTION_ARGS)
{
    FuncCallContext *funcctx;

    if(SRF_IS_FIRSTCALL())
    {
        MemoryContext oldcontext;
        TupleDesc tupdesc;

        funcctx = SRF_FIRSTCALL_INIT();
        oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

        /* 
         * Note: Parameter 3 of get_call_result_type() could be used to retrieve 
         * a TupleDesc based on the function definition. However, this is done
         * manually to illustrate how to create the TupleDesc.
         */
        if(get_call_result_type(fcinfo, NULL, NULL) != TYPEFUNC_COMPOSITE)
            ereport(ERROR, (errmsg("return type must be a composite type")));

        tupdesc = CreateTemplateTupleDesc(3);
        TupleDescInitEntry(tupdesc, (AttrNumber) 1, "name", TEXTOID, -1, 0);
        TupleDescInitEntry(tupdesc, (AttrNumber) 2, "inventor", TEXTOID, -1, 0);
        TupleDescInitEntry(tupdesc, (AttrNumber) 3, "year", INT4OID, -1, 0);

        /* Cache tuple desc */
        funcctx->tuple_desc = BlessTupleDesc(tupdesc);

        MemoryContextSwitchTo(oldcontext);
    }

    funcctx = SRF_PERCALL_SETUP();
    if(funcctx->call_cntr < lengthof(languages))
    {
        Datum		values[3];
        bool		nulls[3] = {0};
        HeapTuple	tuple;
        programming_languages *lang;
        Datum result;

        lang = &languages[funcctx->call_cntr];

        values[0] = CStringGetTextDatum(lang->name);
        values[1] = CStringGetTextDatum(lang->inventor);
        values[2] = Int32GetDatum(lang->year);

        tuple = heap_form_tuple(funcctx->tuple_desc, values, nulls);

        result = HeapTupleGetDatum(tuple);
        SRF_RETURN_NEXT(funcctx, result);
    }
    else
    {
        SRF_RETURN_DONE(funcctx);
    }
}


