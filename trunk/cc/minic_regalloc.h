#ifndef __MINIC_REGALLOC_H__
#define __MINIC_REGALLOC_H__

#include "minic_varmapping.h"
#include "minic_flowanalyse.h"

struct ralloc_info
{
    int * result;
    int consume;
};

struct adjlist
{
    int tonode;
    struct adjlist * next;
};

/* allocate max_reg registers to a ver_set with card of n,  */
/* return the allocation resolution */
extern struct ralloc_info reg_alloc(struct var_list * vlist,
                                    int vlist_size,
                                    const int var_num,
                                    const int max_reg);

#endif
