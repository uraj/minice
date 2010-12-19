#ifndef __MINISIM_BPTREE_H__
#define __MINISIM_BPTREE_H__

#include <stdint.h>

/* this is a bst */
typedef struct
{
    uint32_t bp;
    void * left;
    void * right;
} BpTree;

extern int bp_del(BpTree ** p_bp_tree, uint32_t bp);
extern int bp_add(BpTree ** p_bp_tree, uint32_t bp);
extern int bp_search(BpTree * bp_tree, uint32_t bp);

#endif
