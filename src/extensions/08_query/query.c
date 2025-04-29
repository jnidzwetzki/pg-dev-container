#include "postgres.h"
#include "fmgr.h"

#include "tcop/utility.h"
#include "parser/analyze.h"
#include "nodes/nodeFuncs.h"

PG_MODULE_MAGIC;

/* Previous post-parse analysis hook */
static post_parse_analyze_hook_type prev_post_parse_analyze_hook = NULL;

/* Function prototypes */
static void query_post_parse_analyze(ParseState *pstate, Query *query, JumbleState *jstat);
static bool print_node_walker(Node *node, void *context);

void _PG_init(void)
{
    /* Register our hook during module initialization */
    prev_post_parse_analyze_hook = post_parse_analyze_hook;
    post_parse_analyze_hook = query_post_parse_analyze;
}


/*
 * Post-parse analysis hook to print the query statement
 */
static void
query_post_parse_analyze(ParseState *pstate, Query *query, JumbleState *jstat)
{
    if (query != NULL)
        print_node_walker((Node *)query, NULL);

    /* Call the previous hook if exists */
    if (prev_post_parse_analyze_hook)
        prev_post_parse_analyze_hook(pstate, query, jstat);
}

/*
 * Callback function for the tree walker
 * Prints information about each node in the query tree
 */
static bool
print_node_walker(Node *node, void *context)
{
    if (node == NULL)
        return false;

    /* Print node tag and address */
    elog(INFO, "Found node type: %d", (int)nodeTag(node));

    switch (nodeTag(node))
    {
    case T_Query:
    {
        elog(INFO, "Query node found");
        return query_tree_walker((Query *)node, print_node_walker, context, 0);
        break;
    }
    case T_RangeTblEntry:
    {
        RangeTblEntry *rte = (RangeTblEntry *)node;
        if (rte->rtekind == RTE_RELATION)
            elog(INFO, "  RTE: relation %u", rte->relid);
        else if (rte->rtekind == RTE_SUBQUERY)
            elog(INFO, "  RTE: subquery");
        else if (rte->rtekind == RTE_FUNCTION)
            elog(INFO, "  RTE: function");
        else if (rte->rtekind == RTE_VALUES)
            elog(INFO, "  RTE: values");
        break;
    }
    case T_RangeTblRef:
    {
        RangeTblRef *rtr = (RangeTblRef *)node;
        elog(INFO, "RangeTblRef: rtindex=%d", rtr->rtindex);
        break;
    }
    case T_TargetEntry:
    {
        TargetEntry *te = (TargetEntry *)node;
        elog(INFO, "TargetEntry: resno=%d, name=%s",
             te->resno, te->resname ? te->resname : "(null)");
        break;
    }
    default:
        break;
    }

    /* Continue walking the tree */
    return expression_tree_walker(node, print_node_walker, context);
}

/*
 * Dummy function that takes no arguments and returns void
 */
PG_FUNCTION_INFO_V1(dummy_function);
Datum dummy_function(PG_FUNCTION_ARGS)
{
    elog(NOTICE, "Dummy function called");
    PG_RETURN_VOID();
}