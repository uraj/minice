#include "minic_basicblock.h"
#include "minic_triargtable.h"
#include "minic_triargexpr.h"
#include "minic_varmapping.h"
#include "minic_flowanalyse.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
//some local data
static char * flag_list;
static struct basic_block ** entry_index_to_block;//waste some space, but is more faster
//the two list are all temp, must be free at once after used.
static int cur_func_index;
static struct basic_block_list * DFS_list = NULL;

static inline void new_temp_list(int size)//called first
{
	flag_list = malloc(sizeof(char) * size);
	entry_index_to_block = malloc(sizeof(struct basic_block *) * size);
	memset(flag_list, 0, size * sizeof(char));
	memset(entry_index_to_block, 0, size * sizeof(struct basic_block *));
	/* to make sure */
}

static inline void free_temp_list()
{
	free(flag_list);
	free(entry_index_to_block);
}

static inline struct basic_block_list * basic_block_list_append(struct basic_block_list * list, struct basic_block * newblock)//append block to the list
{
	struct basic_block_list * newnode = malloc(sizeof(struct basic_block_list));	
	newnode -> entity = newblock;
	newnode -> next = NULL;
	if(list != NULL)
		newnode -> next = list;
	return newnode;
}

static struct triarg unary_search(struct triargexpr * table, struct triargexpr * expr)
{
	struct triarg tmp_arg;
	switch(expr -> op)
	{
		case Uplus:                      /* +  */
		case Plusplus:                   /* ++ */
		case Minusminus:                 /* -- */
			if(expr -> arg1.type == ExprArg)
			{
				tmp_arg = unary_search(table, &table[expr -> arg1.expr]); 
				if(tmp_arg.type != -1)
					expr -> arg1 = tmp_arg;
			}
			return expr -> arg1;
		default:
			tmp_arg.type = -1;
			return tmp_arg;
	}
}

static void scan_for_entry(struct triargexpr * table, int expr_num)//scan for entry and record them
{
	int i;
	struct triargexpr expr;
	struct triargexpr cond_expr;
	for(i = 0; i < expr_num; i++)
	{
		expr = table[i];
		switch(expr.op)
		{
			case Assign:					 /* = */
				if(expr.arg1.type == ExprArg)
					insert_tempvar(expr.arg1.expr);
				if(expr.arg2.type == ExprArg)/* mark */
					insert_tempvar(expr.arg2.expr);//will be removed as a kind of optimizing later

				break;//Current now, don't treat assign reference as real reference, can be optimizing some, and will be optimized some
			case Eq:                         /* == */
			case Neq:                        /* != */
			case Ge:                         /* >= */
			case Le:                         /* <= */
			case Nge:                        /* <  */
			case Nle:                        /* >  */
			case Plus:                       /* +  */
			case Minus:                      /* -  */
			case Mul:                        /* *  */
				if(expr.arg1.type == ExprArg)
					insert_tempvar(expr.arg1.expr);
				if(expr.arg2.type == ExprArg)
					insert_tempvar(expr.arg2.expr);
				break;	
			case Uplus:                      /* +  */
			case Plusplus:                   /* ++ */
			case Minusminus:                 /* -- */
				/*
				//this optimizing shall be opened later
				{
					struct triarg tmp_arg;
					tmp_arg = unary_search(table, &table[i]);
					if(tmp_arg.type == ExprArg)
						insert_tempvar(tmp_arg.type);
				}
				break;
				*/
			
			case Subscript:                  /* [] */
				if(expr.arg1.type == IdArg)/* in fact it is sure */
				{
					struct var_info * tmp_var_info = get_info_of_id(expr.arg1.idname);
					struct value_info * tmp_func_info = symt_search(simb_table, table_list[cur_func_index] -> funcname);
					struct value_info * tmp_value_info = symbol_search(simb_table, tmp_func_info -> func_symt, expr.arg1.idname);
					if(tmp_value_info -> type -> type == Array )
					{
						if(tmp_var_info -> ref_point == NULL)
							tmp_var_info -> ref_point = var_list_new();
						tmp_var_info -> ref_point = var_list_append(tmp_var_info -> ref_point, expr.index);
					}
				}/* make array ref point list used when flush array elem in regs */

			case Uminus:                     /* -  */	
			case Ref:                        /* &  */
			case Deref:                      /* '*' */
			case Arglist:
			case Return:
				if(expr.arg1.type == ExprArg)
					insert_tempvar(expr.arg1.expr);
				break;
			
			case UncondJump:
				if(expr.arg1.expr != -1)//-1 means the end of the function
					flag_list[expr.arg1.expr] = 1;
				break;
			case TrueJump:
				if(expr.arg1.type == ExprArg)
				{
					cond_expr = table[expr.arg1.expr];
					switch(cond_expr.op)
					{
						case Eq:                         /* == */
						case Neq:                        /* != */
						case Ge:                         /* >= */
						case Le:                         /* <= */
						case Nge:                        /* <  */
						case Nle:                        /* >  */
							break;
						default:
							insert_tempvar(expr.arg1.expr);
							break;
					}
				}
				if(expr.arg2.expr != -1)//the destination never changed
					flag_list[expr.arg2.expr] = 1;//may don't need next expr as an entry, we'll know that at the next step.
				if(expr.arg1.type == ImmArg)
				{	
					if(expr.arg1.imme != 0)//TrueJump not 0 (x) = UncondJump (x)
					{
						table[i].op = UncondJump;
						table[i].arg1.type = ExprArg;
						table[i].arg1.expr = expr.arg2.expr;
					}
					else///TrueJump 0 (x) = UncondJump next
					{
						table[i].op = UncondJump;
						table[i].arg1.type = ExprArg;
						struct triargexpr_list * tmp_node = table_list[cur_func_index] -> index_to_list[expr.index];
						if(tmp_node -> next == NULL )
							table[i].arg1.expr = -1;//the end of the function
						else
							table[i].arg1.expr = tmp_node -> next -> entity -> index;
						if(expr.arg2.expr != -1)//the destination never changed
							flag_list[expr.arg2.expr] = 0;//cancel the entry
						if(table[i].arg1.expr != -1)
							flag_list[table[i].arg1.expr] = 1;
					}
				}	
			break;
			case FalseJump:
				if(expr.arg1.type == ExprArg)
				{
					insert_tempvar(expr.arg1.expr);
					cond_expr = table[expr.arg1.expr];
					switch(cond_expr.op)
					{
						case Eq:                         /* == */
						case Neq:                        /* != */
						case Ge:                         /* >= */
						case Le:                         /* <= */
						case Nge:                        /* <  */
						case Nle:                        /* >  */
							break;
						default:
							insert_tempvar(expr.arg1.expr);
							break;
					}	
				}
				if(expr.arg2.expr != -1)
					flag_list[expr.arg2.expr] = 1;	
				if(expr.arg1.type == ImmArg)
				{
					if(expr.arg1.imme == 0)//FalseJump 0 (x) = UncondJump(x)
					{
						table[i].op = UncondJump;
						table[i].arg1.type = ExprArg;
						table[i].arg1.expr = expr.arg2.expr;
					}
					else//FalseJump 1 (x) has no meaning, don't deal with
					{
						table[i].op = UncondJump;
						table[i].arg1.type = ExprArg;
						struct triargexpr_list * tmp_node = table_list[cur_func_index] -> index_to_list[expr.index];
						if(tmp_node -> next == NULL )
							table[i].arg1.expr = -1;//the end of the function
						else
							table[i].arg1.expr = tmp_node -> next -> entity -> index;
						if(expr.arg2.expr != -1)//the destination never changed
							flag_list[expr.arg2.expr] = 0;//cancel the entry
						if(table[i].arg1.expr != -1)
							flag_list[table[i].arg1.expr] = 1;	
					}
				}
				break;
			default: break;
			/* Nullop done when meet exprnum = 1 or exprnum = 0 */
		}
	}
}

static inline struct basic_block * new_block()
{
	struct basic_block * newblock = malloc(sizeof(struct basic_block));
	newblock -> index = 0;
	newblock -> prev = NULL;
	newblock -> next = NULL;
	newblock -> head = NULL;
	newblock -> tail = NULL;	
	return newblock;
}

/*
for example:
   if(a < c)
   {
	  ;
   }

result:
   (0)a < c
   (1)TrueJump (0) (3)
   (2)UncondJump (3)
   (3)
   
   After optimizing, there should be only one path from 

 */

/*The last jump destination is the number of the basic_block*/
static struct basic_block * make_block(struct triargexpr_list * entrance)//generate one block
{
	if(entrance == NULL)
		return NULL;
	if(entry_index_to_block[entrance -> entity -> index] != NULL)//had reached
		return entry_index_to_block[entrance -> entity -> index];

	if(entrance -> entity -> op == UncondJump)//no use block
	{
		if(entrance -> entity -> arg1.expr == -1)//jump to the end of the function
			return NULL;
		return make_block(table_list[cur_func_index] -> index_to_list[entrance -> entity -> arg1.expr]);
	}

	struct basic_block * newblock = new_block();
	struct basic_block * childblock;

	entry_index_to_block[entrance -> entity -> index] = newblock;//mark reached

	newblock -> head = entrance;
	struct triargexpr_list * p_cur_expr = entrance;
	struct triargexpr * expr;
	newblock -> tail = p_cur_expr;//change every loop
	expr = p_cur_expr -> entity;	
	while(1)
	{
		switch(expr -> op)
		{
			case TrueJump:
			case FalseJump:
				if(expr -> arg2.expr != -1)
					childblock = make_block(table_list[cur_func_index] -> index_to_list[expr -> arg2.expr]);  
				else childblock = NULL;

				if(childblock != NULL)
				{
					newblock -> next = basic_block_list_append(newblock -> next, childblock);
					childblock -> prev = basic_block_list_append(childblock -> prev, newblock);
				}
				else
					expr -> arg2.expr = -1;

				childblock = make_block(p_cur_expr -> next);//deal with this kind of NULL in make_block	

				if(childblock != NULL)
				{
					if(childblock == newblock -> next -> entity)//if condition jump's destination is the same as next, then optimizing
					{
						expr -> op = UncondJump;//should judge whether is the first instruction
						expr -> arg1.type = ExprArg;//the dest block index will be filled later
					}//Current now there must be an compare before this, so should not be only one Uncond Jump
					else
					{
						newblock -> next = basic_block_list_append(newblock -> next, childblock);
						childblock -> prev = basic_block_list_append(childblock -> prev, newblock);
					}
				}

				DFS_list = basic_block_list_append(DFS_list, newblock);	
				newblock -> tail -> next = NULL;
				g_block_num ++;
				return newblock;
			case UncondJump:
				if(expr -> arg1.expr != -1)
					childblock = make_block(table_list[cur_func_index] -> index_to_list[expr -> arg1.expr]);
				else
					childblock = NULL;

				if(childblock != NULL)// may be an empty block, like Jump to -1
				{
					newblock -> next = basic_block_list_append(newblock -> next, childblock);
					childblock -> prev = basic_block_list_append(childblock -> prev, newblock);
				}
				else
					expr -> arg1.expr = -1;//means that jump to the end of the function

				DFS_list = basic_block_list_append(DFS_list, newblock);
				newblock -> tail -> next = NULL;
				g_block_num ++;
				return newblock;//need a func
			default: break;
		}
		p_cur_expr = p_cur_expr -> next;
		if(p_cur_expr == NULL)//to the end of the function
			break;
		expr = p_cur_expr -> entity;//update
		if(flag_list[expr -> index] == 1)//is an entry, but the first expr is sure to be an entry
		{
			childblock = make_block(p_cur_expr);
			if(childblock != NULL)
			{
				newblock -> next = basic_block_list_append(newblock -> next, childblock);
				childblock -> prev = basic_block_list_append(childblock -> prev, newblock);
			}

			DFS_list = basic_block_list_append(DFS_list, newblock);
			newblock -> tail = newblock -> tail;
			newblock -> tail -> next = NULL;//may need next, set NULL currentnow, Mark
			g_block_num ++;
			return newblock;
		}
		newblock -> tail = p_cur_expr;//change every loop	
	}
	g_block_num ++;
	DFS_list = basic_block_list_append(DFS_list, newblock);
	return newblock;
}

static void trans_to_array()//DFS_array should be free later
{
	if(DFS_array != NULL)
		free(DFS_array);//the content should be rested
	DFS_array = malloc(sizeof(struct basic_block *) * g_block_num);
	int index = 0;
	struct basic_block_list * list_node = DFS_list;
	struct basic_block_list * tmp_node = NULL;
	while(list_node != NULL)
	{
		list_node -> entity -> index = index;
		DFS_array[index++] = list_node -> entity;
		tmp_node = list_node;
		list_node = list_node -> next;
		free(tmp_node);
	}
	DFS_list = NULL;

	for(index = 0; index < g_block_num; index++)
	{
		switch(DFS_array[index] -> tail -> entity -> op)
		{
			case UncondJump:
				if(DFS_array[index] -> tail -> entity -> arg1.expr != -1)
					DFS_array[index] -> tail -> entity -> arg1.expr = 
						DFS_array[index] -> next -> entity -> index;//There must be only one next
				break;
			case FalseJump:
			case TrueJump:
				if(DFS_array[index] -> tail -> entity -> arg2.expr != -1)
				{
					struct basic_block_list * temp_node = DFS_array[index] -> next;
					while(temp_node -> next != NULL)
						temp_node = temp_node -> next;//There must be one or two
					DFS_array[index] -> tail -> entity -> arg2.expr = temp_node -> entity -> index;
				}
				break;
			default: break;
		}
	}
}

static void print_fd()
{
	printf("Block num:%d\n\n", g_block_num);/**/
	int index;
	for(index = 0; index < g_block_num; index++)
	{
		printf("Block No.%d\n", DFS_array[index] -> index);
		struct triargexpr_list * expr = DFS_array[index] -> head; 
		while(expr != NULL)
		{
			print_triargexpr(*(expr -> entity));
			expr = expr -> next;
		}
		struct basic_block_list * next_list_node = DFS_array[index] -> next;
		printf("next blocks:");
		while(next_list_node != NULL)
		{
			printf("%d ", next_list_node -> entity -> index);
			next_list_node = next_list_node -> next;
		}
		printf("\n");

		next_list_node = DFS_array[index] -> prev;
		printf("prev blocks:");
		while(next_list_node != NULL)
		{
			printf("%d ", next_list_node -> entity -> index);
			next_list_node = next_list_node -> next;
		}
		printf("\n\n");
	}
}

struct basic_block * make_fd(int function_index)
{
	cur_func_index = function_index;
	new_var_map(function_index);//********************************
	new_temp_list(table_list[function_index] -> expr_num);	
	g_block_num = 0;
	scan_for_entry(table_list[function_index] -> table, table_list[function_index] -> expr_num);
	struct basic_block * head = make_block(table_list[function_index] -> head);
	trans_to_array();
#ifdef SHOWBASICBLOCK
	print_fd();
#endif
	free_temp_list();
//	free_var_map();	
	return head;
}


