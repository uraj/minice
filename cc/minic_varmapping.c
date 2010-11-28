#include "minic_varmapping.h"
#include "minic_symtable.h"
#include "minic_triargexpr.h"
#include "minic_basicblock.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static struct value_info * cur_func_info;//used only in varmap
static int cur_expr_num;

static inline int set_cur_func(int func_index)
{
	if(func_index < g_table_list_size)//just in case
	{
		cur_func_info = symt_search(simb_table ,table_list[func_index] -> funcname);
		cur_expr_num = table_list[func_index] -> expr_num;
		return 1;
	}
	else 
	{
		printf("the index has overflow.");
		return 0;
	}
}

int get_index_of_temp(int expr)
{
	if(var_info_table[g_var_id_num + expr] == NULL)
		return -1;
	else
		return var_info_table[g_var_id_num + expr] -> index;
}

int get_index_of_id(char * idname)
{
	struct value_info * id_info = symt_search(cur_func_info -> func_symt, idname);
	return id_info -> no;
}

struct var_info * get_info_from_index(int index)
{
	if(index < g_var_id_num)
		return var_info_table[index];
	else
		return var_info_table[map_bridge[cur_expr_num - g_var_id_num]];
}

int new_var_map(int func_index)
{
	if(!set_cur_func(func_index))
		return 0;
	var_info_table = malloc(sizeof(struct var_info *) * (cur_expr_num + g_var_id_num));
	return 1;
}

void free_var_map()
{
	int i;
	for(i = 0; i < cur_expr_num + g_var_id_num; i++)
	{
		if(var_info_table[i] != NULL)
			free(var_info_table[i]);
	}
	free(var_info_table);
}

/* also need some functions for dynamic array ----- var_bridge*/
void new_map_bridge()
{
    map_bridge =
        malloc(sizeof(int) * INITIALTEMPVARSIZE);
    map_bridge_bound = INITIALTEMPVARSIZE;
    map_bridge_cur_index = 0;
}

int insert_tempvar(int exprindex)
{
	if(var_info_table[exprindex + g_var_id_num] != NULL)
		return 0;
	var_info_table[exprindex + g_var_id_num] = malloc(sizeof(struct var_info));
	var_info_table[exprindex + g_var_id_num] -> index = map_bridge_cur_index;
	//malloc the flags in the var_info_table
    if(map_bridge_cur_index >= map_bridge_bound)
    {
        int * tmp_map_bridge =
            malloc(sizeof(int) * map_bridge_bound * 2);
        memcpy(tmp_map_bridge, map_bridge,
               sizeof(int) * map_bridge_bound); 
        map_bridge_bound *= 2;
        free(map_bridge);
        map_bridge = tmp_map_bridge;
    }
    map_bridge[map_bridge_cur_index++] = exprindex + g_var_id_num;
	//total_var_num ++;
	return 1;
}

void free_map_bridge()//free the global table list
{
    if(map_bridge != NULL)
        free(map_bridge);
}
