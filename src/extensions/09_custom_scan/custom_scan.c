/*
 * Custom scan node example
 *
 * Replaces the scan of tables with a 'credit_card_number' column by a custom
 * scan node that masks all but the last four digits of the number.
 *
 * This is a demo, not a security feature: COPY TO, UPDATE/DELETE, and
 * SELECT ... FOR UPDATE do not use the custom scan node.
 */
#include "postgres.h"
#include "fmgr.h"

#include "access/tableam.h"
#include "catalog/pg_class.h"
#include "catalog/pg_type.h"
#include "commands/explain_format.h"
#include "executor/executor.h"
#include "nodes/extensible.h"
#include "optimizer/optimizer.h"
#include "optimizer/pathnode.h"
#include "optimizer/paths.h"
#include "optimizer/restrictinfo.h"
#include "utils/builtins.h"
#include "utils/lsyscache.h"

PG_MODULE_MAGIC;

#define MASKED_COLUMN_NAME "credit_card_number"
#define UNMASKED_DIGITS 4

/* Execution state, CustomScanState must be the first field */
typedef struct MaskScanState
{
    CustomScanState css;
    TableScanDesc scandesc;
    TupleTableSlot *tableslot;
    AttrNumber attnum;
} MaskScanState;

/* Previous set_rel_pathlist hook */
static set_rel_pathlist_hook_type prev_set_rel_pathlist_hook = NULL;

/* Function prototypes */
static void mask_set_rel_pathlist(PlannerInfo *root, RelOptInfo *rel,
                                  Index rti, RangeTblEntry *rte);
static Plan *mask_plan_custom_path(PlannerInfo *root, RelOptInfo *rel,
                                   CustomPath *best_path, List *tlist,
                                   List *clauses, List *custom_plans);
static Node *mask_create_scan_state(CustomScan *cscan);
static void mask_begin_scan(CustomScanState *node, EState *estate, int eflags);
static TupleTableSlot *mask_exec_scan(CustomScanState *node);
static TupleTableSlot *mask_next(ScanState *node);
static bool mask_recheck(ScanState *node, TupleTableSlot *slot);
static void mask_end_scan(CustomScanState *node);
static void mask_rescan(CustomScanState *node);
static void mask_explain_scan(CustomScanState *node, List *ancestors,
                              ExplainState *es);
static Datum mask_credit_card(Datum value);

static const CustomPathMethods mask_path_methods = {
    .CustomName = "MaskScan",
    .PlanCustomPath = mask_plan_custom_path,
};

static const CustomScanMethods mask_scan_methods = {
    .CustomName = "MaskScan",
    .CreateCustomScanState = mask_create_scan_state,
};

static const CustomExecMethods mask_exec_methods = {
    .CustomName = "MaskScan",
    .BeginCustomScan = mask_begin_scan,
    .ExecCustomScan = mask_exec_scan,
    .EndCustomScan = mask_end_scan,
    .ReScanCustomScan = mask_rescan,
    .ExplainCustomScan = mask_explain_scan,
};

void _PG_init(void)
{
    /* Register our hook during module initialization */
    prev_set_rel_pathlist_hook = set_rel_pathlist_hook;
    set_rel_pathlist_hook = mask_set_rel_pathlist;

    RegisterCustomScanMethods(&mask_scan_methods);
}

/*
 * Replace the paths of tables with a credit card number column
 */
static void
mask_set_rel_pathlist(PlannerInfo *root, RelOptInfo *rel, Index rti,
                      RangeTblEntry *rte)
{
    AttrNumber attnum;
    Oid atttype;
    Path *seqpath;
    CustomPath *cpath;

    /* Call the previous hook if exists */
    if (prev_set_rel_pathlist_hook)
        prev_set_rel_pathlist_hook(root, rel, rti, rte);

    if (rte->rtekind != RTE_RELATION || rte->relkind != RELKIND_RELATION ||
        rte->inh || rte->tablesample != NULL || IS_DUMMY_REL(rel))
        return;

    /* Only read-only queries */
    if (root->parse->commandType != CMD_SELECT || root->parse->rowMarks != NIL)
        return;

    attnum = get_attnum(rte->relid, MASKED_COLUMN_NAME);
    if (attnum == InvalidAttrNumber)
        return;

    atttype = get_atttype(rte->relid, attnum);
    if (atttype != TEXTOID && atttype != VARCHAROID)
        return;

    /* Costs of a sequential scan plus the masking of each tuple */
    seqpath = create_seqscan_path(root, rel, rel->lateral_relids, 0);

    cpath = makeNode(CustomPath);
    cpath->path.pathtype = T_CustomScan;
    cpath->path.parent = rel;
    cpath->path.pathtarget = rel->reltarget;
    cpath->path.param_info = seqpath->param_info;
    cpath->path.parallel_aware = false;
    cpath->path.parallel_safe = false;
    cpath->path.rows = seqpath->rows;
    cpath->path.startup_cost = seqpath->startup_cost;
    cpath->path.total_cost = seqpath->total_cost + rel->tuples * cpu_operator_cost;
    cpath->path.pathkeys = NIL;
    cpath->flags = 0;
    cpath->methods = &mask_path_methods;
    cpath->custom_private = list_make1_int(attnum);

    /* Remove all other paths, they would return unmasked values */
    rel->pathlist = NIL;
    rel->partial_pathlist = NIL;
    add_path(rel, &cpath->path);
}

/*
 * Convert the CustomPath into a CustomScan plan node
 */
static Plan *
mask_plan_custom_path(PlannerInfo *root, RelOptInfo *rel,
                      CustomPath *best_path, List *tlist, List *clauses,
                      List *custom_plans)
{
    CustomScan *cscan = makeNode(CustomScan);

    cscan->scan.plan.targetlist = tlist;
    cscan->scan.plan.qual = extract_actual_clauses(clauses, false);
    cscan->scan.scanrelid = rel->relid;
    cscan->flags = best_path->flags;
    cscan->custom_private = best_path->custom_private;
    cscan->custom_scan_tlist = NIL;
    cscan->methods = &mask_scan_methods;

    return &cscan->scan.plan;
}

/*
 * Create the execution state
 */
static Node *
mask_create_scan_state(CustomScan *cscan)
{
    MaskScanState *state;

    state = (MaskScanState *) newNode(sizeof(MaskScanState), T_CustomScanState);
    state->css.methods = &mask_exec_methods;

    return (Node *) state;
}

/*
 * Initialize the scan, the table is already opened by the executor
 */
static void
mask_begin_scan(CustomScanState *node, EState *estate, int eflags)
{
    MaskScanState *state = (MaskScanState *) node;
    CustomScan *cscan = (CustomScan *) node->ss.ps.plan;

    state->attnum = linitial_int(cscan->custom_private);
    state->tableslot = table_slot_create(node->ss.ss_currentRelation,
                                         &estate->es_tupleTable);
    state->scandesc = NULL;
}

/*
 * Return the next tuple, ExecScan() evaluates the quals on the masked tuple
 */
static TupleTableSlot *
mask_exec_scan(CustomScanState *node)
{
    return ExecScan(&node->ss, mask_next, mask_recheck);
}

/*
 * Fetch the next tuple and mask the credit card number
 */
static TupleTableSlot *
mask_next(ScanState *node)
{
    MaskScanState *state = (MaskScanState *) node;
    TupleTableSlot *scanslot = node->ss_ScanTupleSlot;
    TupleTableSlot *tableslot = state->tableslot;
    int natts = scanslot->tts_tupleDescriptor->natts;
    int maskidx = state->attnum - 1;

    if (state->scandesc == NULL)
        state->scandesc = table_beginscan(node->ss_currentRelation,
                                          node->ps.state->es_snapshot, 0, NULL);

    /* An empty slot indicates the end of the scan */
    ExecClearTuple(scanslot);
    if (!table_scan_getnextslot(state->scandesc, ForwardScanDirection, tableslot))
        return scanslot;

    slot_getallattrs(tableslot);
    memcpy(scanslot->tts_values, tableslot->tts_values, natts * sizeof(Datum));
    memcpy(scanslot->tts_isnull, tableslot->tts_isnull, natts * sizeof(bool));

    /* Allocate the masked value in the per-tuple memory context */
    if (!scanslot->tts_isnull[maskidx])
    {
        MemoryContext oldcontext;

        oldcontext = MemoryContextSwitchTo(node->ps.ps_ExprContext->ecxt_per_tuple_memory);
        scanslot->tts_values[maskidx] = mask_credit_card(scanslot->tts_values[maskidx]);
        MemoryContextSwitchTo(oldcontext);
    }

    /* Keep ctid and tableoid */
    scanslot->tts_tid = tableslot->tts_tid;
    scanslot->tts_tableOid = tableslot->tts_tableOid;

    return ExecStoreVirtualTuple(scanslot);
}

/*
 * EvalPlanQual recheck, not needed since queries with row marks are skipped
 */
static bool
mask_recheck(ScanState *node, TupleTableSlot *slot)
{
    return true;
}

static void
mask_end_scan(CustomScanState *node)
{
    MaskScanState *state = (MaskScanState *) node;

    if (state->scandesc != NULL)
        table_endscan(state->scandesc);
}

static void
mask_rescan(CustomScanState *node)
{
    MaskScanState *state = (MaskScanState *) node;

    if (state->scandesc != NULL)
        table_rescan(state->scandesc, NULL);

    ExecScanReScan(&node->ss);
}

static void
mask_explain_scan(CustomScanState *node, List *ancestors, ExplainState *es)
{
    ExplainPropertyText("Masked Column", MASKED_COLUMN_NAME, es);
}

/*
 * Replace all but the last UNMASKED_DIGITS digits with an X
 */
static Datum
mask_credit_card(Datum value)
{
    char *str = TextDatumGetCString(value);
    int digits = 0;
    char *c;

    for (c = str; *c != '\0'; c++)
    {
        if (*c >= '0' && *c <= '9')
            digits++;
    }

    for (c = str; *c != '\0' && digits > UNMASKED_DIGITS; c++)
    {
        if (*c >= '0' && *c <= '9')
        {
            *c = 'X';
            digits--;
        }
    }

    return PointerGetDatum(cstring_to_text(str));
}
