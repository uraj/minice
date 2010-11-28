#ifndef __MINIC_BASICBLOCK_H__
#define __MINIC_BASICBLOCK_H__

#include "minic_triargexpr.h"
#include "minic_triargtable.h"
#include "minic_varmapping.h"
#define SHOWBASICBLOCK	//show block

struct var_list
{
	int var_map_index;
	struct var_list * next;
};

struct basic_block
{
	int index;
	struct triargexpr_list * head;
	struct triargexpr_list * tail;
	struct basic_block_list * prev;
	struct basic_block_list * next;
};

struct basic_block_list//used for prev_list next_list and DFS_list
{
	struct basic_block * entity;
	struct basic_block_list * next;
};

extern int g_block_num;
extern struct triargtable ** table_list; 
extern struct basic_block ** DFS_array;

//extern void new_var_list();//use g_block_num 
//extern void free_var_list();
/*
//function to make Flow Diagram
extern void analyse_all_function();//analyse all funtion and store the result in fd_list
*/
extern struct basic_block * make_fd(int function_index);//make the flow diagram for a function
//function to analyse data flow
/*
extern void local_analysis();//can just use g_DFS_list
extern void	global_analysis();//use g_DFS_list
extern void active_var_analysis();//can just use g_DFS_list
*/
#endif
