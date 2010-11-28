#include "minic_regalloc.h"
#include <stdlib.h>
#include <memory.h>

typedef struct
{
    int key;
    int val;
} isort_elem;

/* get interference graph from data flow statisitc, */
/* return adjacent matrix */
char ** interfer_graph_m()
{
    return NULL;
}

/* get interference graph from data flow statisitc, */
/* return adjacent list */
struct adjlist * interfer_graph_l()
{
    return NULL;
}

/* return a new set of n variables */
/* set implemented by adj matrix */
static char ** new_interfer_graph_m(int n)
{
    if(n <= 0)
        return NULL;
    int i;
    char ** ret;
    ret = malloc((n - 1) * sizeof(char *));
    ret[0] = NULL;
    for(i = n - 1; i > 0; --i)
        ret[i] = calloc(sizeof(char), i);
    return ret;
}

static void free_interfer_graph_m(char ** igmatrix, int n)
{
    int i;
    for(i = 1; i < n; ++i)
        free(igmatrix[i]);
    return;
}

static int isort_elem_cmp(const void * elem1, const void * elem2)
{
    return
        ((isort_elem *)elem1) -> key -
        ((isort_elem *)elem2) -> key;
}

static void get_degree(const char ** var_set, int n, isort_elem * elem)
{
    int i, j;
    for(i = 0; i < n; ++i)
    {
        elem[i].val = i;
        for(j = 0; j < i; ++j)
            elem[i].key += var_set[i][j];
        for(j = i + i; j < nv; ++j)
            elem[i].key += var_set[j][i];
    }
    return;
}

static void delete_node(char ** var_set, int n,int node)
{
    int i;
    memset(var_set[node], 0, sizeof(char) * i);
    for(i = node + 1; i < n; ++i)
        var_set[i][node] = 0;
    return;
}

int color_interfer_graph(struct adjlist ** ig, int * alloc_info,
                         const int max_reg, int * stack, const int top)
{
    if(max_reg > top)
        exit(1);
    int i, j;
    char * color = malloc(sizeof(char) * max_reg);
    struct adjlist * temp = NULL;
    for(i = 0; i < top; ++i)
    {
        temp = ig[stack[i]];
        memset(color, 0, sizeof(char) * max_reg);
        while(temp != NULL)
        {
            if(alloc_info[temp -> tonode] != 0)
                color[temp -> tonode] = 1;
            temp = temp -> next;
        }
        for(j = 0; j < max_reg && color[j] != 0; ++j)
            ;
        alloc_info[stack[i]] = j;
    }
    return 0;
}

static int check(struct adjlist ** ig, int * alloc_info, int n)
{
    int i;
    struct adjlist * temp = NULL;
    for(i = 0; i < n; ++i)
    {
        if(alloc_info[i] == -1)
            continue;
        temp = ig[i];
        while(temp)
        {
            if(alloc_info[temp->tonode] == alloc_info[i])
                return 0;
            temp = temp -> next;
        }
    }
    return 1;
}

struct ralloc_info reg_alloc(char ** igmatrix,
                             struct adjlist ** iglist,
                             const int n, const int max_reg)
{
    struct ralloc_info ret;
    ret.result = calloc(sizeof(int), n);
    int * statck = malloc(n * sizeof(int));
    isort_elem * elem = malloc(n * sizeof(isort_elem));
    int top = 0, left = n, i, j;
    while(left > 0)
    {
        int spill = -1;
        char sflag = 1;         /* whether spill or not */
        int deg_tmp = 0;
        get_degree((const char **)igmatrix, n, elem);
        qsort(elem, n, sizeof(isort_elem), isort_elem_cmp);
        for(i = 0; i < n; ++i)
        {
            if(ret.result[i] != 0)     /* i already allocated */
                continue;
            if(elem[i].val < max_reg)
            {
                statck[top++] = i;     /* i can be placed in a reg */
                delete_node(igmatrix, n, i);
                sflag = 0;
                --left;
            }
            else                /* record the candidate var to be spilled */
            {
                                /* spill policy, maybe need a better one */
                if(deg_tmp < elem[i].val)
                {
                    spill = i;
                    deg_tmp = elem[i].val;
                }
                break;
            }
        }
        if(sflag)               /* spill the var with max degree */
        {
            delete_node(igmatrix, n, spill);
            ret.result[i] = -1;
            --left;
        }
    }
    ret.consume = color_interfer_graph(iglist, ret.result, max_reg, statck, top);
    /* debug */
    if(!check(iglist, ret.result, n))
        printf("wrong answer\n");
    /* debug end */
    return ret;
}
