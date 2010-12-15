#include "minic_machinecode.h"
#include "minic_varmapping.h"
#include "minic_regalloc.h"
#include <stdio.h>

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
					struct int arg1_index, arg2_index;
					struct var_info * arg1_info, * arg2_info; 
					struct mach_code_list * new_code = new_mach_code_list();
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

					if(expr -> arg2.type == IdArg)
					{
						arg2_index = get_index_of_id(expr -> arg2.idname);
						arg2_info = get_info_from_index(arg2_index);
					}
					else//can't be immed
					{
						arg2_index = get_index_of_temp(expr -> arg2.expr);
						arg2_info = get_info_from_index(arg2_index);
					}
						
					if(alloc_reg.result[arg1_index] != -1)
					{
						if(arg1_info -> reg_addr != -1)
						{
							new_node -> dest.type = Mach_Reg;
							new_node -> dest.reg = arg1_info -> reg_addr;
						}
						else
						{
							arg1_info -> reg_addr = alloc_reg.result[arg1_index];
							if(is_global(expr -> arg1.idname))
							{
								/* lod */
							}

							new_node -> dest.type = Mach_Reg;
							new_node -> dest.reg = arg1_info -> reg_addr;
						}
					}
					else
					{	
						if(arg1_info -> mem_addr != -1)
						{
							new_node -> arg1.type = Mach_Reg;
							new_node -> arg1.

				break;				
			case Eq:                         /* == */
			case Neq:                        /* != */
			case Ge:                         /* >= */
			case Le:                         /* <= */
			case Nge:                        /* <  */
			case Nle:                        /* >  */
			case Plus:                       /* +  */
			case Minus:                      /* -  */
			case Mul:                        /* *  */
			case Subscript:                  /* [] */
			case Funcall:                    /* () */
    
			case TrueJump:
			case FalseJump:
			case UncondJump:
    
			case Uplus:                      /* +  */
			case Uminus:                     /* -  */
			case Plusplus:                   /* ++ */
			case Minusminus:                 /* -- */
			case Ref:                        /* &  */
			case Deref:                      /* '*' */

			case Arglist:
			case Return:
 
			case Nullop:
};
 
		}
	}
}
