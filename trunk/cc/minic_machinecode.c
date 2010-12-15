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
static int cur_code_index;

static int total_tag_num;//set zero in new_code_table_list 

/* may only use bp sp lr and pc, so there is 29 registers can be used*/
static const int max_reg_num = 27;

static inline void set_cur_function(int func_index)
{
	cur_table = table_list[func_index];
	cur_func_info = symt_search(simb_table ,table_list[func_index] -> funcname);//the 
	cur_var_id_num = table_list[func_index] -> var_id_num;
	cur_ref_var_num = get_ref_var_num();//the varmapping is always right during this file
	
	code_table[func_index].table = calloc(1, sizeof(struct mach_code_table));  
	code_table[func_index].code_num = 0;
	cur_sp = 0;//just point to bp
	cur_code_index = 0;
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
			if(is_global(arg_index))
			{
				/* load global var */
			}
		}
	}
	else
	{
		flag = Arg_Mem;
		if(arg_info -> mem_addr == -1)
		{
			if(!isglobal(arg_index))
			{
				/* push and mark*/
			}
		}
	}
	return flag;
}

static void gen_per_code(struct triargexpr * expr)
{
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
				else//need to deal with * and []
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

				if(arg1_flag == Arg_Mem)//can't be immd
					/* lod tempreg */;

				/* add or sub tempreg, tempreg, 1 */;

				if(dest_index != -1)/*be used later*//******************** can be optimized *******************/
				{
					if(dest_flag == Arg_Reg)
						/* mov tempreg => dest reg*/;
					else
					{
						/* str */;
					}
				}

				/* restore for arg1 */;

				break;
			}

		case TrueJump:
		case FalseJump://immed cond has been optimized before, so the cond can only be expr or id
			{
				struct int arg1_index;
				struct var_info * arg1_info; 
				enum arg_flag arg1_flag;

				//ImmArg op ImmArg should be optimized before
				if(expr -> arg1.type == IdArg)
				{
					arg1_index = get_index_of_id(expr -> arg1.idname);
					arg1_info = get_info_from_index(arg1_index);
				}
				else//can't be immed
				{
					arg1_index = get_index_of_temp(expr -> arg1.expr);
					if(arg1_index != -1)
						arg1_info = get_info_from_index(arg1_index);
				}
				
				if(arg1_index == -1)
				{
					/*gen condition*/;//should return jump cond
					/* cond jump to .L+(varmap->tag or total_tag_num) */;//we need a jump table in varmap and total_tag_num
				}
				else
				{
					arg1_flag = mach_prepare_arg(arg1_index, arg1_info, 1);/* when must be 1, not be 0 */

					if(arg1_flag == Arg_Mem)//can't be immd
						/* lod tempreg */;
					/* CSUB tempreg, 0 */
					/* beq or bne to .L+..*/;
					/* restore tempreg */;
				}

				break;
			}

		case UncondJump:
			{
				/* brx to .L+..*/;
				break;
			}


		case Uplus:                      /* +  */
		case Uminus:                     /* -  */
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

					arg1_flag = mach_prepare_arg(arg1_index, arg1_info, 1);
				}
				else
					arg1_flag = Arg_Imm;	

				if(dest_flag == Arg_Reg)
				{
					if(arg1_flag == Arg_Mem)
					{
						if(expr -> op == Uminus)
						{
							/* lod tempreg */;
							/* mvn tempreg, dest */;
							/* restore for arg1 */;
						}
						else
							/* lod dest, arg1 */;
					}
					else
					{
						if(expr -> op == Uminus)
							/* mvn arg1, dest */;
						else /* mov arg1, dest */;
					}
				}
				else
				{
					if(arg1_flag == Arg_Mem)
					{
						/* lod tempreg */;
						if(expr -> op == Uminus)
							/* mvn tempreg, tempreg */;
						/* str tempreg, dest */;
						/* restore for arg1 */;
					}
					else
					{
						if(expr -> op == Uminus)
							/* mvn arg1, arg1 */;
						/* str arg1, dest */;
					}
				}	
				break;
			}

		case Subscript:                  /* [] */
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
				arg1_index = get_index_of_id(expr -> arg1.idname);//can only be id
				arg1_info = get_info_from_index(arg1_index);
				arg1_flag = mach_prepare_arg(arg1_index, arg1_info, 1);

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
					/* lod arg1_flag, tempreg1 */;

				if(arg2_flag == Arg_Mem)
				{
					/* lod arg2_flag, tempreg2 */;
					if(dest_flag == Arg_Mem)
					{
						/* lod tempreg1, [tempreg1, {+}tempreg2] */;
						/* str tempreg1, dest */;
					}
					else
						/* lod dest, [tempreg1, {+}tempreg2] */;
					/* restore tempreg1 and tempreg2 */;
				}
				else if(arg2_flag == Arg_Imm)
				{
					if(/*arg2_flag >= 5 bits*/)
					{
						/*mov arg2_flag, tempreg2*/;
						if(dest_flag == Arg_Mem)
						{
							/* lod tempreg1, [tempreg1,{+}tempreg2] */;
							/* str tempreg1, dest */;
							/* restore tempreg1 and tempreg2 */;
						}
						else
							/* lod dest, [tempreg1,{+}tempreg2] */;
						/* restore tempreg1 and tempreg2 */;
					}
					else
					{
						if(dest_flag == Arg_Mem)
						{
							/* lod tempreg1, [tempreg1, arg2] */;
							/* str tempreg1, dest */;
						}
						else
							/* lod dest, [tempreg1, arg2] */;
						/* restore tempreg1 */;
					}
				}
				break;	
			}

		case Ref:                        /* &  */
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

				if(arg1_info -> mem_addr == -1)
				{
					/* push arg1 */;
				}

				if(dest_flag == Arg_Reg)
				{
					if(/* arg1 -> mem_addr > 9 bits */)//the mem_addr need some adjust
					{
						/* mov tempreg, mem_addr */;
						/* mov dest, tempreg*/;
						/* restore tempreg */;
					}
					else
						/* mov dest, mem_addr; */
				}
				else
				{
					if(/* arg1 -> mem_addr > 9 bits */)//the mem_addr need some adjust
						/* mov tempreg, mem_addr */;
					else
						/* mov tempreg, mem_addr */;

					/* str tempreg, dest */;
					/* str tempreg, dest */
					/* restore tempreg */;

				}
				break;
			}

		case Deref:                      /* '*' */
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

				if(dest_flag == Arg_Reg)
				{
					if(arg1_flag == Arg_Reg)
						/* lod dest, [arg1] */;
					else
					{
						/* lod dest, tempreg */;
						/* lod dest, [tempreg] */;
						/* restore dest */;
					}
				}
				else
				{
					if(arg1_flag == Arg_Reg)
					{
						/* lod tempreg, [arg1] */;
						/* str tempreg, dest */;
						/* restore tempreg */;
					}
					else
					{
						/* lod tempreg2, arg1.mem_addr */;
						/* lod tempreg,  [tempreg2]*/;
						/* str tempreg, dest */;
						/* restore tempreg */;
						/* restore tempreg2 */;
					}
				}
				break;
			}	
		
		case Funcall:                    /* () */
			{
				/* b.l arg1.idname */;
				/* caller save */; // callee save and sp ip fp deal at the head of each function */
			}

		case Arglist:
			{
				/* push */
			}
		case Return:

		case Nullop:

	}
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
	}
}
