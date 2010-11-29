#include "minic_udanalyse.h"
#include "minic_varmapping.h"
#include <stdio.h>
#include <stdlib.h>

static int cur_var_id_num;
static struct var_list *** ud_in;
static struct var_list *** ud_out;
static struct var_list *** ud_gen;
static int * local_var_id_flag;

static inline int set_cur_function(int function_index)
{
	cur_var_id_num = table_list[function_index] -> var_id_num;
}

static inline void new_temp_list()
{
	ud_in = malloc(sizeof(struct var_list **) * g_block_num);
	ud_out = malloc(sizeof(struct var_list **) * g_block_num);
	ud_gen = malloc(sizeof(struct var_list **) * g_block_num);
	local_var_id_flag = malloc(sizeof(int) * cur_var_id_num); 
	int index;
	for(index = 0; index < g_block_num; index ++)
	{
		ud_in[index] = malloc(sizeof(struct var_list *) * cur_var_id_num);
		ud_out[index] = malloc(sizeof(struct var_list *) * cur_var_id_num);
		ud_gen[index] = malloc(sizeof(struct var_list *) * cur_var_id_num);
	}
}

static inline void clear_local_var_id_flag()
{
	memset(local_var_id_flag, 0, sizeof(int) * cur_var_id_num);
}

static inline void free_temp_list()
{
	int index;
	for(index = 0; index < g_block_num; index ++)
	{
		free(ud_in[index]);
		free(ud_out[index]);
		free(ud_gen[index]);
	}
	free(local_var_id_flag);
	free(ud_in);
	free(ud_out);
	free(ud_gen);
}

static inline void generate_gen_for_each(struct basic_block * block)
{
	clear_local_var_id_flag();
	struct triargexpr_list * temp_node = block -> head;
	while(temp_node != NULL)
	{
		struct triargexpr * expr = temp_node -> entity;
		struct var_info * id_info;
		struct int index;
		switch(expr -> op)
		{
			case Assign://Nullop?
				if(expr -> arg1.type == IdArg)
				{
					index = get_index_of_id(expr -> arg1.idname);
					local_var_id_flag[index] = expr -> index;
				}

				
		temp_node = temp_node -> next;	
	}
}

static void generate_gen_for_all()
{
	int index;
	for(index = 0; index < g_block_num; index ++)
		generate_gen_for_each(DFS_array[index]);
}

void ud_analyse(int function_index)
{
	set_cur_function(function_index);//set the function	
	new_temp_list();

	free_temp_list();
}

