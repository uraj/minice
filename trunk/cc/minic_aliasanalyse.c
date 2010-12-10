#include "minic_udanalyse.h"
#include "minic_varmapping.h"
#include <stdio.h>
#include <stdlib.h>

static int cur_var_id_num;
static struct var_list *** pointer_in;
static struct var_list *** pointer_out;
static struct value_info * cur_func_info;

struct entity_type
{
	int index;
	char ispointer;//-1 flush tag; 0 not pointer; 1 is pointer; 2 is array
};

static inline int set_cur_function(int function_index)
{
	cur_var_id_num = table_list[function_index] -> var_id_num;
	cur_func_info = symt_search(simb_table ,table_list[func_index] -> funcname);
}

static void new_temp_list()
{
	pointer_in = malloc(sizeof(struct var_list **) * g_block_num);
	pointer_out = malloc(sizeof(struct var_list **) * g_block_num);
	int index;
	for(index = 0; index < cur_var_id_num; index ++)
	{
		pointer_in[index] = malloc(sizeof(struct var_list *));
		pointer_out[index] = malloc(sizeof(struct var_list *));
		pointer_in[index] = NULL;
		pointer_out[index] = NULL;
	}
}

static void free_temp_list()
{
	int index;
	for(index = 0; index < g_block_num; index ++)
	{
		var_list_free(pointer_in[index]);
		var_list_free(pointer_out[index]);
	}
	free(pointer_in);
	free(pointer_out);
}

static void pointer_list_merge(struct var_list ** adder, struct var_list ** dest)
{
	int index;
	for(index = 0; index < cur_var_id_num; index ++)
		dest[index] = var_list_merge(adder[index], dest[index]);
}

static void pointer_list_replace(struct var_list ** dest, int elem, struct var_list * new_list)
{
	if(dest[elem] != NULL)
		var_list_free(dest[elem]);
	dest[elem] = var_list_copy(new_list);
}

static int pointer_list_is_equal(struct var_list ** first_list, struct var_list ** second_list)
{
	int index;
	for(index = 0; index < cur_var_id_num; index ++)
	{
		if(var_list_isequal(first_list[index], second_list[index]) != 1)
			return 0;
	}
	return 1;
}

/*
   modify pointer_in first, then move pointer_in to pointer_out, then update pointer_in
*/

static void trans_in_to_out(struct basic_block * block)//update
{
	struct triargexpr_list * temp_node;
	int index = block -> index;
	temp_node = block -> head;
	while(temp_node != NULL)
	{
		struct triargexpr * expr = temp_node -> entity;
		if(expr -> op == Assign)
		{
			if(expr -> arg1.type == IdArg)
			{
				struct value_info * arg1_info, arg2_info;
				int arg1_index, arg2_index;
				arg1_info = symbol_search(simb_table, cur_func_info -> func_symt, expr -> arg1.idname);
				arg1_index = arg1_info -> no;
				if(arg1_info -> type -> type == Pointer)
				{
					switch(expr.arg2.type)
					{
						case IdArg:
							arg2_info = symbol_search(simb_table, cur_func_info -> func_symt, expr -> arg2.idname);
							arg2_index = arg2_info -> no;
							pointer_list_replace(pointer_out[index], arg1_index, pointer_out[arg2_index]);//should use out current now
							break;
						case ExprArg:
							arg2_index = arg2_info -> no
							break;
					}
				}
			}
		}
		temp_node = temp_node -> next;
	}
}

/*
   there won't be more than one & once a time, and there the pointer level is less than once.
*/

/*
   some of the -1 is returned when the program is wrong, but most of -1 tells us to cancel the pointer
*/

static entity_type search_entity(int exprnum, int ispointer)
{
	struct triargexpr expr = table_list[exprnum];
	struct value_info * temp_info;
	struct entity_type entity;
	switch(expr.op)
	{
		case Ref:
			if(expr.arg1.type == IdArg)
			{
				temp_info = symbol_search(simb_table, cur_func_info -> func_symt, expr.arg1.idname);
				entity.index = temp_info -> no;
				switch(ispointer)
				{
					case -1://solved in type check
						if(temp_info -> type -> type == Pointer)
							entity.ispointer = 1;
						else if(temp_info -> type -> type == Array)
							entity.ispointer = 2;
						else entity.ispointer = -1;
						return entity;
					case 0:
						if(temp_info -> type -> type != Pointer && temp_info -> type -> type != Array)
							entity.ispointer = 0;
						else
							entity.ispointer = -1;
						return entity;
				}
			}
			if(expr.arg1.type == ExprArg)//ispointer == 1 has been checked
				return search_entity(expr.arg1.expr, ispointer + 1);
			//there won't be more than one & * once a time
			break;

		case Deref:
			if(expr.arg1.type == IdArg)
			{
				temp_info = symbol_search(simb_table, cur_func_info -> func_symt, expr.arg1.idname);
				entity.index = temp_info -> no;
				switch(ispointer)
				{
					case 0:
						entity.ispointer = -1;
						return entity;
					case 1:
						if(temp_info -> type -> type == Pointer)
							entity.ispointer = 1;
						else if(temp_info -> type -> type == Array)
							entity.ispointer = 2;
						else
							entity.ispointer = -1;
						return entity;
				}
			}
			if(expr.arg1.type == ExprArg)
				return search_entity(expr.arg1.expr, ispointer + 1);
			break;
	
		case Subscript://The arg of subscript can only be an ident
			if(expr.arg1.type == IdArg)//just in case
			{
				temp_info = symbol_search(simb_table, cur_func_info -> func_symt, expr.arg1.idname);
				entity.index = temp_info -> no;
				switch(ispointer)//if the id is not suitable, the program has been wrong before
				{
					case 0:
						entity.ispointer = -1;
						return entity;
					case 1:
						if(temp_info -> type -> type == Pointer)
							entity.ispointer = -1;
						else entity.ispointer = 2;//is other type, then has been wrong
						return entity;
				}
				// other wrong case has been checked before
			}
			break;

		case Plusplus:
		case Minusminus:
			if(expr.arg1.type == IdArg)
			{
				temp_info = symbol_search(simb_table, cur_func_info -> func_symt, expr.arg1.idname);
				entity.index = temp_info -> no;
				switch(ispointer)
				{
					case -1://only pointer or array
						entity.ispointer = -1;
						return entity;
					case 0://Pointer ++ also return -1
						if(temp_info -> type -> type == Array)
							entity.ispointer = 2;
						else entity.ispointer = -1;
				}
			}
			if(expr.arg1.type == ExprArg)
			{
				entity = search_entity(expr.arg1.expr, ispointer);
				if(entity.ispointer != Array)
					entity.ispointer = -1;
				return entity;
			}
			break;

		case Minus:
			if(expr.arg1.type == IdArg)
			{
				temp_info = symbol_search(simb_table, cur_func_info -> func_symt, expr.arg1.idname);
				entity.index = temp_info -> no;	
				switch(ispointer)
				{
					case -1:
						entity.ispointer = -1;
						return entity;
					case 0://Pointer + or Pointer - also return -1
						if( temp_info -> type -> type == Array)
							entity.ispointer = 2;
						else
							entity.ispointer = -1;
						return entity;
				}
			}
			if(expr.arg1.type == ExprArg)
			{
				entity = search_entity(expr.arg1.expr, ispointer);
				if(entity.ispointer != Array)
					entity.ispointer = -1;
				return entity;
			}
			break;
			
		case Plus://there should be only one entity
			if(expr.arg1.type == IdArg)
			{
				temp_info = symbol_searc(simb_table, cur_func_info -> func_symt, expr.arg1.idname);
				entity.index = temp_info -> no;	
				switch(ispointer)
				{
					case -1:
						if(temp_info -> type -> type == Pointer || temp_info -> type -> type == Array)
						{
							entity.ispointer = -1;
							return entity;
						}
						break;
					case 0:
						if( temp_info -> type -> type == Array)
						{
							entity.ispointer = 2;
							return entity;
						}
						else if(temp_info -> type -> type == Pointer)
						{
							entity.ispointer = -1;
							return entity;
						}
						break;
				}
			}
			if(expr.arg2.type == IdArg)
			{
				temp_info = symbol_search(simb_table, cur_func_info -> func_symt, expr.arg2.idname);
				entity.index = temp_info -> no;	
				switch(ispointer)
				{
					case -1://can only be pointer
						return -1;
					case 0://Pointer + or - also return -1
						if(temp_info -> type -> type == Array)
							return temp_info -> no;
						else
							return -1;
				}
			}
			int tmp_result;
			if(expr.arg1.type == ExprArg)
			{
				tmp_result = search_entity(expr.arg1.expr, ispointer);
				if(tmp_result != -1)
					return tmp_result;
			}
			if(expr.arg2.type == ExprArg)//number is better than -1
				return search_entity(expr.arg2.expr, ispointer);
			break;	

		default://if there are some other op, the arg can't be Pointer and there won't be & before the expr, just return -1
			return -1;
	}
	/* if the program has already been wrong, anything returned is meaningless, so just return -1 */
	return -1;
}

static void generate_in_out_for_all()
{
	int index;
	//All in and out are empty at first

	int change = 1;
	while(change)
	{
		change = 0;
		for(index = 0; index < g_block_num; index++)
		{
			struct basic_block_list * list_node = DFS_array[index] -> prev;
			while(list_node != NULL)
			{
				pointer_in[index] = pointer_list_merge(pointer_out[list_node -> entity-> index], pointer_in[index]);
				list_node = list_node -> next;
			}

			struct pointer_list * newout;
			newout = def_point_list_sub_merge(ud_in[index], ud_gen[index]);
			if(def_point_list_is_equal(newout, ud_out[index]) != 1)
			{
				change = 1;
				def_point_list_free(ud_out[index]);
				ud_out[index] = newout;//see what the function is next
			}
		}
	}
}

void pointer_analyse(int funcindex)
{
	set_cur_function(funcindex);
	new_temp_list();
	free_temp_list();
}

