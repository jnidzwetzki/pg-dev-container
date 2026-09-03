#include "postgres.h"
#include "fmgr.h"
#include "funcapi.h"

#include "access/tupdesc.h"
#include "executor/spi.h"
#include "utils/builtins.h"

PG_MODULE_MAGIC;

// Internal state that survives between the calls of the SRF
typedef struct spi_join_state
{
    TupleDesc  spi_tupdesc;  // descriptor of the SPI tuples
    HeapTuple *rows;         // copies of the SPI result tuples
    uint64     nrows;        // number of copied tuples
} spi_join_state;

PG_FUNCTION_INFO_V1(spi_join_catalog_data);

Datum spi_join_catalog_data(PG_FUNCTION_ARGS)
{
    FuncCallContext *funcctx;
    spi_join_state *state;

    if (SRF_IS_FIRSTCALL())
    {
        int ret;
        uint64 i;
        TupleDesc tupdesc;
        MemoryContext oldcontext;

        funcctx = SRF_FIRSTCALL_INIT();

        // Initialize the SPI connection and execute the query
        SPI_connect();
        ret = SPI_execute("SELECT relname, nspname FROM pg_class JOIN "
                          "pg_namespace ON pg_class.relnamespace = pg_namespace.oid;",
                          true, 0);

        if (ret != SPI_OK_SELECT)
            ereport(ERROR, (errcode(ERRCODE_INTERNAL_ERROR),
                            errmsg("SPI query failed: %s", SPI_result_code_string(ret))));

        /*
         * The SPI result is allocated in the memory context of the SPI call,
         * which is released by SPI_finish(). Everything that is needed later
         * is therefore copied into the multi_call_memory_ctx first.
         */
        oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

        // Create a tuple descriptor for the result set
        tupdesc = CreateTemplateTupleDesc(2);
        TupleDescInitEntry(tupdesc, (AttrNumber)1, "relname", TEXTOID, -1, 0);
        TupleDescInitEntry(tupdesc, (AttrNumber)2, "nspname", TEXTOID, -1, 0);
        funcctx->tuple_desc = BlessTupleDesc(tupdesc);

        state = palloc(sizeof(spi_join_state));
        state->nrows = SPI_processed;
        state->spi_tupdesc = CreateTupleDescCopy(SPI_tuptable->tupdesc);
        state->rows = palloc(sizeof(HeapTuple) * state->nrows);

        for (i = 0; i < state->nrows; i++)
            state->rows[i] = heap_copytuple(SPI_tuptable->vals[i]);

        funcctx->user_fctx = state;
        MemoryContextSwitchTo(oldcontext);

        /*
         * The copies are independent of SPI now, so the connection can be
         * closed here.
         */
        SPI_finish();
    }

    funcctx = SRF_PERCALL_SETUP();
    state = (spi_join_state *)funcctx->user_fctx;

    if (funcctx->call_cntr < state->nrows)
    {
        // Create a new tuple for the output
        Datum values[2];
        bool nulls[2] = {false, false};
        HeapTuple spi_tuple = state->rows[funcctx->call_cntr];
        HeapTuple result_tuple;
        char *relname;
        char *nspname;

        // Get the relname value
        relname = SPI_getvalue(spi_tuple, state->spi_tupdesc, 1);
        values[0] = CStringGetTextDatum(relname ? relname : "");
        nulls[0] = (relname == NULL);

        // Get the nspname value
        nspname = SPI_getvalue(spi_tuple, state->spi_tupdesc, 2);
        values[1] = CStringGetTextDatum(nspname ? nspname : "");
        nulls[1] = (nspname == NULL);

        // Create the result tuple (it copies also the values into the current memory context)
        result_tuple = heap_form_tuple(funcctx->tuple_desc, values, nulls);

        SRF_RETURN_NEXT(funcctx, HeapTupleGetDatum(result_tuple));
    }

    SRF_RETURN_DONE(funcctx);
}