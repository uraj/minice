#include "minic_machinecode.h"
#include "minic_varmapping.h"
#include "minic_regalloc.h"
#include <stdio.h>

enum Arg_Flag
{
	Arg_Imm,
	Arg_Reg,
	Arg_Mem
};

static struct ralloc_info alloc_reg;
static struct triargtable * cur_table;
static int cur_var_id_num;
static int cur_ref_var_num;
static int value_info * cur_func_info;
static var_list * active_var_array; 
static int cur_sp;

/* may only use bp sp lr and pc, so there is 29 registers can be used*/
/* all callee save, but only save the registers it has used? */
/* or caller save and callee save, but only save the registers that will be used */
static const int max_reg_num = 27;

static inline void set_cur_function(int func_index)
{
	cur_table = table_list[func_index];
	cur_func_info = symt_search(simb_table ,table_list[func_index] -> funcname);//the 
	cur_var_id_num = table_list[func_index] -> var_id_num;
	cur_ref_var_num = get_ref_var_num();//the varmapping is always right during this file

	code_table[func_index] = calloc(1, sizeof(struct mach_code_table));
	code_table[func_index].mach_code_num = 0;
	code_table[func_index].head = NULL;
	code_table[func_index].tail = NULL;
	cur_sp = 0;//just point to bp
}

static inline void new_active_var_array()
{
	active_var_array = calloc(cur_table -> exprnum, sizeof(struct var_list));//the exprnum should be updated
	//copy
}

static inline struct mach_code_list * new_mach_code_list()
{
	struct mach_code_list * new_node;
	new_node = calloc(1, sizeof(struct mach_code_list));
	new_node -> code = calloc(1, sizeof(struct mach_code));
	new_node -> next = NULL;
}

static inline void free_mach_code_list(struct mach_code_list * old_node)
{
	if(old_node != NULL)
	{
		if(old_node -> code != NULL)
			free(old_node -> code);
		free(old_node);
	}
}

static inline struct mach_code_list * new_push_node()
{
}

static inline struct mach_code_list * new_pop_node()
{
}

//static inline void deal_with_args(int dest)

static inline enum arg_flag mach_prepare_arg(int arg_index, struct var_info * arg_info, int arg_type)/* arg_type : 0=>dest, 1=>normal */
{
	enum Arg_Flag flag;
	
	if(alloc_reg.result[arg_index] != -1)
	{
		flag = Arg_Reg;
		if(arg_info -> reg_addr == -1)
			arg_info -> reg_addr = alloc_reg.result[arg_index];
		if(arg_type == 1)
		{
			if(is_global(arg2_index))
			{
				/* load global var */
			}
		}
	}
	else
	{
		flag = Arg_Mem;
		if(arg1_info -> mem_addr == -1)
		{
			if(!isglobal(arg1_index))
			{
				/* push and mark*/
			}
		}
	}
	return flag;
}

void gen_machine_code(int func_index)//Don't forget NULL at last
{
	set_cur_function(func_index);
	new_active_var_array();
	alloc_reg = reg_alloc(active_var_array, cur_table -> exprnum, cur_ref_var_num, max_reg_num);//current now
	struct triargexpr_list * tmp_node = cur_table -> head;
	while(tmp_node != NULL)//make tail's next to be NULL
	{
		struct triargexpr * expr = tmp_node -> entity;
		switch(expr -> op)
		{
			case Assign:                     /* =  */
				{
					struct int dest_index, arg1_index, arg2_index;
					struct var_info * dest_info, * arg1_info, * arg2_info; 
					enum arg_flag dest_flag, arg1_flag, arg2_flag;
									
					if(expr -> arg1.type == IdArg)
					{
						arg1_index = get_index_of_id(expr -> arg1.idname);
						arg1_info = get_info_from_index(arg1_index);
					}
					else//can't be immed
					{
						arg1_index = get_index_of_temp(expr -> arg1.expr);
						arg1_info = get_info_from_index(arg1_index);
					}
				
					arg1_flag = mach_prepare_arg(arg1_index, arg1_info, 0);

					if(expr -> arg2.type != ImmArg)
					{
						if(expr -> arg2.type == IdArg)
						{
							arg2_index = get_index_of_id(expr -> arg2.idname);
							arg2_info = get_info_from_index(arg2_index);
						}
						else
						{
							arg2_index = get_index_of_temp(expr -> arg2.expr);
							arg2_info = get_info_from_index(arg2_index);
						}
						arg2_flag = mach_prepare_arg(arg2_index, arg2_info, 1);
					}
					else
						arg2_flag = Arg_Imm;

					dest_index = get_index_of_temp(expr -> index);//can be optimized
					if(dest_index != -1)//the value has been used
					{
						dest_info = get_info_from_index(dest_index);	
						dest_flag = mach_prepare_arg(dest_index, dest_info, 0);
					}

					if(arg1_flag == Arg_Reg)
					{
						if(arg2_flag == Arg_Reg)
						{
							/* if(arg1_info -> reg_addr != arg2_info -> reg_addr)//may be very important */
							
							/* mov reg */;
						}
						else if(arg2_flag == Arg_Mem)
							/* lod */;
						else
							/* mov immd */;

						if(dest_index != -1)/* deal with return value */
						{
							if(dest_flag == Arg_Reg)	
								/* mov arg1 => dest */;
							else if(dest_flag == Arg_Mem)
								/* str arg1 => dest */;
						}
					}
					else
					{
						if(arg2_flag == Arg_Reg)
							/* str */;
						else if(arg2_flag == Arg_Imm)
						{
							/* mov immd tempreg */;
							/* str */;
							/* restore tempreg */;
						}
						else
						{
							/* lod tempreg */;
							/* str */;
							/* restore tempreg */;
						}

						if(dest_index != -1)/* deal with return value */ 
						{
							if(dest_flag == Arg_Reg)
								/* lod */;
							else if(dest_flag == Arg_Mem)
							{
								/* lod tempreg */;
								/* str */;/**************** can be optimized some ****************/
								/* restore tempreg */;
							}
						}
					}

				}
				break;				
			/* cond op translated when translating condjump */
			
			case Plus:                       /* +  */
			case Minus:                      /* -  */	
			case Mul:                        /* *  */
				{
					struct int dest_index, arg1_index, arg2_index;
					struct var_info * dest_info, * arg1_info, * arg2_info; 
					enum arg_flag dest_flag, arg1_flag, arg2_flag;
					
					dest_index = get_index_of_temp(expr -> index);
					if(dest_index == -1)
						break;
					dest_info = get_info_from_index(dest_index);
				
					dest_flag = mach_prepare_arg(dest_index, dest_info, 0);
					
					//ImmArg op ImmArg should be optimized before
					if(expr -> arg1.type != ImmArg)
					{
						if(expr -> arg1.type == IdArg)
						{
							arg1_index = get_index_of_id(expr -> arg1.idname);
							arg1_info = get_info_from_index(arg1_index);
						}
						else//can't be immed
						{
							arg1_index = get_index_of_temp(expr -> arg1.expr);
							arg1_info = get_info_from_index(arg1_index);
						}
			
						arg1_flag = mach_prepare_arg(arg1_index, arg1_info, 1);
					}
					else
						arg1_flag = Arg_Imm;


					if(expr -> arg2.type != ImmArg)
					{
						if(expr -> arg2.type == IdArg)
						{
							arg2_index = get_index_of_id(expr -> arg2.idname);
							arg2_info = get_info_from_index(arg2_index);
						}
						else
						{
							arg2_index = get_index_of_temp(expr -> arg2.expr);
							arg2_info = get_info_from_index(arg2_index);
						}

						arg2_flag = mach_prepare_arg(arg2_index, arg2_info, 1);
					}
					else
						arg2_flag = Arg_Imm;

					if(arg1_flag == Arg_Mem)
						/* lod tempreg */;
					else if(arg1_flag == Arg_Imm)//Only one Imm
						/* mov immd tempreg  */;
					
					if(arg2_flag == Arg_Imm)
						/* lod tempreg */;
					else if(arg2_flag == Arg_IMm)
						/* mov immd tempreg */;
					
					if(dest_flag == Arg_Reg)
						/* add or sub or mul */;
					else
					{
						/* add or sub to tempreg */;
						/* str */;
						/* restore tempreg */;
					}
					/* restore tempreg for arg2 */;
					/* restore tempreg for arg1 */;
					break;
				}
			
			case Plusplus:                   /* ++ */
			case Minusminus:                 /* -- */
				{
					struct int dest_index, arg1_index;
					struct var_info * dest_info, * arg1_info; 
					enum arg_flag dest_flag, arg1_flag;
					
					dest_index = get_index_of_temp(expr -> index);
					if(dest_index == -1)
						break;
					dest_info = get_info_from_index(dest_index);
				
					dest_flag = mach_prepare_arg(dest_index, dest_info, 0);
					
					//ImmArg op ImmArg should be optimized before
					if(expr -> arg1.type != ImmArg)
					{
						if(expr -> arg1.type == IdArg)
						{
							arg1_index = get_index_of_id(expr -> arg1.idname);
							arg1_info = get_info_from_index(arg1_index);
						}
						else//can't be immed
						{
							arg1_index = get_index_of_temp(expr -> arg1.expr);
							arg1_info = get_info_from_index(arg1_index);
						}
			
						arg1_flag = mach_prepare_arg(arg1_index, arg1_info, 1);/* when must be 1, not be 0 */
					}
					else
						arg1_flag = Arg_Imm;

					if(arg1_flag == Arg_Mem)//can't be immd
						/* lod tempreg */;
				
					/* add or sub tempreg, tempreg, 1 */;

					if(dest_index != -1)/*be used later*//******************** can be optimized *******************/
					{
						if(dest_flag == Arg_Reg)
							/* mov tempreg => */;
							/* add or sub or mul */;
						else
						{
							/* add or sub to tempreg */;
							/* str */;
							/* restore tempreg */;
						}
					}

					/* restore for arg1 */;
					
					break;
				}

			case Subscript:                  /* [] */
			case Funcall:                    /* () */
    
			case TrueJump:
			case FalseJump:
			case UncondJump:
    
			case Uplus:                      /* +  */
			case Uminus:                     /* -  */
			case Ref:                        /* &  */
			case Deref:                      /* '*' */

			case Arglist:
			case Return:
 
			case Nullop:
};
 
		}
	}
}
