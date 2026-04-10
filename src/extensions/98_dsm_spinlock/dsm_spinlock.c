/*
 * This file implements a PostgreSQL extension that demonstrates the use of
 * dynamic shared memory (DSM) and spinlocks.
*/
#include "postgres.h"

#include "fmgr.h"
#include "funcapi.h"
#include "port.h"
#include "storage/dsm_registry.h"
#include "storage/spin.h"
#include "utils/builtins.h"
#include "utils/timestamp.h"

PG_MODULE_MAGIC;

#define SL_WAIT_SHM_NAME "SpinlockWaitSharedState"

/* Shared state information */
typedef struct
{
    slock_t spinlock;
} SpinlockWaitSharedState;

/* Pointer to state in shared memory */
static SpinlockWaitSharedState *sl_wait_state = NULL;

/* Function prototypes */
static void slw_init_state(void *ptr);
static void attach_shm(void);
static Datum make_spinlock_result(FunctionCallInfo fcinfo, long acquire_ms, long total_ms);

/* Exported functions */
PG_FUNCTION_INFO_V1(grab_spinlock);
Datum grab_spinlock(PG_FUNCTION_ARGS);


/* Init shared memory state */
static void
slw_init_state(void *ptr)
{
    SpinlockWaitSharedState *state = (SpinlockWaitSharedState *) ptr;
    SpinLockInit(&state->spinlock);
}

/* Attach to shared memory */
static void 
attach_shm(void)
{
    bool found;

    /* Check if we are already attached */
    if(sl_wait_state != NULL)
        return;

    /* If not attached, attach now and init state if needed */
    sl_wait_state = GetNamedDSMSegment(SL_WAIT_SHM_NAME,
        sizeof(SpinlockWaitSharedState),
        slw_init_state,
        &found);

    /* We should now have a state */
    Assert(sl_wait_state != NULL);
}

/*
 * Create a composite type result containing acquire_time and total_time as intervals.
 */
static Datum
make_spinlock_result(FunctionCallInfo fcinfo, int64 acquire_ms, int64 total_ms)
{
    Interval *acquire_interval;
    Interval *total_interval;
    Datum values[2];
    bool nulls[2] = {false, false};
    TupleDesc tupdesc;
    HeapTuple tuple;
    Datum result;

    acquire_interval = (Interval *) palloc(sizeof(Interval));
    total_interval = (Interval *) palloc(sizeof(Interval));

    acquire_interval->time = acquire_ms * 1000;
    acquire_interval->day = 0;
    acquire_interval->month = 0;

    total_interval->time = total_ms * 1000;
    total_interval->day = 0;
    total_interval->month = 0;

    /* The function should return a composite type */
    if (get_call_result_type(fcinfo, NULL, &tupdesc) != TYPEFUNC_COMPOSITE)
        ereport(ERROR,
                (errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
                 errmsg("function result type is not a composite type")));

    /* Build the result tuple */
    tupdesc = BlessTupleDesc(tupdesc);

    values[0] = PointerGetDatum(acquire_interval);
    values[1] = PointerGetDatum(total_interval);

    tuple = heap_form_tuple(tupdesc, values, nulls);
    result = HeapTupleGetDatum(tuple);

    return result;
}

/*
 * Function that grabs a spinlock, waits for a specified time,
 * and returns the acquire time and total time.
 */
Datum grab_spinlock(PG_FUNCTION_ARGS)
{
    int32 wait_time_ms;
    TimestampTz start_time;
    TimestampTz acquire_time;
    TimestampTz end_time;
    int64 acquire_ms;
    int64 total_ms;
    Datum result;

    wait_time_ms = PG_GETARG_INT32(0);

    if (wait_time_ms < 0)
        ereport(ERROR,
                (errcode(ERRCODE_DATA_EXCEPTION),
                 errmsg("wait time must be greater or equal than zero")));

    /* Make sure we are connected to the shared memory region */
    attach_shm();

    /* Measure the start time */
    start_time = GetCurrentTimestamp();
    Assert(sl_wait_state != NULL);

    /* Grab the spinlock... */
    SpinLockAcquire(&sl_wait_state->spinlock);
    acquire_time = GetCurrentTimestamp();
    elog(DEBUG1, "Spinlock acquired");

    /* ... and wait */
    pg_usleep((long) wait_time_ms * 1000L);

    /* Release the spinlock */
    SpinLockRelease(&sl_wait_state->spinlock);

    /* Measure the end time */
    end_time = GetCurrentTimestamp();
    acquire_ms = TimestampDifferenceMilliseconds(start_time, acquire_time);
    total_ms = TimestampDifferenceMilliseconds(start_time, end_time);

    /* Create the result tuple */
    result = make_spinlock_result(fcinfo, acquire_ms, total_ms);

    PG_RETURN_DATUM(result);
}
