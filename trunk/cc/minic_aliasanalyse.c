#include "minic_flowanalyse.h"
#include "minic_aliasanalyse.h"
#include "minic_varmapping.h"
#include "minic_symtable.h"
#include <stdio.h>
#include <stdlib.h>
#define POINTER_DEBUG	/*Use to debug in out*/
//#define FUNC_DEBUG
//#define TRANS_DEBUG		/*Use to debug trans*/	//almost right
//#define ENTITY_DEBUG	/*Use to debug entity list*/
static int cur_var_id_num;
static struct var_list *** pointer_in;
static struct var_list *** pointer_out;
static struct value_info * cur_func_info;
static struct var_list ** tmp_out;
static struct triargexpr *cur_expr_table;

struct entity_type//only needed in this file
{
	int index;
	char ispointer;//-1 flush tag; 0 not pointer; 1 is pointer; 2 is array; 3 is modified pointer
};

static void print_list(struct var_list ** old_list)
{
	int index;
	for(index = 0; index < cur_var_id_num; index++)
	{
		printf("Var %d:", index);
		var_list_print(old_list[index]);
	}
	printf("\n");
}

static inline void set_cur_function(int function_index)
{
	cur_var_id_num = table_list[function_index] -> var_id_num;
	cur_func_info = symt_search(simb_table ,table_list[function_index] -> funcname);
	cur_expr_table = table_list[function_index] -> table;
}

static void new_tmp_out()
{
	tmp_out = calloc(cur_var_id_num, sizeof(struct var_list *));
	int index;
	for(index = 0; index < cur_var_id_num; index++)
		tmp_out[index] = var_list_new();
}

static void free_tmp_out()
{
	int index;
	for(index = 0; index < cur_var_id_num; index ++)
		var_list_free(tmp_out[index]);
	free(tmp_out);
}

static void new_temp_list()
{
	pointer_in = malloc(sizeof(struct var_list **) * g_block_num);
	pointer_out = malloc(sizeof(struct var_list **) * g_block_num);
	int block_index, var_index;
	for(block_index = 0; block_index < g_block_num; block_index ++)
	{
		pointer_in[block_index] = calloc(cur_var_id_num, sizeof(struct var_list *));
		pointer_out[block_index] = calloc(cur_var_id_num, sizeof(struct var_list *));
		for(var_index = 0; var_index < cur_var_id_num; var_index ++)
		{
			pointer_in[block_index][var_index] = var_list_new();
			pointer_out[block_index][var_index] = var_list_new();
		}
	}
}

static void free_temp_list()
{
	int index;
	for(index = 0; index < g_block_num; index ++)
	{
		int var_index;
		for(var_index = 0; var_index < cur_var_id_num; var_index ++)
		{
			if(pointer_in[index][var_index] != NULL)
				var_list_free(pointer_in[index][var_index]);
			if(pointer_out[index][var_index] != NULL)
				var_list_free(pointer_out[index][var_index]);
		}
		free(pointer_in[index]);
		free(pointer_out[index]);
	}
	free(pointer_in);
	free(pointer_out);
}

static void pointer_list_clear(struct var_list ** oldone)
{
	int index;
	for(index = 0; index < cur_var_id_num; index ++)
	{
		if(oldone[index] != NULL)
			var_list_clear(oldone[index]);
	}
}

static void pointer_list_copy(struct var_list ** newone, struct var_list ** oldone)//the newone can't be NULL
{
	int index;
	for(index = 0; index < cur_var_id_num; index++)
	{
		if(newone[index] != NULL)
			var_list_clear(newone[index]);
		newone[index] = var_list_copy(oldone[index], newone[index]);
	}
}

static void pointer_list_move(struct var_list ** newone, struct var_list ** oldone)//the newone can't be NULL
{
	int index;
	for(index = 0; index < cur_var_id_num; index ++)
	{
		if(newone[index] != NULL)
			var_list_free(newone[index]);
		newone[index] = oldone[index];
		oldone[index] = NULL;
	}
}

static void pointer_list_merge(struct var_list ** adder, struct var_list ** dest)//the dest can't be NULL
{
	int index;
	for(index = 0; index < cur_var_id_num; index ++)
		dest[index] = var_list_merge(adder[index], dest[index]);
}

static void pointer_list_rplc(struct var_list ** dest, int elem, int newelem)
{
	if(dest[elem] != NULL)
		var_list_clear(dest[elem]);
	if(newelem == -1)
		return;
	dest[elem] = var_list_copy(dest[newelem], dest[elem]);
}

static void pointer_list_rplc_entity(struct var_list ** dest, int elem, int entity)
{
	if(dest[elem] != NULL)
		var_list_clear(dest[elem]);
	dest[elem] = var_list_append(dest[elem], entity);
}

static struct var_list * var_list_copy_array(struct var_list *source , struct var_list *dest)
{
	if(source->head == NULL)
		return dest;
	struct var_list_node *temp = source -> head;
	while(temp != source->tail)
	{
		struct value_info * tmp_info = get_valueinfo_byno(cur_func_info -> func_symt, temp -> var_map_index);//must be id in this file
		if(tmp_info -> type -> type == Array)
			var_list_append(dest , temp->var_map_index);
		temp = temp->next;
	}
	var_list_append(dest , temp->var_map_index);
	return dest;
}

static void pointer_list_rplc_modptr(struct var_list ** dest, int elem, int newelem)
{
	if(dest[elem] != NULL)
		var_list_clear(dest[elem]);
	dest[elem] = var_list_copy_array(dest[newelem], dest[elem]);//in fact should only copy array member
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
   there won't be more than one & once a time, and there the pointer level is less than once.
*/

/*
   some of the -1 is returned when the program is wrong, but most of -1 tells us to cancel the pointer
*/

static struct entity_type search_entity(int exprnum, int ispointer)
{
	struct triargexpr expr = cur_expr_table[exprnum];
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
				return search_entity(expr.arg1.expr, ispointer - 1);
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
							entity.ispointer = 3;//modified pointer
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
						if(temp_info -> type -> type == Pointer)
							entity.ispointer = 3;//modified pointer
						else entity.ispointer = -1;
				}
			}
			if(expr.arg1.type == ExprArg)
			{
				entity = search_entity(expr.arg1.expr, ispointer);
				if(entity.ispointer == 1)
				{
					entity.ispointer = 3;//modified pointer
					return entity;
				}
				else if(entity.ispointer == 3)//if array => wrong
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
						else if(temp_info -> type -> type == Pointer)
							entity.ispointer = 3;//modified pointer
						else
							entity.ispointer = -1;
						return entity;
				}
			}
			if(expr.arg1.type == ExprArg)
			{
				entity = search_entity(expr.arg1.expr, ispointer);
				if(entity.ispointer == 1)
				{
					entity.ispointer = 3;
					return entity;
				}
				else if(entity.ispointer >= 2)//modified pointer or array
					return entity;			
			}
			break;
			
		case Plus://there should be only one entity
			if(expr.arg1.type == IdArg)
			{
				temp_info = symbol_search(simb_table, cur_func_info -> func_symt, expr.arg1.idname);
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
							entity.ispointer = 3;//modified pointer
							return entity;
						}
						break;
				}
			}
			if(expr.arg2.type == IdArg)//if arg1 and arg2 are all Id, will return -1 at the end
			{
				temp_info = symbol_search(simb_table, cur_func_info -> func_symt, expr.arg2.idname);
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
					case 0://Pointer + or - also return -1
						if(temp_info -> type -> type == Array)
						{
							entity.ispointer = 2;
							return entity;
						}
						else if(temp_info -> type -> type == Pointer)
						{
							entity.ispointer = 3;
							return entity;
						}
						break;
				}
			}
			if(expr.arg1.type == ExprArg)
			{
				entity = search_entity(expr.arg1.expr, ispointer);
				if(entity.ispointer == 1)
				{
					entity.ispointer = 3;
					return entity;
				}
				else if(entity.ispointer >= 2)//modified pointer or array
					return entity;
			}
			if(expr.arg2.type == ExprArg)//number is better than -1
			{
				entity = search_entity(expr.arg1.expr, ispointer);
				if(entity.ispointer == 1)
				{
					entity.ispointer = 3;
					return entity;
				}
				else if(entity.ispointer >= 2)//modified pointer or array
					return entity;

			}
			break;//don't care other case	

		default://if there are some other op, the arg can't be Pointer and there won't be & before the expr, just return -1
			break;
	}
	/* if the program has already been wrong, anything returned is meaningless, so just return -1 */
	entity.ispointer = -1;
	return entity;
}

/*
   modify pointer_in first, then move pointer_in to pointer_out, then update pointer_in
*/

static void trans(struct triargexpr_list * temp_node)
{
	struct triargexpr * expr = temp_node -> entity;
	if(expr -> op == Assign)
	{
		if(expr -> arg1.type == IdArg)
		{
			struct value_info * arg1_info, * arg2_info;
			struct entity_type entity;
			int arg1_index, arg2_index;
			arg1_info = symbol_search(simb_table, cur_func_info -> func_symt, expr -> arg1.idname);
			arg1_index = arg1_info -> no;
			if(arg1_info -> type -> type == Pointer)
			{
				switch(expr -> arg2.type)
				{
					case IdArg:
						arg2_info = symbol_search(simb_table, cur_func_info -> func_symt, expr -> arg2.idname);
						arg2_index = arg2_info -> no;
						if(arg2_info -> type -> type == Pointer || arg2_info -> type -> type == Array)
							pointer_list_rplc(tmp_out, arg1_index, arg2_index);//should use out current now
						else
							pointer_list_rplc(tmp_out, arg1_index, -1);
						break;
					case ExprArg:
						entity = search_entity(expr -> arg2.expr, 0);
						switch(entity.ispointer)
						{
							case -1:
								pointer_list_rplc(tmp_out, arg1_index, -1);
								break;
							case 1:
								pointer_list_rplc(tmp_out, arg1_index, entity.index);
								break;
							case 0:
							case 2:
								pointer_list_rplc_entity(tmp_out, arg1_index, entity.index);
								break;
							case 3:
								pointer_list_rplc_modptr(tmp_out, arg1_index, entity.index);
								break;
							default:
								fprintf(stderr, "Error just in case.\n");
								break;
						}
						break;
					default:
						fprintf(stderr, "Error just in case.\n");
						break;
				}
			}
		}
	}
	/*else do nothing*/
}

static void trans_in_to_out(struct basic_block * block)//update
{
	struct triargexpr_list * temp_node;
	temp_node = block -> head;
	while(temp_node != NULL)
	{
		trans(temp_node);
#ifdef TRANS_DEBUG
		print_triargexpr(*(temp_node -> entity));
		print_list(tmp_out);
#endif
		temp_node = temp_node -> next;
	}
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
			new_tmp_out();

			pointer_list_copy(tmp_out, pointer_in[index]);//just copy
			
			trans_in_to_out(DFS_array[index]);
			
			pointer_list_clear(pointer_in[index]);
			
			struct basic_block_list * list_node = DFS_array[index] -> prev;
			while(list_node != NULL)
			{
				pointer_list_merge(pointer_out[list_node -> entity-> index], pointer_in[index]);
				list_node = list_node -> next;
			}

			if(pointer_list_is_equal(tmp_out, pointer_out[index]) != 1)
			{
				change = 1;
				pointer_list_move(pointer_out[index], tmp_out);//tmp_out will be moved to pointer_out[index], and the old pointer_out[index] will be free
			}
			else
				free_tmp_out();//there will be an extra tmp_out		
#ifdef POINTER_DEBUG
			printf("block %d:\n", index);
			printf("In:");
			print_list(pointer_in[index]);
			printf("Out:");
			print_list(pointer_out[index]);
#endif	
		}
	//	change = 0;/*Temp method*/
	}
#ifdef POINTER_DEBUG
	printf("\n");
#endif
}

static void generate_entity_for_each(struct triargexpr_list * tmp_node)
{
	struct triargexpr * expr = tmp_node -> entity;
	struct value_info * arg1_info; 
	struct entity_type entity;
	switch(expr -> op)
	{
		case Subscript:
			/*The arg1 must be id and is pointer or array*/
			arg1_info = symbol_search(simb_table, cur_func_info -> func_symt, expr -> arg1.idname);	
			if(arg1_info -> type -> type == Pointer)
			{
				tmp_node -> pointer_entity = var_list_new();
				tmp_node -> pointer_entity = var_list_copy(tmp_out[arg1_info -> no], tmp_node -> pointer_entity);
			}
			break;
		case Deref:
			/*The arg1 can be any thing*/
			if(expr -> arg1.type == IdArg)//must be a pointer or an array
			{
				arg1_info = symbol_search(simb_table, cur_func_info -> func_symt, expr -> arg1.idname);	
				if(arg1_info -> type -> type == Pointer)//if Array, need not to deal with it
				{
					tmp_node -> pointer_entity = var_list_new();
					tmp_node -> pointer_entity = var_list_copy(tmp_out[arg1_info -> no], tmp_node -> pointer_entity);
				}
			}
			else if(expr -> arg1.type == ExprArg)
			{
				entity = search_entity(expr -> arg1.expr, 0);
				switch(entity.ispointer)
				{
					case -1:
						tmp_node -> pointer_entity = NULL;//Empty
						break;
					case 0:
					case 2:
						tmp_node -> pointer_entity = var_list_new();
						tmp_node -> pointer_entity = var_list_append(tmp_node -> pointer_entity, entity.index);
						break;
					case 1:
						tmp_node -> pointer_entity = var_list_new();
						tmp_node -> pointer_entity = var_list_copy(tmp_out[entity.index], tmp_node -> pointer_entity);
					default:
						fprintf(stderr, "Error when generate_entity\n");
						break;
				}
			}
			break;
		default:
			break;
	}
}/*editing now*/	

static void generate_entity_for_all()
{
	struct triargexpr_list * temp_node;
	int block_index, index;
	for(block_index = 0; block_index < g_block_num; block_index ++)
	{
		index = DFS_array[block_index] -> index;
		temp_node = DFS_array[block_index] -> head;

		new_tmp_out();
		pointer_list_copy(tmp_out, pointer_in[index]);

		while(temp_node != NULL)
		{
			trans(temp_node);
			generate_entity_for_each(temp_node);
#ifdef ENTITY_DEBUG
			print_triargexpr(*(temp_node -> entity));
			var_list_print(temp_node -> pointer_entity);
			printf("\n");
#endif
			temp_node = temp_node -> next;
		}
		
		free_tmp_out();
	}
}

void pointer_analyse(int funcindex)
{
	set_cur_function(funcindex);
	new_temp_list();
#ifdef POINTER_DEBUG
	printf("Done:new_temp_list\n");
#endif
	generate_in_out_for_all();
#ifdef POINTER_DEBUG
	printf("Done:generate_in_out_for_all\n");
#endif
	generate_entity_for_all();
#ifdef POINTER_DEBUG
	printf("Done:generate_entity_for_all\n");
#endif
	free_temp_list();
#ifdef POINTER_DEBUG
	printf("Done:free_temp_list\n");
#endif
}
