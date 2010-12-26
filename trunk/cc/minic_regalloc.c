#include "minic_regalloc.h"
#include <stdlib.h>
#include <memory.h>
#include <stdio.h>

typedef struct
{
    int key;
    int val;
} isort_elem;

/* return a new set of n variables */
/* set implemented by adj matrix */
static char ** new_interfer_graph_m(int n)
{
    if(n <= 0)
        return NULL;
    int i;
    char ** ret;
    ret = malloc(n * sizeof(char *));
    ret[0] = NULL;
    for(i = n - 1; i > 0; --i)
        ret[i] = calloc(sizeof(char), i);
    return ret;
}

/* get interference graph from data flow statisitc, */
/* return adjacent matrix */
static char ** igraph_m_construct(struct var_list * vlist, int size, int var_num, char * appear)
{
    char ** igraph_m = new_interfer_graph_m(var_num);
    int i;
    for(i = 0; i < size; ++i)
    {
        if(vlist[i].head == NULL || vlist[i].head == vlist[i].tail)
            continue;
        struct var_list_node * ofocus = vlist[i].head;
        appear[ofocus->var_map_index] = 1;
        do
        {
            struct var_list_node * ifocus = ofocus;
            do
            {
                ifocus = ifocus->next;
                igraph_m[ifocus->var_map_index][ofocus->var_map_index] = 1;
            }while(ifocus != vlist[i].tail);
            ofocus = ofocus->next;
        }
        while(ofocus != vlist[i].tail);     
    }
    return igraph_m;
}

/* get interference graph from data flow statisitc, */
/* return adjacent list */
static struct adjlist ** igraph_l_construct(char ** igraph_m, int var_num)
{
    struct adjlist ** igraph_l = malloc(sizeof(struct adjlist *) * var_num);
    int i, j;
    for(i = 0; i < var_num; ++i)
    {
        struct adjlist ** focus = &igraph_l[i];
        *focus = NULL;
        for(j = 0; j < i; ++j)
            if(igraph_m[i][j] == 1)
            {
                *focus = malloc(sizeof(struct adjlist));
                (*focus)->tonode = j;
                focus = &((*focus)->next);
                *focus = NULL;
            }
        for(j = i + 1; j < var_num; ++j)
            if(igraph_m[j][i] == 1)
            {
                *focus = malloc(sizeof(struct adjlist));
                (*focus)->tonode = j;
                focus = &((*focus)->next);
                *focus = NULL;
            }
    }
    return igraph_l;
}

static inline void free_igraph_m(char ** igmatrix, int n)
{
    int i;
    for(i = 1; i < n; ++i)
        free(igmatrix[i]);
    return;
}


static void free_igraph_l_util(struct adjlist * list)
{
    if(list == NULL)
        return;
    free(list->next);
    free(list);
    return;
}

static void free_igraph_l(struct adjlist ** igraph_l, int n)
{
    int i;
    for(i = 0; i < n; ++i)
    {
        free_igraph_l_util(igraph_l[i]);
    }
    return;
}


static int isort_elem_cmp(const void * elem1, const void * elem2)
{
    return
        ((isort_elem *)elem2) -> key -
        ((isort_elem *)elem1) -> key;
}

static void get_degree(const char ** var_set, int n, isort_elem * elem)
{
    int i, j;
    for(i = 0; i < n; ++i)
    {
        elem[i].val = i;
        elem[i].key = 0;
        for(j = 0; j < i; ++j)
            elem[i].key += var_set[i][j];
        for(j = i + 1; j < n; ++j)
            elem[i].key += var_set[j][i];
    }
    return;
}

static void delete_node(char ** var_set, int n, int node)
{
    int i;
    memset(var_set[node], 0, sizeof(char) * node);
    for(i = 0; i < node; ++i)
        var_set[node][i] = 0;
    for(i = node + 1; i < n; ++i)
        var_set[i][node] = 0;
    return;
}

int color_interfer_graph(struct adjlist ** ig, int * alloc_info, const int max_reg, int * stack, const int top)
{
    int i, j, ret = 0;
    char * color = malloc(sizeof(char) * max_reg);
    char * used = calloc(sizeof(char), max_reg);
    struct adjlist * temp = NULL;
    for(i = 0; i < top; ++i)
    {
        temp = ig[stack[i]];
        memset(color, 0, sizeof(char) * max_reg);
        while(temp != NULL)
        {
            if(alloc_info[temp->tonode] != 0 &&
               alloc_info[temp->tonode] != -1)
                color[alloc_info[temp->tonode] - 1] = 1;
            temp = temp -> next;
        }
        for(j = 1; j <= max_reg && color[j - 1] != 0; ++j)
            ;
        alloc_info[stack[i]] = j;
        used[j - 1] = 1;
    }
    free(color);
    for(i = 0; i < max_reg; ++i)
        ret += used[i];
    free(used);
    return ret;
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

static struct ralloc_info reg_alloc_core(char ** igmatrix, struct adjlist ** iglist, const int n, const int max_reg, char * appear)
{
    struct ralloc_info ret;
    ret.result = calloc(sizeof(int), n);
    int * statck = malloc(n * sizeof(int));
    isort_elem * elem = malloc(n * sizeof(isort_elem));
    int top = 0, left = n, i;
    for(i = 0; i < n; ++i)
        if(appear[i] == 0)
        {
            ret.result[i] = -1;
            left -= 1;
        }
    while(left > 0)
    {
        get_degree((const char **)igmatrix, n, elem);
        qsort(elem, n, sizeof(isort_elem), isort_elem_cmp);
        for(i = 0; i < n; ++i)
        {
            if(ret.result[elem[i].val] != 0)     /* i already allocated */
                continue;
            if(elem[i].key < max_reg)
            {
                statck[top++] = elem[i].val;     /* i can be placed in a reg */
                delete_node(igmatrix, n, i);
                --left;
            }
            else                /* spill the var with max degree */
            {
                /* spill policy, maybe need a better one */
                delete_node(igmatrix, n, elem[i].val);
                (ret.result)[elem[i].val] = -1;
                --left;
                break;
            }
        }
    }
    ret.consume = color_interfer_graph(iglist, ret.result, max_reg, statck, top);
    /* debug */
    
    if(!check(iglist, ret.result, n))
        printf("wrong answer\n");
    /* debug end */
    return ret;
}

struct ralloc_info reg_alloc(struct var_list * vlist, int vlist_size, int var_num, const int max_reg)
{
    char * appear = calloc(sizeof(char), var_num);
    
    char ** igraph_m = igraph_m_construct(vlist, vlist_size, var_num, appear);
    struct adjlist ** igraph_l = igraph_l_construct(igraph_m, var_num);
    struct ralloc_info ret;
    ret = reg_alloc_core(igraph_m, igraph_l, var_num, max_reg, appear);
    free(appear);
    free_igraph_m(igraph_m, var_num);
    free_igraph_l(igraph_l, var_num);

    /* struct ralloc_info ret; */
    /* ret.result = malloc(var_num * sizeof(int)); */
    /* int i; */
    /* for(i = 0; i < var_num; ++i) */
    /*     ret.result[i] = -1; */
    /* ret.consume = 0; */
    
    return ret;
}

void pm(char ** m, int var_num)
{
    int i, j;
    for(i = 0; i < var_num; ++i)
    {
        for(j = 0; j < i; ++j)
            printf("%d ", (int)m[i][j]);
        printf("\n");
    }
}
