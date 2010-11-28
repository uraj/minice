#ifndef __MINIC_REGALLOC_H__
#define __MINIC_REGALLOC_H__

#include "minic_varmapping.h"

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
extern struct ralloc_info reg_alloc(char ** igmatrix,
                                    struct adjlist ** iglist,
                                    const int n,
                                    const int max_reg);

#endif
