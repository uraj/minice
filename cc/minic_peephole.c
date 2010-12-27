#include "minic_machcode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern struct mach_code_table * code_table_list;
static int cur_func_index;
static int cur_code_num;
static mach_code * cur_table;
static inline void set_cur_function(int func_index)
{
	cur_func_index = func_index;
	cur_code_num = code_table_list[func_index].code_num;
	cur_table = code_table_list[func_index].table;
}

static void instruction_scheduling()
{
	int index;
	for(index = 0; index < cur_code_num; index ++)
	{
		if(cur_table[index].op_type == MEM && (cur_table[index].mem_op_type == LDW || cur_table[index].mem_op_type == LDB))
	}
}

void peephole(int func_index)
{
	set_cur_function(func_index);
}
