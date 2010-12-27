#include "minic_peephole.h"
#include "minic_machineode.h"
#include "minic_asmout.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct mach_code_list
{
	struct mach_code * entity;
	struct mach_code_list * next, *prev;
};

static struct mach_code_list * head;

static int cur_func_index;
static int cur_code_num;
static struct mach_code * cur_table;

static int window_size;
static char reg_mark[32];

static int success;

static struct mach_code_list * new_code_node(struct mach_code * entity)
{
	struct mach_code_list * new_node = calloc(1, sizeof(struct mach_code_list));
	new_node -> entity = entity;
	new_node -> next = NULL;
	new_node -> prev = NULL;
}

static void gen_code_list()
{
	struct mach_code_list * old_node, * new_node;
	for(index = 1; index < cur_code_num; index ++)
	{
		if(index == 0)
		{
			head = new_code_node(&cur_table[index]);
			old_node = head;
			continue;
		}
		new_node = new_code_node(&cur_table[index]);
		old_node -> next = new_node;
		new_node -> prev = old_node;
		old_node = new_node;
	}
}

static void free_code_list()
{
	struct mach_code_list * tmp = head;
	while(head != NULL)
	{
		head = tmp -> next;
		free(tmp);
	}
	head = NULL;
}

static inline void set_cur_function(int func_index)
{
	cur_func_index = func_index;
	cur_code_num = code_table_list[func_index].code_num;
	cur_table = code_table_list[func_index].table;
	head = NULL;
	gen_code_list();
	window_size = 20;//MARK TAOTAOTHERIPPER. can be modified
	success = 0;
}

static int is_reg_disabled(int regnum)
{
	if(regnum < 4 || regnum == 16 || regnum == FP || regnum >= SP)
		return 1;
	return 0;
}

static int check_all_reg_used()
{
	int reg_index;
	for(reg_index = 0; reg_index < 32; reg_index ++)
	{
		if(is_reg_disabled(reg_index))
			continue;
		if(reg_mark[reg_index] != 1)
			return 0;
	}
	return 1;
}

static struct mach_code_list * search_inst(struct mach_code_list * cur)
{
	memset(reg_mark, 0, 32 * sizeof(char));
	if(cur -> entity.arg1.type == Mach_Reg)
		reg_mark[cur -> entity.arg1.reg] = 1;
	if(cur -> entity.arg1.type == Mach_Reg)
		reg_mark[cur -> entity.arg2.reg] = 1;
	cur = cur -> prev;
	int step = 0;
	while(cur != NULL)
	{
		if(step == window_size)
			return NULL;
		if(cur -> entity.op_type == BRANCH || cur -> entity.op_type == LABEL)//MARK TAOTAOTHERIPPER, cmp must be above the branch
			return NULL;
		if(check_all_reg_used())
			return NULL;
		if(reg_mark[cur -> entity.dest] != 1)
			return cur;
		if(cur -> entity.arg1.type == Mach_Reg)
			reg_mark[cur -> entity.arg1.reg] = 1;
		if(cur -> entity.arg2.type == Mach_Reg)
			reg_mark[cur -> entity.arg2.reg] = 1;
		step ++;
	}
	return NULL;
}

static void instruction_scheduling()
{
	int index;
	struct mach_code_list * last = NULL, * insert = NULL, * cur = head;
	while(cur != NULL)
	{
		if(cur -> entity.op_type == MEM 
				&& (cur -> entity.mem_op_type == LDW || cur -> entity.mem_op_type == LDB))
		{
			if(cur -> next != NULL)
			{
				if((cur -> next -> entity.arg1.type == Mach_Reg && cur -> entity.dest) 
						|| (cur -> next -> entity.arg2.type == Mach_Reg && cur -> entity.dest))
				{
					insert = search_inst(cur);
					if(insert != NULL)
					{
						success ++;
						insert -> prev -> next = insert -> next;
						insert -> next -> prev = insert -> prev;
						insert -> next = cur -> next;
						cur -> next -> prev = insert;
						cur -> next = insert;
						insert -> prev = cur;
					}
				}
			}
		}
		last = cur;
		cur = cur -> next;
	}
}

static void print_code_list(FILE out_buf)
{
	struct mach_code_list * tmp = head;
	while(tmp != NULL)
	{
		mcode_out(tmp -> entity, out_buf);
		tmp = tmp -> next;
	}
}

void peephole(int func_index)
{
	set_cur_function(func_index);
	instruction_scheduling();
	print_code_list(stdout);
	printf("scheduling success:%d\n", success);
	free_code_list();
}
