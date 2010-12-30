#ifndef __MINIC_ALIASANALYSE_H__
#define __MINIC_ALIASANALYSE_H__
#define POINTER_OPTIMIZE
extern int g_block_num;
extern struct triargtable ** table_list;
extern struct basic_block ** DFS_array;  

extern void pointer_analyse(int function_index);

#endif
