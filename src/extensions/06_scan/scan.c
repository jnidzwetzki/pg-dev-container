#include "postgres.h"
#include "fmgr.h"
#include "funcapi.h"

#include "access/table.h"
#include "access/tableam.h"
#include "utils/builtins.h"
#include "utils/lsyscache.h"
#include "utils/fmgroids.h"

PG_MODULE_MAGIC;

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

    /* Clean up */
    index_endscan(indexscandesc);
    ExecDropSingleTupleTableSlot(slot);
    index_close(indexrelation, AccessShareLock);
    table_close(relation, AccessShareLock);

    PG_RETURN_VOID();
}
