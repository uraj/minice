#ifndef __MINIC_UDANALYSE_H__
#define __MINIC_UDANALYSE_H__

#include "minic_flowanalyse.h"

//ud_kill is not used in our method

extern struct triargtable **table_list;
extern struct basic_block ** DFS_array;
extern int g_block_num;

extern void ud_analyse(int function_index);/* The DFS_array should have been filled */

#endif
