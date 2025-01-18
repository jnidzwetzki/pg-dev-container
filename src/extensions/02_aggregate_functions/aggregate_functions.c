#include "postgres.h"
#include "fmgr.h"

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(int32_sum_trans);

/*
 * State transition function for mysum aggregate.
 * Based on PG's int4_sum function.
 */
Datum int32_sum_trans(PG_FUNCTION_ARGS)
{
    int64 state;
    int64 newval;

    /* If the state is null initialize it */
    if (PG_ARGISNULL(0))
    {
        /* If the first parameter is also null, the call
         * is a no-op.
         */
        if (PG_ARGISNULL(1))
            PG_RETURN_NULL();

        /* Initialize the state to the first parameter */
        state = (int64)PG_GETARG_INT32(1);
        PG_RETURN_INT64(state);
    }

    /* Get the current state */
    state = PG_GETARG_INT64(0);

    /* If the second parameter is null, the call is a no-op */
    if (PG_ARGISNULL(1))
        PG_RETURN_INT64(state);

    /* Perform the sum operation and return the new state */
    newval = state + (int64)PG_GETARG_INT32(1);
    PG_RETURN_INT64(newval);
}

PG_FUNCTION_INFO_V1(int32_abs_avg_trans);
PG_FUNCTION_INFO_V1(int32_abs_avg_final);

/* Internal state of ths abs_avg aggregate */
typedef struct int32_abs_avg_state
{
    int64 sum;
    int64 count;
} int32_abs_avg_state;

/*
 * abs_avg aggregate transition function. This function takes two arguments:
 * (1) the current state
 * (2) the next element
 */
Datum int32_abs_avg_trans(PG_FUNCTION_ARGS)
{
    MemoryContext agg_context;
    MemoryContext old_context;
    int32_abs_avg_state *state;

    /* Check that we are called as an aggregate and populate agg_context */
    if (! AggCheckCallContext(fcinfo, &agg_context))
        elog(ERROR, "aggregate function called in non-aggregate context");

    /* Allocate new data in the agg memory context */
    old_context = MemoryContextSwitchTo(agg_context);

    /* If the state is null initialize it */
    if (PG_ARGISNULL(0))
    {
        state = palloc(sizeof(int32_abs_avg_state));
        state->sum = 0;
        state->count = 0;
    }
    /* If we have a state use it */
    else
    {
        state = (int32_abs_avg_state *)PG_GETARG_POINTER(0);
    }

    /* If the argument is not NULL, update the state */
    if (! PG_ARGISNULL(1))
    {
        state->sum += abs(PG_GETARG_INT32(1));
        state->count++;
    }

    /* Restore the old memory context */
    MemoryContextSwitchTo(old_context);

    /* Return the state */
    PG_RETURN_POINTER(state);
}

/*
 * abs_avg aggregate final function. 
 */
Datum int32_abs_avg_final(PG_FUNCTION_ARGS)
{
    int32_abs_avg_state *state;

    state = (int32_abs_avg_state *)PG_GETARG_POINTER(0);

    if (state->count == 0)
        PG_RETURN_NULL();

    PG_RETURN_FLOAT8((double)state->sum / state->count);
}

