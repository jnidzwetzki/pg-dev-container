/*
 * Custom scan proxy example
 *
 * Places a custom scan node on top of the regular scans (sequential scan,
 * index scan, ...) of tables with a 'credit_card_number' column. The node
 * forwards the tuples of the regular scan and masks all but the last four
 * digits of the number.
 *
 * This is a demo, not a security feature: COPY TO, UPDATE/DELETE, and
 * SELECT ... FOR UPDATE do not use the custom scan node.
 */
#include "postgres.h"
#include "fmgr.h"

#include "catalog/pg_class.h"
#include "catalog/pg_type.h"
#include "commands/explain_format.h"
#include "executor/executor.h"
#include "nodes/extensible.h"
#include "optimizer/optimizer.h"
#include "optimizer/pathnode.h"
#include "optimizer/paths.h"
#include "utils/builtins.h"
#include "utils/lsyscache.h"

PG_MODULE_MAGIC;

#define MASKED_COLUMN_NAME "credit_card_number"
#define UNMASKED_DIGITS 4

/* Previous set_rel_pathlist hook */
static set_rel_pathlist_hook_type prev_set_rel_pathlist_hook = NULL;

/* Function prototypes */
static void proxy_set_rel_pathlist(PlannerInfo *root, RelOptInfo *rel,
                                   Index rti, RangeTblEntry *rte);
static Plan *proxy_plan_custom_path(PlannerInfo *root, RelOptInfo *rel,
                                    CustomPath *best_path, List *tlist,
                                    List *clauses, List *custom_plans);
static Node *proxy_create_scan_state(CustomScan *cscan);
static void proxy_begin_scan(CustomScanState *node, EState *estate, int eflags);
static TupleTableSlot *proxy_exec_scan(CustomScanState *node);
static TupleTableSlot *proxy_next(ScanState *node);
static bool proxy_recheck(ScanState *node, TupleTableSlot *slot);
static void proxy_end_scan(CustomScanState *node);
static void proxy_rescan(CustomScanState *node);
static void proxy_explain_scan(CustomScanState *node, List *ancestors,
                               ExplainState *es);
static Datum mask_credit_card(Datum value);

static const CustomPathMethods proxy_path_methods = {
    .CustomName = "MaskProxy",
    .PlanCustomPath = proxy_plan_custom_path,
};

static const CustomScanMethods proxy_scan_methods = {
    .CustomName = "MaskProxy",
    .CreateCustomScanState = proxy_create_scan_state,
};

static const CustomExecMethods proxy_exec_methods = {
    .CustomName = "MaskProxy",
    .BeginCustomScan = proxy_begin_scan,
    .ExecCustomScan = proxy_exec_scan,
    .EndCustomScan = proxy_end_scan,
    .ReScanCustomScan = proxy_rescan,
    .ExplainCustomScan = proxy_explain_scan,
};

void _PG_init(void)
{
    /* Register our hook during module initialization */
    prev_set_rel_pathlist_hook = set_rel_pathlist_hook;
    set_rel_pathlist_hook = proxy_set_rel_pathlist;

    RegisterCustomScanMethods(&proxy_scan_methods);
}

/*
 * Put a custom path on top of each path of tables with a credit card number
 * column
 */
static void
proxy_set_rel_pathlist(PlannerInfo *root, RelOptInfo *rel, Index rti,
                       RangeTblEntry *rte)
{
    AttrNumber attnum;
    Oid atttype;
    List *subpaths;
    ListCell *lc;

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

    /* No unwrapped path must remain, it would return unmasked values */
    subpaths = rel->pathlist;
    rel->pathlist = NIL;
    rel->partial_pathlist = NIL;

    foreach (lc, subpaths)
    {
        Path *subpath = (Path *) lfirst(lc);
        CustomPath *cpath = makeNode(CustomPath);

        /* The proxy keeps the properties of the regular scan, e.g., the sort order */
        cpath->path.pathtype = T_CustomScan;
        cpath->path.parent = rel;
        cpath->path.pathtarget = subpath->pathtarget;
        cpath->path.param_info = subpath->param_info;
        cpath->path.parallel_aware = false;
        cpath->path.parallel_safe = false;
        cpath->path.rows = subpath->rows;
        cpath->path.disabled_nodes = subpath->disabled_nodes;
        cpath->path.startup_cost = subpath->startup_cost;
        cpath->path.total_cost = subpath->total_cost + subpath->rows * cpu_operator_cost;
        cpath->path.pathkeys = subpath->pathkeys;
        cpath->flags = 0;
        cpath->custom_paths = list_make1(subpath);
        cpath->custom_private = list_make1_int(attnum);
        cpath->methods = &proxy_path_methods;

        add_path(rel, &cpath->path);
    }
}

/*
 * Convert the CustomPath into a CustomScan plan node with the regular scan
 * as child
 */
static Plan *
proxy_plan_custom_path(PlannerInfo *root, RelOptInfo *rel,
                       CustomPath *best_path, List *tlist, List *clauses,
                       List *custom_plans)
{
    CustomScan *cscan = makeNode(CustomScan);
    Plan *child = linitial(custom_plans);
    AttrNumber attnum = linitial_int(best_path->custom_private);
    List *maskcols = NIL;
    ListCell *lc;

    /* Find the positions of the credit card number in the tuples of the child */
    foreach (lc, child->targetlist)
    {
        TargetEntry *tle = (TargetEntry *) lfirst(lc);
        Var *var = (Var *) tle->expr;

        if (IsA(var, Var) && var->varno == rel->relid && var->varattno == attnum)
            maskcols = lappend_int(maskcols, tle->resno);
    }

    /*
     * The node does not scan a relation itself (scanrelid = 0). It gets the
     * tuples from the child, custom_scan_tlist describes them. The quals are
     * evaluated by the child.
     */
    cscan->scan.plan.targetlist = tlist;
    cscan->scan.plan.qual = NIL;
    cscan->scan.scanrelid = 0;
    cscan->flags = best_path->flags;
    cscan->custom_plans = custom_plans;
    cscan->custom_private = maskcols;
    cscan->custom_scan_tlist = child->targetlist;
    cscan->methods = &proxy_scan_methods;

    return &cscan->scan.plan;
}

/*
 * Create the execution state
 */
static Node *
proxy_create_scan_state(CustomScan *cscan)
{
    CustomScanState *state;

    state = (CustomScanState *) newNode(sizeof(CustomScanState), T_CustomScanState);
    state->methods = &proxy_exec_methods;

    return (Node *) state;
}

/*
 * Initialize the child node
 */
static void
proxy_begin_scan(CustomScanState *node, EState *estate, int eflags)
{
    CustomScan *cscan = (CustomScan *) node->ss.ps.plan;
    PlanState *child;

    child = ExecInitNode(linitial(cscan->custom_plans), estate, eflags);
    node->custom_ps = list_make1(child);
}

/*
 * Return the next tuple
 */
static TupleTableSlot *
proxy_exec_scan(CustomScanState *node)
{
    return ExecScan(&node->ss, proxy_next, proxy_recheck);
}

/*
 * Fetch the next tuple from the child and mask the credit card number
 */
static TupleTableSlot *
proxy_next(ScanState *node)
{
    CustomScanState *css = (CustomScanState *) node;
    CustomScan *cscan = (CustomScan *) node->ps.plan;
    TupleTableSlot *scanslot = node->ss_ScanTupleSlot;
    TupleTableSlot *childslot;
    int natts = scanslot->tts_tupleDescriptor->natts;
    ListCell *lc;

    /* An empty slot indicates the end of the scan */
    ExecClearTuple(scanslot);
    childslot = ExecProcNode(linitial(css->custom_ps));
    if (TupIsNull(childslot))
        return scanslot;

    slot_getallattrs(childslot);
    memcpy(scanslot->tts_values, childslot->tts_values, natts * sizeof(Datum));
    memcpy(scanslot->tts_isnull, childslot->tts_isnull, natts * sizeof(bool));

    /* Allocate the masked values in the per-tuple memory context */
    foreach (lc, cscan->custom_private)
    {
        int maskidx = lfirst_int(lc) - 1;
        MemoryContext oldcontext;

        if (scanslot->tts_isnull[maskidx])
            continue;

        oldcontext = MemoryContextSwitchTo(node->ps.ps_ExprContext->ecxt_per_tuple_memory);
        scanslot->tts_values[maskidx] = mask_credit_card(scanslot->tts_values[maskidx]);
        MemoryContextSwitchTo(oldcontext);
    }

    return ExecStoreVirtualTuple(scanslot);
}

/*
 * EvalPlanQual recheck, not needed since queries with row marks are skipped
 */
static bool
proxy_recheck(ScanState *node, TupleTableSlot *slot)
{
    return true;
}

static void
proxy_end_scan(CustomScanState *node)
{
    ExecEndNode(linitial(node->custom_ps));
}

/*
 * Restart the child, the executor does not do this for custom_ps
 */
static void
proxy_rescan(CustomScanState *node)
{
    PlanState *child = linitial(node->custom_ps);

    /* Pass changed parameters (e.g., of a nested loop) to the child */
    if (node->ss.ps.chgParam != NULL)
        UpdateChangedParamSet(child, node->ss.ps.chgParam);

    /* With changed parameters, the next ExecProcNode() rescans the child */
    if (child->chgParam == NULL)
        ExecReScan(child);

    ExecScanReScan(&node->ss);
}

static void
proxy_explain_scan(CustomScanState *node, List *ancestors, ExplainState *es)
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
