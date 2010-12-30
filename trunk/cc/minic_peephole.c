#include "minic_peephole.h"
#include "minic_machinecode.h"
#include "minic_asmout.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#define LARGEWINDOW
#define MORE_OPTIMIZE

struct mach_code_list
{
	struct mach_code * entity;
	struct mach_code_list * next, * prev;
};

static struct mach_code_list * head;

static struct mach_code * cur_table;
static int cur_func_index;
static int cur_code_num;

static int window_size;
static char des_reg_mark[32];
static char src_reg_mark[32];

static int scheduling_success;
static int merge_success;

static struct mach_code_list * new_code_node(struct mach_code * entity)
{
	struct mach_code_list * new_node = calloc(1, sizeof(struct mach_code_list));
	new_node -> entity = entity;
	new_node -> next = NULL;
	new_node -> prev = NULL;
	return new_node;
}

static void gen_code_list()
{
	struct mach_code_list * old_node, * new_node;
	int index;
	for(index = 0; index < cur_code_num; index ++)
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
	while(tmp != NULL)
	{
		head = tmp;
		tmp = tmp -> next;
		free(head);
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
	scheduling_success = 0;
	merge_success = 0;
}

#ifdef LARGEWINDOW
static int check_all_reg_used()
{
	int reg_index;
	for(reg_index = 0; reg_index < 31; reg_index ++)/* every register is possible except PC */
	{
		if(src_reg_mark[reg_index] != 1)
			return 0;
		if(des_reg_mark[reg_index] != 1)
			return 0;
	}
	return 1;
}
#endif
/*
   we can prove that the label won't affet the instruction scheduling
   here is the situation the label appear
	1. pointer
		lod r1, .L0
		mov r0, r1	//this situation may be optimized to lod r0, .L0.
					//if that occurs, because the pointer never changed
					//so the the result is still right
	2. store global var
		lod r1, .L0
		stw r0, [r1]
	3. load global var
		lod r1, .L0
		lod r1, [r1]
	when .L0 appear it must be conflict with the next sentence
*/

static struct mach_code_list * search_inst(struct mach_code_list * cur)
{
	memset(des_reg_mark, 0, 32 * sizeof(char));
	memset(src_reg_mark, 0, 32 * sizeof(char));

	if(cur -> entity -> arg1.type == Mach_Reg)
		src_reg_mark[cur -> entity -> arg1.reg] = 1;
	if(cur -> entity -> arg1.type == Mach_Reg)
		src_reg_mark[cur -> entity -> arg2.reg] = 1;
	if(cur -> next -> entity -> arg1.type == Mach_Reg)/* cur's next won't be NULL here */
		src_reg_mark[cur -> next -> entity -> arg1.reg] = 1;
	if(cur -> next -> entity -> arg2.type == Mach_Reg)
		src_reg_mark[cur -> next -> entity -> arg2.reg] = 1;

	if(cur -> next -> entity -> op_type == MEM && 
			(cur -> next -> entity -> mem_op == STW || cur -> next -> entity -> mem_op == STB))/* special for next instrution is store */
		src_reg_mark[cur -> next -> entity -> dest] = 1;

	des_reg_mark[cur -> entity -> dest] = 1;
	if(cur -> entity -> indexed == PREW || cur -> entity -> indexed == POST)
		des_reg_mark[cur -> entity -> arg1.reg] = 1;/* must be reg here */

	cur = cur -> prev;
	int step = 0;
	while(cur != NULL)
	{
		if(step == window_size)
			return NULL;
#ifdef LARGEWINDOW
		if(check_all_reg_used())
			return NULL;
#endif
		switch(cur -> entity -> op_type)/* deal with normal arg later, deal with dest and special arg in switch */
		{

			case BRANCH:
			case LABEL:
			case CMP://MARK TAOTAOTHERIPPER, cmp must be above the branch
				return NULL;
			case MEM:
				{
					switch(cur -> entity -> mem_op)
					{
						case LDW:
						case LDB:
							if((cur -> entity -> arg1.type != Mach_Reg || des_reg_mark[cur -> entity -> arg1.reg] != 1)
									&& (cur -> entity -> arg2.type != Mach_Reg || des_reg_mark[cur -> entity -> arg2.reg] != 1))
							{
								if(src_reg_mark[cur -> entity -> dest] != 1)
								{
									if(cur -> entity -> indexed != PREW && cur -> entity -> indexed != POST)
										return cur;
									if(src_reg_mark[cur -> entity -> arg1.reg] != 1)/* must in reg here */
										return cur;
									else
										des_reg_mark[cur -> entity -> arg1.reg] = 1;/* special dest */
								}
								else
									des_reg_mark[cur -> entity -> dest] = 1;/* special dest */
							}
							break;
						case STW:
						case STB:
							//return NULL;
#if 0
							if(des_reg_mark[cur -> entity -> dest] != 1
									&& (cur -> entity -> arg1.type != Mach_Reg || des_reg_mark[cur -> entity -> arg1.reg] != 1)
									&& (cur -> entity -> arg2.type != Mach_Reg || des_reg_mark[cur -> entity -> arg2.reg] != 1))
								if(cur -> entity -> indexed != PREW && cur -> entity -> indexed != POST)
									return cur;
								if(src_reg_mark[cur -> entity -> arg1.reg] != 1)/* must in reg here */
									return cur;
								else
#endif
									if(cur -> entity -> indexed == PREW || cur -> entity -> indexed == POST)
										des_reg_mark[cur -> entity -> arg1.reg] = 1;/* special dest */
								src_reg_mark[cur -> entity -> dest] = 1;/* special src */
							break;
						default:
							fprintf(stderr, "error mem type in instruction scheduling.\n");
							exit(1);
					}
					break;
				}
			case DP:
				{
					if((cur -> entity -> arg1.type != Mach_Reg || des_reg_mark[cur -> entity -> arg1.reg] != 1)
							&& (cur -> entity -> arg2.type != Mach_Reg || des_reg_mark[cur -> entity -> arg2.reg] != 1))
					{
						if(src_reg_mark[cur -> entity -> dest] != 1)
							return cur;
						else des_reg_mark[cur -> entity -> dest]  = 1;
					}
					break;
				}
			default:	
				fprintf(stderr, "error op type in instruction scheduling.\n");
				break;
		}
		if(cur -> entity -> arg1.type == Mach_Reg)
			src_reg_mark[cur -> entity -> arg1.reg] = 1;
		if(cur -> entity -> arg2.type == Mach_Reg)
			src_reg_mark[cur -> entity -> arg2.reg] = 1;	
		cur = cur -> prev;
		step ++;
	}
	return NULL;
}

static void instruction_scheduling()
{
	struct mach_code_list * last = NULL, * insert = NULL, * cur = head;
	while(cur != NULL)
	{
		if(cur -> entity -> op_type == MEM 
				&& (cur -> entity -> mem_op == LDW || cur -> entity -> mem_op == LDB))
		{
			if(cur -> next != NULL)
			{
				if((cur -> next -> entity -> arg1.type == Mach_Reg 
							&& cur -> next -> entity -> arg1.reg == cur -> entity -> dest) 
						|| (cur -> next -> entity -> arg2.type == Mach_Reg 
							&& cur -> next -> entity -> arg2.reg == cur -> entity -> dest)
						|| (cur -> next -> entity -> op_type == MEM 
							&& (cur -> next -> entity -> mem_op == STW 
								|| cur -> next -> entity -> mem_op == STB)
							&& cur -> next -> entity -> dest == cur -> entity -> dest))
				{
					insert = search_inst(cur);
					if(insert != NULL)
					{
						scheduling_success ++;
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

static void merge_binary_operation()
{
	struct mach_code_list * unused = NULL, * cur = head;
	while(cur != NULL)
	{
		/* if there is this kind of op the dest will occupy the reg 
		   for only one one clock, so that won't affect register 
		   allocation too much */
		if(cur -> entity -> optimize && cur -> next != NULL)/* the one which can be optimized must have dest reg */
		{
			if(cur -> next -> entity -> op_type == DP 
					&& cur -> next -> entity -> dp_op == MOV 
					&& cur -> next -> entity -> arg2.type == Mach_Reg 
					&& cur -> next -> entity -> arg2.reg == cur -> entity -> dest)
			{
				cur -> entity -> dest = cur -> next -> entity -> dest;
				cur -> next -> next -> prev = cur;
				unused = cur -> next;
				cur -> next = cur -> next -> next;
				free(unused);
				merge_success ++;
			}
		}
		cur = cur -> next;
	}
}

static void print_code_list(FILE * out_buf)
{
	struct mach_code_list * tmp = head;
	while(tmp != NULL)
	{
		mcode_out(tmp -> entity, out_buf);
		tmp = tmp -> next;
	}
}

void peephole(int func_index, FILE * out_buf)
{
	set_cur_function(func_index);
	int former_success;
	merge_binary_operation();
	do
	{
		former_success = scheduling_success;	
		instruction_scheduling();
	}while(former_success != scheduling_success);
	print_code_list(out_buf);
	//printf("scheduling success:%d\n", scheduling_success);
	//printf("merge success:%d\n", merge_success);
	free_code_list();
}
