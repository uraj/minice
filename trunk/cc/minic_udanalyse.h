#ifndef __MINIC_UDANALYSE_H__
#define __MINIC_UDANALYSE_H__

#include "minic_flowanalyse.h"

//ud_kill is not used in our method

extern struct triargtable **table_list;

extern void var_list_sort(struct var_list * list_array);
extern void ud_analyse(struct basic_block * block_head);

#endif
