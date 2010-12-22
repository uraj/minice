#include "minic_varmapping.h"
#include "minic_symtable.h"
#include "minic_triargexpr.h"
#include "minic_basicblock.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static struct value_info * cur_func_info;//used only in varmap
static int cur_expr_num;//should not be changed later, because the exprnum of table may be changed
static int cur_var_id_num;
static int cur_fun_index;

static int * map_bridge;//connect index with var_info_index, the indexs are sequential. The length of the array is not sure.
static int map_bridge_bound;
static int map_bridge_cur_index;
static struct var_info ** var_info_table;//var_info contains some flags, name is tmperary, shall be modified later

static inline int set_cur_func(int func_index)
{
	if(func_index < g_table_list_size)//just in case
	{
		cur_func_info = symt_search(simb_table ,table_list[func_index] -> funcname);
		cur_expr_num = table_list[func_index] -> expr_num;
		cur_var_id_num = table_list[func_index] -> var_id_num;
		cur_func_index = func_index;
		return 1;
	}
	else 
	{
		printf("the index has overflow.");
		return 0;
	}
}

/* also need some functions for dynamic array ----- var_bridge*/
static inline void new_map_bridge()
{
    map_bridge =
        malloc(sizeof(int) * cur_var_id_num * 2);
    map_bridge_bound = cur_var_id_num * 2;
    map_bridge_cur_index = 0;
}

static inline void free_map_bridge()//free the global table list
{
    if(map_bridge != NULL)
        free(map_bridge);
}

static inline struct var_info *  new_var_info()
{
	struct var_info * new_info = malloc(sizeof(struct var_info));
	new_info -> is_define = -1;
	new_info -> is_use = -1;
	new_info -> reg_addr = -1;
	new_info -> mem_addr = INITIAL_MEM_ADDR;
	new_info -> label_num = -1; 
	new_info -> ref_point = NULL;
	return new_info;
}

int new_var_map(int func_index)
{
	if(!set_cur_func(func_index))
		return 0;
	var_info_table = malloc(sizeof(struct var_info *) * (cur_expr_num + cur_var_id_num));
	memset(var_info_table, 0, sizeof(struct var_info *) * (cur_expr_num + cur_var_id_num));
	int index = 0;
	while(index < cur_var_id_num)//malloc var info for id
    {
		var_info_table[index] = new_var_info();
        var_info_table[index++]->index = map_bridge_cur_index;
    }
	new_map_bridge();
	return 1;
}

void free_var_map()
{
	int i;
	free_map_bridge();
	for(i = 0; i < cur_expr_num + cur_var_id_num; i++)
	{
		if(var_info_table[i] != NULL)
			free(var_info_table[i]);
	}
	free(var_info_table);
}

int insert_tempvar(int exprindex)
{
	if(var_info_table[exprindex + cur_var_id_num] != NULL)
		return 0;
	var_info_table[exprindex + cur_var_id_num] = new_var_info();
	var_info_table[exprindex + cur_var_id_num] -> index = map_bridge_cur_index + cur_var_id_num;
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
    map_bridge[map_bridge_cur_index++] = exprindex + cur_var_id_num;
	//total_var_num ++;
	return 1;
}

int get_index_of_temp(int expr)
{
	if(var_info_table[cur_var_id_num + expr] == NULL)
		return -1;
	else
		return var_info_table[cur_var_id_num + expr] -> index;
}

int get_index_of_id(char * idname)
{
	struct value_info * id_info = symbol_search(simb_table, cur_func_info -> func_symt, idname);
	return id_info -> no;
}

struct var_info * get_info_of_temp(int expr)
{
	return get_info_from_index(get_index_of_temp(expr));
}

struct var_info * get_info_of_id(char * idname)
{
	return get_info_from_index(get_index_of_id(idname));
}

struct var_info * get_info_from_index(int index)
{
	if(index < 0 || index >= (map_bridge_cur_index + cur_var_id_num))
		return NULL;
	if(index < cur_var_id_num)
		return var_info_table[index];
	else
		return var_info_table[map_bridge[index - cur_var_id_num]];
}

struct int get_width_from_index(int index)
{
	if(index < 0 || index >= (map_bridge_cur_index + cur_var_id_num))
		return -1;
	if(index < cur_var_id_num)
	{
		struct value_info * g_value_info = get_valueinfo_byno(index);
		if(g_value_info -> type -> type == Char)
			return 1;
		else return 4;
	}
	else
	{
		if(var_info_table[map_bridge[index - cur_var_id_num]] == NULL)
			return -1;
		return table_list[cur_func_index] -> table[map_bridge[index - cur_var_id_num]].width; 
	}
}

void set_expr_label_mark(int exprnum)//only for label
{
	struct var_info * tmp_info = get_info_of_temp(exprnum);
	if(tmp_info == NULL)
	{
		var_info_table[exprnum + cur_var_id_num] = new_var_info();
		var_info_table[exprnum + cur_var_id_num] -> index = -1;
		var_info_table[exprnum + cur_var_id_num] -> label_num = -2;//-2 means is a jump dest
	}
	else tmp_info -> label_num = -2;
}

struct var_info * get_info_of_temp_for_label(int exprnum)//only for label
{
	return var_info_table[exprnum + cur_var_id_num];	
}

int is_global(int index)/* global or const str */
{
	return is_conststr_byno(index) || ((index < (get_globalvar_num())) && (index >= 0));
}

int get_ref_var_num()/* new and used, just mark */
{
	return map_bridge_cur_index + cur_var_id_num;
}
