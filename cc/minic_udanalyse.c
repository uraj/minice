#include "minic_udanalyse.h"
#include "minic_varmapping.h"
#include <stdio.h>
#include <stdlib.h>

static int cur_var_id_num;
static struct var_list *** ud_in;
static struct var_list *** ud_out;
static struct var_list *** ud_gen;

static int set_cur_function(int function_index)
{
	cur_var_id_num = table_list[function_index] -> var_id_num;
}

static void new_temp_list()
{
	ud_in = malloc(sizeof(struct var_list **) * g_block_num);
	ud_out = malloc(sizeof(struct var_list **) * g_block_num);
	ud_gen = malloc(sizeof(struct var_list **) * g_block_num);
	int index;
	for(index = 0; index < g_block_num; index ++)
	{
		ud_in[index] = malloc(sizeof(struct var_list *) * cur_var_id_num);
		ud_out[index] = malloc(sizeof(struct var_list *) * cur_var_id_num);
		ud_gen[index] = malloc(sizeof(struct var_list *) * cur_var_id_num);
	}
}

static void free_temp_list()
{
	int index;
	for(index = 0; index < g_block_num; index ++)
	{
		free(ud_in[index]);
		free(ud_out[index]);
		free(ud_gen[index]);
	}
	free(ud_in);
	free(ud_out);
	free(ud_gen);
}

void ud_analyse(struct basic_block * block_head, int function_index)
{
	set_cur_function(function_index);//set the function	
	new_temp_list();

	free_temp_list();
}

