#include "postgres.h"
#include "fmgr.h"
#include "funcapi.h"

#include "access/tupdesc.h"
#include "executor/spi.h"
#include "utils/builtins.h"

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(spi_join_catalog_data);

Datum spi_join_catalog_data(PG_FUNCTION_ARGS)
{
    FuncCallContext *funcctx;
    SPITupleTable *tuptab;

    if (SRF_IS_FIRSTCALL())
    {
        int ret;
        TupleDesc tupdesc;
        MemoryContext oldcontext;

        // Initialize the SPI connection
        SPI_connect();

        funcctx = SRF_FIRSTCALL_INIT();
        oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

        // Create a tuple descriptor for the result set
        tupdesc = CreateTemplateTupleDesc(2);
        TupleDescInitEntry(tupdesc, (AttrNumber)1, "relname", TEXTOID, -1, 0);
        TupleDescInitEntry(tupdesc, (AttrNumber)2, "nspname", TEXTOID, -1, 0);

        funcctx->tuple_desc = BlessTupleDesc(tupdesc);
        
        // Execute the query
        ret = SPI_execute("SELECT relname, nspname FROM pg_class JOIN "
                          "pg_namespace ON pg_class.relnamespace = pg_namespace.oid;",
                          true, 0);

        if (ret != SPI_OK_SELECT)
            ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR),
                            errmsg("SPI query failed: %s", SPI_result_code_string(ret))));

        funcctx->user_fctx = SPI_tuptable;

        MemoryContextSwitchTo(oldcontext);
    }

    funcctx = SRF_PERCALL_SETUP();

    tuptab = (SPITupleTable *)funcctx->user_fctx;

    if (funcctx->call_cntr < tuptab->numvals)
    {
        // Create a new tuple for the output
        Datum values[2];
        bool nulls[2] = {false, false};
        int row = funcctx->call_cntr;
        HeapTuple spi_tuple = tuptab->vals[row];
        TupleDesc spi_tupdesc = tuptab->tupdesc;
        HeapTuple result_tuple;
        char *relname;
        char *nspname;

        // Get the relname value
        relname = SPI_getvalue(spi_tuple, spi_tupdesc, 1);
        values[0] = CStringGetTextDatum(relname ? relname : "");
        nulls[0] = (relname == NULL);

        // Get the nspname value
        nspname = SPI_getvalue(spi_tuple, spi_tupdesc, 2);
        values[1] = CStringGetTextDatum(nspname ? nspname : "");
        nulls[1] = (nspname == NULL);

        // Create the result tuple (it copies also the values into the current memory context)
        result_tuple = heap_form_tuple(funcctx->tuple_desc, values, nulls);

        SRF_RETURN_NEXT(funcctx, HeapTupleGetDatum(result_tuple));
    }

    SPI_finish();
    SRF_RETURN_DONE(funcctx);
}