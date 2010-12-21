#include "minic_machinecode.h"
#include "minic_varmapping.h"
#include "minic_regalloc.h"
#include <stdio.h>
#define INITIAL_MACH_CODE_SIZE 100
#define MAX_TAG_NAME_LEN 20

enum Arg_Flag
{
	Arg_Imm,
	Arg_Reg,
	Arg_Mem
};

struct global_var_dpt
{
	int dirty;
	int last_ref;
	char has_refed;
};

static struct ralloc_info alloc_reg;
static int reg_dpt[32];//register discription
static int cur_sp;
static int total_tag_num;//set zero in new_code_table_list 

static struct global_var_dpt * g_var_dpt;
static int * global_var_tag_offset;
static int ref_g_var_num;
static int global_var_tag;

static struct triargtable * cur_table;
static int value_info * cur_func_info;
static int cur_var_id_num;
static int cur_ref_var_num;
static int cur_func_index;
static var_list * active_var_array; 

/* may only use bp sp lr and pc, so there is 29 registers can be used*/
static const int max_reg_num = 27;
static int cur_code_index;
static int cur_code_bound;
static inline void set_cur_function(int func_index)
{
	cur_table = table_list[func_index];
	cur_func_info = symt_search(simb_table ,table_list[func_index] -> funcname);//the 
	cur_var_id_num = table_list[func_index] -> var_id_num;
	cur_ref_var_num = get_ref_var_num();//the varmapping is always right during this file
	cur_func_index = func_index;
	
	int string_num;//get string num from the symtable
	g_var_dpt = calloc(g_global_id_num, sizeof(struct global_var_dpt));//need get last ref from active var analyse
	global_var_tag_offset = calloc(g_global_id_num + string_num, sizeof(int));//need free
	ref_g_var_num = 0;
	global_var_tag = total_tag_num ++;//as the head element of the global var

	cur_sp = 0;//just point to bp

	cur_code_index = 0;
	cur_code_bound = INITIAL_MACH_CODE_SIZE; 
	code_table_list[func_index].table = calloc(cur_code_bound, sizeof(struct mach_code));  
	code_table_list[func_index].code_num = 0;
	int index;
	for(index = 0; index < 31; index ++)
		reg_dpt[index] = -1;
}

static inline void leave_cur_function()
{
	free(global_var_tag_offset);
	free(g_var_dpt);
}

static inline void insert_code(struct mach_code newcode)
{
    if(cur_code_index >= cur_code_bound)
    {
        struct mach_code * tmp_table_list = calloc(2 * cur_code_bound, sizeof(struct mach_code));
        memcpy(tmp_table_list, code_table_list[cur_func_index].table, sizeof(struct mach_code) * cur_code_bound); 
        cur_code_bound *= 2;
        free(code_table_list[cur_func_index].table);
		code_table_list[cur_func_index].table = tmp_table_list;
    }
	code_table_list[cur_func_index].code_num ++;
	code_table_list[cur_func_index].table[cur_code_index++] = newcode;
}

static inline void insert_tag_code(int tag_num)
{
	struct mach_code new_code;
	new_code.op_type = TAG;
	new_code.tag_num = tag_num;
	insert_code(new_code);
}

static inline int ref_global_var(int var_id)//get the global var offset
{
	struct var_info * id_info = get_info_of_id(var_id);
	if(id_info -> tag_offset == -1)
	{
		id_info -> tag_offset = ref_g_var_num;
		global_var_tag_offset[ref_g_var_num ++] = var_id;
	}
	return id_info -> tag_offset; 
}

static inline char * check_is_dest(int exprnum)
{
	struct var_info * expr_info = get_info_of_temp(exprnum);
	if(expr_info != NULL && expr_info -> tag_num == -2)//means this is a dest
	{
		expr_info -> tag_num = total_tag_num ++;
		insert_tag_code(expr_info -> tag_num); 
	}
}

static inline char * ref_jump_dest(int expr_id)//get the tag for jump dest
{
	struct var_info * id_info = get_info_of_temp_for_tag(expr_id);
	return gen_new_tag(id_info -> tagnum);
}

static inline char * gen_new_tag(int tag_num)
{
	char tag_name[MAX_TAG_NAME_LEN] = ".L";
	char tag_num_name[MAX_TAG_NAME_LEN];
	itoa(tag_num, tag_num_name, 10);
	strcat(tag_name, tag_num_name);
	return strdup(tag_name);//but need to free later
}

static inline struct mach_code_list * new_push_node();//push to stack and mark the varinfo
static inline load_global_var();
static inline gen_tempreg();//general an temp reg for the var should be in memory
static inline restore_tempreg();
struct mach_code//mach means machine
{
	enum mach_op_type op_type;
	
	union
	{
		enum dp_op_type dp_op;
		enum mem_op_type mem_op;
		enum brach_op_type branch_op;
	};

	int dest;//only reg
	
	union
	{
		struct mach_arg arg1;
		enum condition_type cond;					/* used in condition-jump */
	};
		
	struct mach_arg arg2, arg3;
	
	union
	{
		uint8_t sign;								/* 1 change sign, 0 not *//* used in dp */
		uint8_t link;								/* 1 jump and link, 0 not *//* used in branch */
		uint8_t offet;								/* 1 is +, and 0 is no, -1 is - *//* used in mem */
	};	

	union
	{
		enum shift_type shift;					/* used in data-processing and memory-access*/
		enum special_width width;
	};

	enum indexed_type indexed;					/* used in mem */
	
	int tag_num;								/* -1 is no */
};

//static inline struct mach_code * new_dp_code(enum dp_op_type dp_op, int dest)   
static inline void insert_code_store()
{

static inline insert_mem_code(enum mem_op_type mem_op, int dest, struct mach_arg arg1, struct mach_arg arg2, struct mach_arg arg3, int offset, enum shift_type shift, enum indexed_type indexed, int tag_num)
{
	struct mach_code new_code;
	new_code.op_type = MEM;
	new_code.mem_op = mem_op;
	new_code.dest = dest;
	new_code.arg1 = arg1;
	new_code.arg2 = arg2;
	new_code.arg3 = arg3;
	new_code.offset = offset;
	new_code.offset = shift;
		insert_code(struct mach_code newcode);	
}


static inline void check_reg(int ref_index)//must deal with dirty and has_refed before
{
	int tmp_g_var_id = reg_dpt[reg_num];
	if(is_global(tmp_g_var_id) && g_var_dpt[tmp_g_var_id] -> has_refed && g_var_dpt[tmp_g_var_id] -> dirty)
	{
		/*store*/
		
	}
}

static inline enum arg_flag mach_prepare_arg(int ref_index, int arg_index, struct var_info * arg_info, int arg_type)/* arg_type : 0=>dest, 1=>normal */
{
	enum Arg_Flag flag;
	
	if(alloc_reg.result[arg_index] != -1)
	{
		flag = Arg_Reg;
		if(arg_info -> reg_addr == -1)
		{
			arg_info -> reg_addr = alloc_reg.result[arg_index];
			check_reg(arg_info -> reg_addr);
		}
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

				arg1_flag = mach_prepare_arg(expr -> index, arg1_index, arg1_info, 0);

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
					arg1_flag = mach_prepare_arg(arg1_index, arg1_info, 1);

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
						/* lod tempreg, arg1 */;
						/* lod dest, [tempreg] */;
						/* restore tempreg */;
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
				struct int arg1_index;
				struct var_info * arg1_info; 
				enum arg_flag arg1_flag;

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

				if(arg1_flag == Arg_Reg)
					/* push arg1 */;
				else
				{
					/* lod tempreg, arg1 */;
					/* push tempreg */;
					/* restore tempreg */;
				}
				break;
			}

		case Return:
			{
				struct int arg1_index;
				struct var_info * arg1_info; 
				enum arg_flag arg1_flag;

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

				if(arg1_flag == Arg_Reg)
				{
					/* if arg1 in r0 can be optimized */;
					/* mov arg1, r0 */;
				}
				else
					/* lod r0, arg1 */;
				break;
			}

		case Nullop:
			break;

	}
}

void new_code_table_list()
{
	code_table_list = calloc(g_table_list_size, sizeof(struct mach_code_table)); 
	total_tag_num = 0;
}

void free_code_table_list()
{
	int index;
	for(index = 0; index < g_table_list_size; index ++)
	{
		if(code_table_list[index].table != NULL)
			free(code_table_list[index].table);
	}
	free(code_table_list);
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
