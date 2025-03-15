#include "postgres.h"
#include "fmgr.h"
#include "funcapi.h"

#include "miscadmin.h"
#include "access/table.h"
#include "access/tableam.h"
#include "utils/builtins.h"
#include "utils/fmgroids.h"
#include "utils/lsyscache.h"
#include "utils/sortsupport.h"
#include "utils/syscache.h"
#include "utils/tuplesort.h"
#include "utils/typcache.h"

PG_MODULE_MAGIC;

/*
 * This function is used to print the attributes of a tuple.
 */
static void print_tuple(TupleTableSlot *slot, TupleDesc tupdesc)
{
    for (int i = 0; i < tupdesc->natts; i++)
    {
        bool isnull;
        Datum value = slot_getattr(slot, i + 1, &isnull);
        char *text_value;

        if (isnull)
            text_value = "null";
        else
        {
            Oid foutoid;
            bool typisvarlena;
            Oid typoid;

            /* lookup output func for the type */
            typoid = tupdesc->attrs[i].atttypid;
            getTypeOutputInfo(typoid, &foutoid, &typisvarlena);
            text_value = OidOutputFunctionCall(foutoid, value);
        }

        elog(INFO, "Attribute %d: %s", i + 1, text_value);
    }

    elog(INFO, "==========================");
}

PG_FUNCTION_INFO_V1(full_table_scan);

Datum full_table_scan(PG_FUNCTION_ARGS)
{
    Relation relation;
    TableScanDesc tablescandesc;
    TupleTableSlot *slot;
    TupleDesc tupdesc;
    Oid relid = PG_ARGISNULL(0) ? InvalidOid : PG_GETARG_OID(0);

    if (relid == InvalidOid)
        ereport(ERROR, (errmsg("invalid relation OID")));

    /* Open and lock the relation. */
    relation = table_open(relid, AccessShareLock);

    /* Build the tuple slot */
    slot = MakeSingleTupleTableSlot(RelationGetDescr(relation), table_slot_callbacks(relation));

    tupdesc = slot->tts_tupleDescriptor;

    /* Perform a full table scan */
    tablescandesc = table_beginscan(relation, GetTransactionSnapshot(), 0, NULL);

    /* Log the attribute names of the table */
    for (int i = 0; i < tupdesc->natts; i++)
    {
        char *attname = NameStr(relation->rd_att->attrs[i].attname);
        elog(INFO, "Attribute name %d: %s", i + 1, attname);
    }
    elog(INFO, "==========================");

    /* Iterate through the rows of the table */
    while (table_scan_getnextslot(tablescandesc, ForwardScanDirection, slot))
    {
        /* The slot should be filled at this point */
        Assert(!TTS_EMPTY(slot));

        /* Log the attributes of each row */
        print_tuple(slot, tupdesc);
    }

    /* Clean up */
    table_endscan(tablescandesc);
    ExecDropSingleTupleTableSlot(slot);
    table_close(relation, AccessShareLock);

    PG_RETURN_VOID();
}

PG_FUNCTION_INFO_V1(table_scan_with_scankeys);

Datum table_scan_with_scankeys(PG_FUNCTION_ARGS)
{
    Relation relation;
    TableScanDesc tablescandesc;
    TupleTableSlot *slot;
    TupleDesc tupdesc;
    ScanKeyData scanKeys[2];
    Oid relid = PG_ARGISNULL(0) ? InvalidOid : PG_GETARG_OID(0);

    if (relid == InvalidOid)
        ereport(ERROR, (errmsg("invalid relation OID")));

    /* Open and lock the relation. */
    relation = table_open(relid, AccessShareLock);

    /* Build the tuple slot */
    slot = MakeSingleTupleTableSlot(RelationGetDescr(relation), table_slot_callbacks(relation));

    tupdesc = slot->tts_tupleDescriptor;

    /* Initialize the ScanKey for the attributes */
    if (tupdesc->natts < 2)
        ereport(ERROR, (errmsg("relation has less than 2 attributes")));

    if (tupdesc->attrs[0].atttypid != INT4OID)
        ereport(ERROR, (errmsg("first attribute is not of type int4")));
    ScanKeyInit(&scanKeys[0], 1, InvalidStrategy, F_INT4GE, Int64GetDatum(2));

    if (tupdesc->attrs[1].atttypid != TEXTOID)
        ereport(ERROR, (errmsg("second attribute is not of type text")));
    ScanKeyInit(&scanKeys[1], 2, InvalidStrategy, F_TEXTEQ, PointerGetDatum(cstring_to_text("Bob")));

    /* Perform a full table scan */
    tablescandesc = table_beginscan(relation, GetTransactionSnapshot(), 2, scanKeys);

    /* Log the attribute names of the table */
    for (int i = 0; i < tupdesc->natts; i++)
    {
        char *attname = NameStr(relation->rd_att->attrs[i].attname);
        elog(INFO, "Attribute name %d: %s", i + 1, attname);
    }
    elog(INFO, "==========================");

    /* Iterate through the rows of the table */
    while (table_scan_getnextslot(tablescandesc, ForwardScanDirection, slot))
    {
        /* The slot should be filled at this point */
        Assert(!TTS_EMPTY(slot));

        /* Log the attributes of each row */
        print_tuple(slot, tupdesc);
    }

    /* Clean up */
    table_endscan(tablescandesc);
    ExecDropSingleTupleTableSlot(slot);
    table_close(relation, AccessShareLock);

    PG_RETURN_VOID();
}

PG_FUNCTION_INFO_V1(table_scan_with_index);

Datum table_scan_with_index(PG_FUNCTION_ARGS)
{
    Relation relation;
    Relation indexrelation;
    IndexScanDesc indexscandesc;
    TupleTableSlot *slot;
    TupleDesc tupdesc;
    ScanKeyData scanKeys[2];
    Oid relid = PG_ARGISNULL(0) ? InvalidOid : PG_GETARG_OID(0);
    Oid indexrelid = PG_ARGISNULL(1) ? InvalidOid : PG_GETARG_OID(1);

    if (relid == InvalidOid)
        ereport(ERROR, (errmsg("invalid relation OID")));

    if (indexrelid == InvalidOid)
        ereport(ERROR, (errmsg("invalid index relation OID")));

    /* Open and lock the relation. */
    relation = table_open(relid, AccessShareLock);

    /* Open and lock the index relation. */
    indexrelation = index_open(indexrelid, AccessShareLock);

    /* Build the tuple slot */
    slot = MakeSingleTupleTableSlot(RelationGetDescr(relation), table_slot_callbacks(relation));

    tupdesc = slot->tts_tupleDescriptor;

    /* Initialize the ScanKey for the attributes */
    if (IndexRelationGetNumberOfKeyAttributes(indexrelation) < 2)
        ereport(ERROR, (errmsg("index has less than 2 attributes")));

    if (indexrelation->rd_att->attrs[0].atttypid != INT4OID)
        ereport(ERROR, (errmsg("first attribute is not of type int4")));
    ScanKeyInit(&scanKeys[0], 1, BTGreaterEqualStrategyNumber, F_INT4GE, Int64GetDatum(2));

    if (indexrelation->rd_att->attrs[1].atttypid != TEXTOID)
        ereport(ERROR, (errmsg("second attribute is not of type text")));
    ScanKeyInit(&scanKeys[1], 2, BTEqualStrategyNumber, F_TEXTEQ, PointerGetDatum(cstring_to_text("Bob")));

    /* Perform a full index scan */
    indexscandesc = index_beginscan(relation, indexrelation, GetTransactionSnapshot(), 2, 0);
    index_rescan(indexscandesc, scanKeys, 2, NULL, 0);

    /* Log the attribute names of the table */
    for (int i = 0; i < tupdesc->natts; i++)
    {
        char *attname = NameStr(relation->rd_att->attrs[i].attname);
        elog(INFO, "Attribute name %d: %s", i + 1, attname);
    }
    elog(INFO, "==========================");

    /* Iterate through the rows of the index */
    while (index_getnext_slot(indexscandesc, ForwardScanDirection, slot))
    {
        /* The slot should be filled at this point */
        Assert(!TTS_EMPTY(slot));

        /* Log the attributes of each row */
        print_tuple(slot, tupdesc);
    }

    /* Clean up */
    index_endscan(indexscandesc);
    ExecDropSingleTupleTableSlot(slot);
    index_close(indexrelation, AccessShareLock);
    table_close(relation, AccessShareLock);

    PG_RETURN_VOID();
}

PG_FUNCTION_INFO_V1(table_scan_and_sort_attribute);

Datum table_scan_and_sort_attribute(PG_FUNCTION_ARGS)
{
    Oid relid = PG_ARGISNULL(0) ? InvalidOid : PG_GETARG_OID(0);
    text *sort_attr = PG_ARGISNULL(1) ? NULL : PG_GETARG_TEXT_P(1);
    char *sort_attr_str = text_to_cstring(sort_attr);
    TypeCacheEntry *sort_func;
    Relation relation;
    TableScanDesc tablescandesc;
    TupleTableSlot *slot;
    TupleTableSlot *sorted_slot;
    TupleDesc tupdesc;
    AttrNumber sort_attr_num[1];
    Oid sort_func_oid[1];
    Oid attr_type[1];
    Oid attr_collation[1];
    bool sort_nulls_first[1] = {false};
    Tuplesortstate *tuplesortstate;

    if (relid == InvalidOid)
        ereport(ERROR, (errmsg("invalid relation OID")));

    /* Open and lock the relation. */
    relation = table_open(relid, AccessShareLock);

    /* Find the attribute to sort */
    sort_attr_num[0] = get_attnum(relid, sort_attr_str);

    if (sort_attr_num[0] == InvalidAttrNumber)
        ereport(ERROR,
                (errcode(ERRCODE_UNDEFINED_COLUMN),
                 errmsg("relation \"%s\" does not have a column \"%s\"", get_rel_name(relid), sort_attr_str)));

    /* Build the tuple slot */
    slot = MakeSingleTupleTableSlot(RelationGetDescr(relation), table_slot_callbacks(relation));

    tupdesc = slot->tts_tupleDescriptor;

    /* Get the sort function for the attribute */
    attr_type[0] = TupleDescAttr(tupdesc, sort_attr_num[0] - 1)->atttypid;
    attr_collation[0] = TupleDescAttr(tupdesc, sort_attr_num[0] - 1)->attcollation;
    sort_func = lookup_type_cache(attr_type[0], TYPECACHE_GT_OPR);
    sort_func_oid[0] = sort_func->gt_opr;

    if (!OidIsValid(sort_func_oid[0]))
        ereport(ERROR,
                (errcode(ERRCODE_UNDEFINED_FUNCTION),
                 errmsg("no gt compare function available for attribute \"%s\"", sort_attr_str)));

    /* Initialize tuplesort state */
    tuplesortstate = tuplesort_begin_heap(tupdesc,
                                          1,                /* numSortCols */
                                          sort_attr_num,    /* sortColIdx */
                                          sort_func_oid,    /* sortOperators */
                                          attr_collation,   /* sortCollations */
                                          sort_nulls_first, /* sortNullsFirst */
                                          work_mem,         /* work_mem */
                                          NULL,             /* coordinate */
                                          TUPLESORT_NONE);  /* sortopt */

    /* Perform a full table scan */
    tablescandesc = table_beginscan(relation, GetTransactionSnapshot(), 0, NULL);

    /* Iterate through the rows of the table */
    while (table_scan_getnextslot(tablescandesc, ForwardScanDirection, slot))
    {
        /* The slot should be filled at this point */
        Assert(!TTS_EMPTY(slot));
        tuplesort_puttupleslot(tuplesortstate, slot);
    }
    ExecDropSingleTupleTableSlot(slot);
    slot = NULL;

    /* Sort the tuples */
    tuplesort_performsort(tuplesortstate);

    /* Log the sorted tuples */
    sorted_slot = MakeTupleTableSlot(tupdesc, &TTSOpsMinimalTuple);

    while (tuplesort_gettupleslot(tuplesortstate, true, false, sorted_slot, NULL))
    {
        Assert(!TTS_EMPTY(sorted_slot));
        print_tuple(sorted_slot, tupdesc);
    }

    /* Clean up */
    tuplesort_end(tuplesortstate);
    table_endscan(tablescandesc);
    ExecDropSingleTupleTableSlot(sorted_slot);
    table_close(relation, AccessShareLock);

    PG_RETURN_VOID();
}


PG_FUNCTION_INFO_V1(get_attribute_type);

Datum
get_attribute_type(PG_FUNCTION_ARGS)
{
    Oid relationOid = PG_GETARG_OID(0);
    char *attributeName = text_to_cstring(PG_GETARG_TEXT_P(1));
    HeapTuple attrTuple;
    Form_pg_attribute attform;
    Oid result;
    
    /* Lookup the type of the attribute */
    attrTuple = SearchSysCache2(ATTNAME, ObjectIdGetDatum(relationOid), CStringGetDatum(attributeName));
    if (!HeapTupleIsValid(attrTuple))
        PG_RETURN_NULL();

    /* Decode the tuple */
    attform = (Form_pg_attribute) GETSTRUCT(attrTuple);
    result = attform->atttypid;

    /* Release the cache entry */
    ReleaseSysCache(attrTuple);

    PG_RETURN_OID(result);
}