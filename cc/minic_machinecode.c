#include "minic_machinecode.h"
#include "minic_asmout.h"
#include "minic_basicblock.h"
#include "minic_flowanalyse.h"
#include "minic_aliasanalyse.h"
#include "minic_varmapping.h"
#include "minic_regalloc.h"
#include "minic_typedef.h"
#include "minic_peephole.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#define INITIAL_MACH_CODE_SIZE 100
#define MAX_LABEL_NAME_LEN 20
#define TOTAL_REG_NUM 32
#define BYTE 1
#define WORD 4
#define REG_UNUSED -1
#define REG_TEMP -2

enum Arg_Flag
{
	Arg_Imm,
	Arg_Reg,
	Arg_Mem
};

enum mem
{
     load,
     store
};

struct Reg_Dpt
{
	int content;
};

static struct ralloc_info alloc_reg;
static struct Reg_Dpt reg_dpt[32];//register discription
static int cur_var_id_num;
static int cur_sp;
static int local_var_sp;
static int total_label_num;//set zero in new_code_table_list 

static struct Reg_Dpt shadow_reg_dpt[32];//use to deal with temp reg
static char used_reg[32];
static int * global_var_label_offset;
static int ref_g_var_num;
static char * global_var_label;

static struct triargtable * cur_table;
static struct value_info * cur_func_info;
static int cur_var_id_num;
static int cur_ref_var_num;
static int cur_func_index;

/* may only use bp sp lr and pc, so there is 29 registers can be used*/
static const int max_reg_num = 23;
static int cur_code_index;/*cur_code_index 当前数目*/
static int cur_code_bound;
static const char optmz = 1 , un_optmz = 0;
static int is_combine = 1;

static int arglist_num_mark;//count arglist num, should be clear to zero after func call and pop arg
static int caller_save_index;
static struct mach_arg null;

static inline char * gen_new_label(int label_num);
static inline void push_param(int reg_num);
static inline void pop_param_to_reg(int reg_num);
static inline void pop_param(int param_num); 
static inline void load_temp_var(struct var_info * t_v_info, int reg_num);
static inline void store_temp_var(struct var_info * t_v_info, int reg_num);
static inline void load_global_var(struct var_info * g_v_info, int reg_num);
static inline void store_global_var(struct var_info * g_v_info, int reg_num);
static int gen_tempreg(int * except, int size);
static inline void restore_tempreg(int temp_reg);
static inline void load_var(struct var_info * v_info, int reg_num);
static inline void store_var(struct var_info * v_info, int reg_num);
static inline void load_pointer(int var_index, int reg_num, int imm_offset, int reg_offset);

/****************************** initial begin ***************************/
static inline void set_cur_function(int func_index)
{
	int index;
	cur_table = table_list[func_index];
	cur_func_info = symt_search(simb_table ,table_list[func_index] -> funcname);//the 
	cur_var_id_num = table_list[func_index] -> var_id_num;
	cur_ref_var_num = get_ref_var_num();//the varmapping is always right during this file
	cur_func_index = func_index;
	
	int string_num = get_localstr_num(cur_func_info -> func_symt);//get string num from the symtable
	int padding = 10;//just in case
	global_var_label_offset = calloc(g_global_id_num + string_num + padding, sizeof(int));//need free
	ref_g_var_num = 0;
	global_var_label = gen_new_label(total_label_num ++);//as the head element of the global var
	cur_sp = 0;//just point to bp
	local_var_sp = 0;

	cur_code_index = 0;
	cur_code_bound = INITIAL_MACH_CODE_SIZE; 
	code_table_list[func_index].table = calloc(cur_code_bound, sizeof(struct mach_code));  
	code_table_list[func_index].code_num = 0;
	for(index = 0; index < 31; index ++)
		reg_dpt[index].content = REG_UNUSED;
	null.type = Unused;//used for unused arg
	arglist_num_mark = 0;
}

static inline void leave_cur_function()
{
	if(global_var_label_offset != NULL)
		free(global_var_label_offset);
	if(global_var_label != NULL)
		free(global_var_label);
	global_var_label_offset = NULL;
	global_var_label = NULL;
	free(defed_gvar);
	defed_gvar = NULL;
}
/*************************** initial end *******************************/

/*************************** insert code begin *************************/
static inline int insert_code(struct mach_code newcode)
{
    if(cur_code_index >= cur_code_bound)
    {
        struct mach_code * tmp_table_list = calloc(2 * cur_code_bound, sizeof(struct mach_code));
        memcpy(tmp_table_list, code_table_list[cur_func_index].table, sizeof(struct mach_code) * cur_code_bound); 
        cur_code_bound *= 2;
        free(code_table_list[cur_func_index].table);
		code_table_list[cur_func_index].table = tmp_table_list;
    }
	newcode.optimize = 0;
	code_table_list[cur_func_index].code_num ++;
	code_table_list[cur_func_index].table[cur_code_index++] = newcode;
#ifdef MACH_DEBUG
	mcode_out(&newcode, stdout);
#endif
	return cur_code_index - 1;
}

static inline void insert_label_code(char * label_name)
{
	struct mach_code new_code;
	new_code.op_type = LABEL;
	new_code.label = label_name;
	insert_code(new_code);
}

static inline void insert_dp_code(enum dp_op_type dp_op, int dest, struct mach_arg arg1, struct mach_arg arg2, unsigned int arg3, enum shift_type shift)
{
	struct mach_code new_code;
	new_code.op_type = DP;
	new_code.dp_op = dp_op;
	new_code.dest = dest;
	new_code.arg1 = arg1;
	new_code.arg2 = arg2;
	new_code.arg3 = arg3;
    new_code.shift = shift;
	insert_code(new_code);
}

static inline void insert_cond_dp_code(enum dp_op_type dp_op, int dest, enum condition_type cond, struct mach_arg arg2, unsigned int arg3, int sign, enum shift_type shift)//may not be used
{
	struct mach_code new_code;
	new_code.op_type = DP;
	new_code.dp_op = dp_op;
	new_code.dest = dest;
	new_code.cond = cond;
	new_code.arg2 = arg2;
	new_code.arg3 = arg3;
	new_code.shift = shift;
	insert_code(new_code);
}

static inline void insert_mem_code(enum mem_op_type mem_op, int dest, struct mach_arg arg1, struct mach_arg arg2, unsigned int arg3, int offset, enum shift_type shift, enum index_type indexed)
{
	struct mach_code new_code;
	new_code.op_type = MEM;
	new_code.mem_op = mem_op;
	new_code.dest = dest;
	new_code.arg1 = arg1;
	new_code.arg2 = arg2;
	new_code.arg3 = arg3;
	new_code.offset = offset;
	new_code.shift = shift;
	new_code.indexed = indexed;
	insert_code(new_code);	
}

static inline void insert_cmp_code(struct mach_arg arg1, struct mach_arg arg2)
{
	struct mach_code new_code;
	new_code.op_type = CMP;
	new_code.cmp_op = CMPSUB_A;
	new_code.arg1 = arg1;
	new_code.arg2 = arg2;
	new_code.arg3 = 0;
	new_code.shift = NO;
	insert_code(new_code);
}

static inline void insert_jump_code(int dest)
{
	struct mach_code new_code;
	new_code.op_type = BRANCH;
	new_code.branch_op = JUMP;
	new_code.dest = dest;
	insert_code(new_code);
}

static inline void insert_bcond_code(char * dest_label, enum condition_type cond)
{
	struct mach_code new_code;
	new_code.op_type = BRANCH;
	new_code.branch_op = BCOND;
	new_code.dest_label = dest_label;
	new_code.cond = cond;
	insert_code(new_code);
}

static inline void insert_buncond_code(char * dest_label, char link)
{
    struct mach_code new_code;
	new_code.op_type = BRANCH;
	new_code.branch_op = B;
	new_code.dest_label = dest_label;
	new_code.link = link;
	insert_code(new_code);
}

static inline void set_optmz(int index , char is_optmz)
{
	code_table_list[cur_func_index].table[index].optimize = is_optmz;
}
/*************************** insert code over *************************/


/*******************************一些更加细致的指令生成代码***********************************/
/*MOV rd , rs*/
static inline void gen_mov_rsrd_code(int rd , int rs)
{
     struct mach_arg src_arg;
     src_arg.type = Mach_Reg;
     src_arg.reg = rs;
     insert_dp_code(MOV , rd , null , src_arg , 0 , NO);
}

/*MOV rd , #imme*/
static inline void gen_mov_rsim_code(int rd , int imme)
{
     struct mach_arg src_arg;
     src_arg.type = Mach_Imm;
     src_arg.imme = imme;
     insert_dp_code(MOV , rd , null , src_arg , 0 , NO);
}

/*MOV rd , rs << #imme*/
static inline void gen_mov_lshft_code(int rd , int rs , int imme)
{
     struct mach_arg src_arg;
     src_arg.type = Mach_Reg;
     src_arg.reg = rs;
     insert_dp_code(MOV , rd , src_arg , null , imme , LL);
}

/*RSUB rd , rs1 , #imme*/
static inline void gen_rsub_rri_code(int rd , int rs1 , int imme)
{
     struct mach_arg arg1 , arg2;
     arg1.type = Mach_Reg;
     arg1.reg = rs1;
     arg2.type = Mach_Imm;
     arg2.imme = imme;
     insert_dp_code(RSUB , rd , arg1, arg2, 0 , NO);
}

/*MEM rd , [rs1(off)] , rs2*/
static inline void gen_mem_rrr_code(enum mem type , int rd , int rs1 , int off , int rs2 , int width)//**************
{
     enum mem_op_type mem_op;
     if(type == load)
     {
          if(width == 1)
               mem_op = LDB;
          else
               mem_op = LDW;
     }
     else
     {
          if(width == 1)
               mem_op = STB;
          else
               mem_op = STW;
     }
     struct mach_arg arg1 , arg2;
     arg2.type = arg1.type = Mach_Reg;
     arg1.reg = rs1;
     arg2.reg = rs2;
     insert_mem_code(mem_op , rd , arg1 , arg2 , 0 , off , NO, 0);
}

/*LDB/LDW rd , [rs1(off)] , rs2<<imme*/
static inline void gen_mem_lshft_code(enum mem type , int rd, int rs1, int off, int rs2, int imme, int width)
{
     enum mem_op_type mem_op;
     if(type == load)
     {
          if(width == 1)
               mem_op = LDB;
          else
               mem_op = LDW;
     }
     else
     {
          if(width == 1)
               mem_op = STB;
          else
               mem_op = STW;
     }
     struct mach_arg arg1 , arg2;
     arg2.type = arg1.type = Mach_Reg;
     arg1.reg = rs1;
     arg2.reg = rs2;
     insert_mem_code(mem_op , rd , arg1 , arg2 , imme , off , LL, 0);
}

/*LDB/LDW rd , [rs(off)] , #imme*/
static inline void gen_mem_rri_code(enum mem type , int rd , int rs1 , int off , int imme , int width)
{
     enum mem_op_type mem_op;
     if(type == load)
     {
          if(width == 1)
               mem_op = LDB;
          else
               mem_op = LDW;
     }
     else
     {
          if(width == 1)
               mem_op = STB;
          else
               mem_op = STW;
     }
     struct mach_arg arg1 , arg2;
     arg1.type = Mach_Reg;
     arg2.type = Mach_Imm;
     arg1.reg = rs1;
     arg2.imme = imme;
     insert_mem_code(mem_op , rd , arg1 , arg2 , 0 , off , NO, 0);
}

/*LDB/LDW/STB/STW rd , [rs]*/
static inline void gen_mem_rr_code(enum mem type , int rd, int rs1, int width)
{
     enum mem_op_type mem_op;
     if(type == load)
     {
          if(width == 1)
               mem_op = LDB;
          else
               mem_op = LDW;
     }
     else
     {
          if(width == 1)
               mem_op = STB;
          else
               mem_op = STW;
     }
     struct mach_arg arg1;
     arg1.type = Mach_Reg;
     arg1.reg = rs1;
     insert_mem_code(mem_op , rd , arg1 , null , 0 , 0 , NO, 0);
}

/*OP rd , rs , #imme*/
static inline void gen_dp_rri_code(enum dp_op_type op , int rd , int rs , int imme)
{
     struct mach_arg arg1 , arg2;
     arg2.type = Mach_Imm;
     arg2.imme = imme;
     arg1.type = Mach_Reg;
     arg1.reg = rs;
     insert_dp_code(op, rd, arg1, arg2, 0, NO);
}


/****************** deal with global var begin ************************/
static inline int ref_global_var(int g_var_index)//get the global var offset, gen addr when used
{
	struct var_info * tmp_info = get_info_from_index(g_var_index);
	if(tmp_info -> mem_addr == INITIAL_MEM_ADDR)//var_id = var_index for global var
	{
		tmp_info -> mem_addr = 4 * ref_g_var_num;
		global_var_label_offset[ref_g_var_num ++] = g_var_index;
	}
	return tmp_info -> mem_addr; 
}

static inline char * gen_new_var_offset(int offset)//need free later
{
	if(offset < 0)
		fprintf(stderr, "error when gen label for global var\n");
	char label_name[MAX_LABEL_NAME_LEN];
	strcpy(label_name, global_var_label);
	if(offset > 0)
	{
		char label_num_name[MAX_LABEL_NAME_LEN];
		sprintf(label_num_name, "%d", offset);
		strcat(label_name, "+");
		strcat(label_name, label_num_name);
	}
	return strdup(label_name);
}
/******************** deal with global var end ************************/

/******************** deal with temp var begin ************************/
static void prepare_temp_var_inmem()//gen addr at first
{
	int index;
	struct var_info * tmp_v_info;
	int offset_from_sp = 0;//cur_sp should sp - fp
	
	int param_count = get_param_counts(cur_func_info -> func_symt);
	int param_rank;
	local_var_sp = cur_sp;
	for(index = 0; index < cur_ref_var_num; index ++)//first scan allocate memory, and register it in varmapping
	{
		if(index < cur_var_id_num || alloc_reg.result[index] == -1)
		{
			if(!is_global(index))
			{
				tmp_v_info = get_info_from_index(index);		
				if(index < cur_var_id_num)
				{
					struct value_info * id_info = get_valueinfo_byno(cur_func_info->func_symt ,  index);
					if(id_info -> type -> type == Array)
					{
						int size = id_info -> type -> size;
						if(id_info -> type -> base_type -> type == Char)
							offset_from_sp += (BYTE * size);
						else
						{
							offset_from_sp += (WORD - 1);
							offset_from_sp -= (offset_from_sp % WORD);
							offset_from_sp += (WORD * size);
						}
						tmp_v_info -> mem_addr = cur_sp + offset_from_sp;	
						continue;
					}
					if(is_arglist_byno(cur_func_info -> func_symt, index))
					{
						param_rank = get_arglist_rank(cur_func_info -> func_symt, index);//MARK TAOTAOTHERIPPER	
						if(param_rank <= param_count - 4)
						{
							tmp_v_info -> mem_addr = (- param_rank) * WORD;
							continue;
						}//the first four params' stack memory is allocated in current stack frame
					}
				}
				if(get_width_from_index(index) == BYTE)
					offset_from_sp ++;
				else
				{
					offset_from_sp += WORD;
					if(offset_from_sp % WORD != 0)
						offset_from_sp = offset_from_sp - offset_from_sp %WORD + WORD;
				}
				tmp_v_info -> mem_addr = cur_sp + offset_from_sp;	
			}
		}
	}
	offset_from_sp += (WORD * get_max_func_varlist());
	caller_save_index = offset_from_sp;/* the caller save register should be stored from low addr to high addr */
	cur_sp += offset_from_sp;/* in fact only record how many ids are saved in mem */
	
	if(offset_from_sp > 0)//sub sp
	{
		struct mach_arg tmp_sp, tmp_offset;
		tmp_sp.type = Mach_Reg;
		tmp_sp.reg = REG_SP;
		tmp_offset.type = Mach_Imm;
		tmp_offset.imme = offset_from_sp;
		insert_dp_code(SUB, REG_SP, tmp_sp, tmp_offset, 0, NO);
	}
	/* just here */ /* TAOTAOTHERIPPER MARK */
	struct var_list_node * cur = begin_var_list.head;
	int id_index;
	struct var_info * id_info;
	while(cur != NULL && cur != begin_var_list.tail -> next)
	{
		id_index = cur -> var_map_index;
		id_info = get_info_from_index(id_index);
		if(alloc_reg.result[id_index] != -1)
		{
			if(is_array(id_index) || is_conststr_byno(cur_func_info -> func_symt, id_index))
			{
				load_pointer(id_index, alloc_reg.result[id_index], 0, -1);
				id_info -> reg_addr = alloc_reg.result[id_index];
				reg_dpt[alloc_reg.result[id_index]].content = id_index;
				cur = cur -> next;
				continue;
			}
			if(is_global(id_index))
			{
				ref_global_var(id_index);//global var prepared when first used
				load_var(id_info, alloc_reg.result[id_index]);
				id_info -> reg_addr = alloc_reg.result[id_index];
				reg_dpt[alloc_reg.result[id_index]].content = id_index;
				cur = cur -> next;
				continue;
			}
			if(is_arglist_byno(cur_func_info -> func_symt, id_index))
			{
				param_rank = get_arglist_rank(cur_func_info -> func_symt, id_index);
				if(param_rank > param_count - 4)
					gen_mov_rsrd_code(alloc_reg.result[id_index], (param_count - param_rank));
				else
					load_var(id_info, alloc_reg.result[id_index]);
				reg_dpt[alloc_reg.result[id_index]].content = id_index;
				id_info -> reg_addr = alloc_reg.result[id_index];
			}
		}
		else
		{
			if(is_arglist_byno(cur_func_info -> func_symt, id_index))
			{
				if(is_active_var(id_index))//may be the param is in mem because it has never been refed in the function
					store_var(id_info, (param_count - param_rank));
			}
		}
		cur = cur -> next;
	}
}

static void free_temp_var_inmem()
{
	if((cur_sp - local_var_sp) > 0)//sub sp
	{
		struct mach_arg tmp_sp, tmp_offset;
		tmp_sp.type = Mach_Reg;
		tmp_sp.reg = REG_SP;
		tmp_offset.type = Mach_Imm;
		tmp_offset.imme = cur_sp - local_var_sp;
		insert_dp_code(ADD, REG_SP, tmp_sp, tmp_offset, 0, NO);
	}
}

static void enter_func_push()
{
	/* 
	   push fp	; so that the param index should begin from 4
	   mov fp, sp
	   push lr
	*/	
	push_param(REG_FP);
	gen_mov_rsrd_code(REG_FP, REG_SP);
	cur_sp = 0;
	push_param(REG_LR);
	cur_sp += WORD;
}

static void leave_func_pop()
{
	pop_param_to_reg(REG_LR);
	pop_param_to_reg(REG_FP);
}

static void callee_save_push()
{
	memset(used_reg, 0, 32 * sizeof(char));
	int idx;
	for(idx = 0; idx < cur_ref_var_num; idx ++)
	{
		if(alloc_reg.result[idx] != -1)
			used_reg[alloc_reg.result[idx]] = 1;
	}
	for(idx = 17; idx <= 25; idx ++)
	{
		if(used_reg[idx] == 1)
			push_param(idx);
	}
}

static void callee_save_pop()
{
	int idx;
	for(idx = 25; idx >= 17; idx --)
	{
		if(used_reg[idx] == 1)
			pop_param_to_reg(idx);
	}
}

/******************** deal with temp var end **************************/

/******************** deal with label begin *****************************/
static inline int check_is_jump_dest(int exprnum)//need check every expr before translate
{
	struct var_info * expr_info = get_info_of_temp_for_label(exprnum);
	if(expr_info != NULL)
	{
		if(expr_info -> label_num != -1)
		{
			if(expr_info -> label_num == -2)//means this is a dest
				expr_info -> label_num = total_label_num ++;
			insert_label_code(gen_new_label(expr_info -> label_num));
			return 1;
		}
	}
	return 0;
}

static inline char * gen_new_label(int label_num)
{
	char label_name[MAX_LABEL_NAME_LEN] = ".L";
	char label_num_name[MAX_LABEL_NAME_LEN];
	sprintf(label_num_name, "%d", label_num);
	strcat(label_name, label_num_name);
	return strdup(label_name);//but need to free later
}

static inline int ref_jump_dest(int expr_id)//get the label for jump dest
{	
	struct var_info * id_info = get_info_of_temp_for_label(expr_id);
	if(id_info == NULL)
	{
		fprintf(stderr, "error when ref jump destination.\n");
		exit(1);
	}
	if(id_info -> label_num == -1)
		fprintf(stderr, "error jump to an wrong destination.\n");
	if(id_info -> label_num == -2)
		id_info -> label_num = total_label_num ++;
	return id_info -> label_num;
}
/************************** deal with label end *************************/


/************************** get pointer begin **************************/
static inline void load_pointer(int var_index, int reg_num, int imm_offset, int reg_offset)
{
	/*
		if imm_offset == 0 and reg_offset == -1
			normal;
		if imm_offset == 0 and reg_offset != -1
			for global
				LDW and ADD
			for temp
				SUB
		if imm_offset != 0 and reg_offset == -1
			for global
				LDW and ADD
			for temp
				SUB and ADD
	*/
	struct var_info * id_info = get_info_from_index(var_index);
	struct mach_arg src_reg, offset_arg;
	src_reg.type = Mach_Reg;
	src_reg.reg = reg_num;
	if(is_global(var_index))
	{
		ref_global_var(var_index);//MARK TAOTAOTHERIPPER, do nothing but allocate a label for used global var
									/* the var is always prepared, except load_pointer. */
		int id_offset = id_info -> mem_addr;
		struct mach_arg address;
		address.type = Mach_Label;
		address.label = gen_new_var_offset(id_offset);
		insert_mem_code(LDW, reg_num, address, null, 0, 0, NO, 0);	
		if(imm_offset != 0)
		{
			offset_arg.type = Mach_Imm;
			offset_arg.imme = imm_offset;
			if(get_stride_from_index(var_index) == WORD)
				offset_arg.imme *= WORD;
			else
				offset_arg.imme *= BYTE;
			insert_dp_code(ADD, reg_num, src_reg, offset_arg, 0, NO);
		}
		else if(reg_offset != -1)
		{
			offset_arg.type = Mach_Reg;
			offset_arg.reg = reg_offset;
			if(get_stride_from_index(var_index) == WORD)
				insert_dp_code(ADD, reg_num, src_reg, offset_arg, 2, LL);
			else insert_dp_code(ADD, reg_num, src_reg, offset_arg, 0, NO);
		}
	}
	else
	{
		struct mach_arg tmp_fp;
		tmp_fp.type = Mach_Reg;
		tmp_fp.reg = REG_FP;
		offset_arg.type = Mach_Imm;
		offset_arg.imme = id_info -> mem_addr;
		if(imm_offset != 0)
		{
			if(get_stride_from_index(var_index) == WORD)
				offset_arg.imme = offset_arg.imme - WORD * imm_offset;	
			else
				offset_arg.imme = offset_arg.imme - imm_offset;
			insert_dp_code(SUB, reg_num, tmp_fp, offset_arg, 0, NO);
		}
		else if(reg_offset != -1)
		{
			struct mach_arg reg_arg;
			reg_arg.type = Mach_Reg;
			reg_arg.imme = reg_offset;
			int shift_bits;
			enum shift_type tmp_shift_type;
			if(get_stride_from_index(var_index) == WORD)
			{
				shift_bits = 2;
				tmp_shift_type = LL;
			}
			else
			{
				shift_bits = 0;
				tmp_shift_type = NO;
			}
			insert_dp_code(SUB, reg_num, tmp_fp, offset_arg, 0, NO);
			insert_dp_code(ADD, reg_num, src_reg, reg_arg, shift_bits, tmp_shift_type);	
		}
		else
			insert_dp_code(SUB, reg_num, tmp_fp, offset_arg, 0, NO);	
	}
}
/************************** get pointer end ****************************/

/**************************** load store var beg *****************************/

static inline void push_param(int reg_num)
{	
	struct mach_arg tmp_sp, tmp_offset;
	tmp_sp.type = Mach_Reg;
	tmp_sp.reg = REG_SP;
	tmp_offset.type = Mach_Imm;
	tmp_offset.imme = WORD;
	insert_mem_code(STW, reg_num, tmp_sp, tmp_offset, 0, -1, NO, PREW);	
}

static inline void pop_param_to_reg(int reg_num)
{
	struct mach_arg tmp_sp, tmp_offset;
	tmp_sp.type = Mach_Reg;
	tmp_sp.reg = REG_SP;
	tmp_offset.type = Mach_Imm;
	tmp_offset.imme = WORD;
	insert_mem_code(LDW, reg_num, tmp_sp, tmp_offset, 0, 1, NO, POST);
}

static inline void pop_param(int param_num) 
{
	struct mach_arg tmp_sp, tmp_offset;
	tmp_sp.type = Mach_Reg;
	tmp_sp.reg = REG_SP;
	tmp_offset.type = Mach_Imm;
	tmp_offset.imme = WORD * param_num;
	insert_dp_code(ADD, REG_SP, tmp_sp, tmp_offset, 1, NO);
}

static inline void load_var(struct var_info * v_info, int reg_num)
{
	if(is_global(v_info -> index))
		load_global_var(v_info, reg_num);
	else
		load_temp_var(v_info, reg_num);
}

static inline void store_var(struct var_info * v_info, int reg_num)
{
	if(is_global(v_info -> index))
		store_global_var(v_info, reg_num);
	else
		store_temp_var(v_info, reg_num);
}

static inline void load_temp_var(struct var_info * t_v_info, int reg_num)
{
	struct mach_arg tmp_fp, tmp_offset;
	tmp_fp.type = Mach_Reg;
	tmp_fp.reg = REG_FP;//need width later
	tmp_offset.type = Mach_Imm;
	tmp_offset.imme = t_v_info -> mem_addr;
	int width = get_width_from_index(t_v_info -> index);
	enum mem_op_type tmp_mem_op;
	if(width == BYTE)
		tmp_mem_op = LDB;
	else tmp_mem_op = LDW;
	insert_mem_code(tmp_mem_op, reg_num, tmp_fp, tmp_offset, 0, -1, NO, 0);
}

static inline void store_temp_var(struct var_info * t_v_info, int reg_num)
{
	struct mach_arg tmp_fp, tmp_offset;
	tmp_fp.type = Mach_Reg;
	tmp_fp.reg = REG_FP;//need width later
	tmp_offset.type = Mach_Imm;
	tmp_offset.imme = t_v_info -> mem_addr;
	int width = get_width_from_index(t_v_info -> index);
	enum mem_op_type tmp_mem_op;
	if(width == BYTE)
		tmp_mem_op = STB;
	else tmp_mem_op = STW;	
	insert_mem_code(tmp_mem_op, reg_num, tmp_fp, tmp_offset, 0, -1, NO, 0);
}

static inline void load_global_var(struct var_info * g_v_info, int reg_num)
{
	struct value_info * tmp_info = get_valueinfo_byno(cur_func_info -> func_symt, g_v_info -> index); 
	int offset = g_v_info -> mem_addr;
	struct mach_arg address, dest_reg;
	address.type = Mach_Label;
	address.label = gen_new_var_offset(offset);
	dest_reg.type = Mach_Reg;
	dest_reg.reg = reg_num;
	insert_mem_code(LDW, reg_num, address, null, 0, 0, NO, 0);
	if(tmp_info -> type -> type == Char)
		insert_mem_code(LDB, reg_num, dest_reg, null, 0, 0, NO, 0);
	else insert_mem_code(LDW, reg_num, dest_reg, null, 0, 0, NO, 0);
}

static inline void store_global_var(struct var_info * g_v_info, int reg_num)
{
	struct value_info * tmp_info = get_valueinfo_byno(cur_func_info -> func_symt, g_v_info -> index); 
	int offset = g_v_info -> mem_addr;
	int tmp_reg_num = gen_tempreg(&reg_num, 1);
	struct mach_arg address, temp_reg;
	address.type = Mach_Label;
	address.label = gen_new_var_offset(offset);
	temp_reg.type = Mach_Reg;
	temp_reg.reg = tmp_reg_num;
	insert_mem_code(LDW, tmp_reg_num, address, null, 0, 0, NO, 0);
	if(tmp_info -> type -> type == Char)
		insert_mem_code(STB, reg_num, temp_reg, null, 0, 0, NO, 0);
	else insert_mem_code(STW, reg_num, temp_reg, null, 0, 0, NO, 0);
	restore_tempreg(tmp_reg_num);
}
/**************************** load store var end *****************************/



/************************** get temp reg begin ***********************/
static int is_reg_disabled(int regnum)
{
	if(regnum < 4 || regnum == 16 || regnum == REG_FP || regnum >= REG_SP)
		return 1;
	return 0;
}

static int is_reg_saved(int regnum)
{
	if((regnum >= 4 && regnum <= 15) || regnum == 28)
		return 1;
	else if(used_reg[regnum] == 1)//saved callee save register
		return 1;
	else return 0;
}

static int gen_tempreg(int * except, int size)//general an temp reg for the var should be in memory
{
	int outlop, index, ex;
	for(outlop = 0; outlop < 6; outlop ++)
	{
		for(index = TOTAL_REG_NUM - 1; index >= 0; index --)//look for empty
		{
			if(is_reg_disabled(index))
				continue;
			for(ex = 0; ex < size; ex ++)
			{
				if(index == except[ex])
					break;
			}
			int reg_content  = reg_dpt[index].content;
			switch(outlop)
			{
				case 0:
					if(is_reg_saved(index) && reg_content == REG_UNUSED && ex == size)
					{
						shadow_reg_dpt[index] = reg_dpt[index];
						reg_dpt[index].content = REG_TEMP;//-2 means tmp_reg
						return index;
					}
					break;
				case 1:
					if((is_array(reg_content) || is_conststr_byno(cur_func_info -> func_symt, reg_content)) && ex == size)
					{
						shadow_reg_dpt[index] = reg_dpt[index];
						reg_dpt[index].content = REG_TEMP;
						return index;
					}
					break;
				case 2:
					if(!is_global(reg_content) && is_id_var(reg_content) && ex == size)
					{
						store_var(get_info_from_index(reg_content), index);
						shadow_reg_dpt[index] = reg_dpt[index];
						reg_dpt[index].content = REG_TEMP;
						return index;
					}
					break;
				case 3:
					if(!is_global(reg_content) && ex == size)//include the regs which haven't been saved
					{
						push_param(reg_content);
						shadow_reg_dpt[index] = reg_dpt[index];
						reg_dpt[index].content = REG_TEMP;
						return index;
					}
					break;
				case 4:
					if(ex == size)
					{
						store_global_var(get_info_from_index(reg_content), index);
						shadow_reg_dpt[index] = reg_dpt[index];
						reg_dpt[index].content = REG_TEMP;
						return index;
					}
					break;
				default:
					fprintf(stderr,"error when gen tempreg.\n");
					exit(1);
			}
		}
	}
	exit(1);
}

static inline void restore_tempreg(int temp_reg)
{
	if(temp_reg == -1)
	{
		/*fprintf(stderr, "error when restore tempreg invalid temp reg\n");*/
		return;
	}
	int reg_content = shadow_reg_dpt[temp_reg].content;
	if(is_reg_saved(temp_reg) && reg_content == REG_UNUSED)
		;	
	else if(is_array(reg_content) || is_conststr_byno(cur_func_info -> func_symt, reg_content))
		load_pointer(reg_content, temp_reg, 0, -1);
	else if(!is_global(reg_content) && is_id_var(reg_content))
		load_var(get_info_from_index(reg_content), temp_reg);
	else if(!is_global(reg_content))
		pop_param_to_reg(temp_reg);
	else
		load_global_var(get_info_from_index(reg_content), temp_reg);
	reg_dpt[temp_reg] = shadow_reg_dpt[temp_reg];
	shadow_reg_dpt[temp_reg].content = -1;
}
/*************************** get temp reg end ************************/

/*************************** prepare arg beg *****************************/
static inline void update_reg_content(struct triargexpr * expr)
{
	struct var_list * active_list = expr -> actvar_list;
	if(active_list == NULL)
		return;
	struct var_list_node * tmp_node = active_list -> head;
	struct var_info * tmp_info;
	while(tmp_node != NULL && tmp_node != active_list -> tail -> next)
	{
		if(alloc_reg.result[tmp_node -> var_map_index] != -1)
		{
			tmp_info = get_info_from_index(tmp_node -> var_map_index);
			reg_dpt[alloc_reg.result[tmp_node -> var_map_index]].content = tmp_node -> var_map_index;
			tmp_info -> reg_addr = alloc_reg.result[tmp_node -> var_map_index];
		}
		tmp_node = tmp_node -> next;
	}	
}


static inline enum Arg_Flag mach_prepare_arg(int arg_index, struct var_info * arg_info, int arg_type)/* arg_type : 0=>dest, 1=>normal *///ref_global_var!!!
{
	if(arg_index < 0)
	{
		fprintf(stderr, "error when prepare arg.\n");
		exit(1);
	}
	enum Arg_Flag flag;	
	if(is_global(arg_index))
		ref_global_var(arg_index);//global var prepared when first used
	
	if(alloc_reg.result[arg_index] != -1)/* for the refed var, it must be in the register now,
										  or else it is allocated in the memory. */
		flag = Arg_Reg;	
	else
		flag = Arg_Mem;
	return flag;
}

static inline enum Arg_Flag mach_prepare_index(int index , struct var_info **arg_info , int arg_type)
{
     (*arg_info) = NULL;
     if(index == -2)
          return Arg_Imm;
     (*arg_info) = get_info_from_index(index);
     return mach_prepare_arg(index , (*arg_info) , arg_type);
}

/************************ prepare arg end ********************************/


/******************** flush pointer entity beg ***************************/
static void flush_global_var()
{
	int index;
	struct var_info * v_info;
	int reg_content;
	for(index = 0; index < TOTAL_REG_NUM; index ++)
	{
		if(is_reg_disabled(index))
			continue;
		reg_content = reg_dpt[index].content;
		v_info = get_info_from_index(reg_content);
		if(!is_array(reg_content) && !is_conststr_byno(cur_func_info -> func_symt, reg_content) 
				&& is_global(reg_content))
			store_var(v_info, index);
	}
}

#if 0
/* the flush and reload just deal with the consitent between reg 
   and mem, so the clear global reg, the local reg and the temp 
   reg should be solved in caller save */

static void reload_global_var()
{
	int index;
	struct var_info * v_info;
	int reg_content;
	for(index = 0; index < TOTAL_REG_NUM; index ++)
	{
		if(is_reg_disabled(index))
			continue;
		reg_content = reg_dpt[index].content;
		v_info = get_info_from_index(reg_content);
		if(!is_array(reg_content) && !is_conststr_byno(cur_func_info -> func_symt, reg_content) 
				&& is_global(reg_content))
			load_var(v_info, index);
	}
}
#endif

static void flush_pointer_entity(enum mem type, struct var_list * entity_list)
{
	struct var_info * v_info;
	int index;
	int reg_content;
	if(entity_list == NULL)
		return;
	else if(entity_list -> head == NULL)/* if there is strange pointer, we just consider that the program is wrong */
	{
		/*for(index = 0; index < TOTAL_REG_NUM; index ++)
		{
			if(is_reg_disabled(index))
				continue;
			reg_content = reg_dpt[index].content;
			v_info = get_info_from_index(reg_content);
			if(is_global(reg_dpt[index].content) && !is_array(reg_content) 
				&& !is_conststr_byno(cur_func_info -> func_symt, reg_content))
			{
				if(type == load)
					load_var(v_info, index);
				else
					store_var(v_info, index);
			}
		}*/
		;
	}
	else
	{
		struct var_list_node * tmp;
		tmp = entity_list -> head;
		while(tmp != entity_list -> tail -> next)
		{
			for(index = 0; index < TOTAL_REG_NUM; index ++)
			{
				if(is_reg_disabled(index))
					continue;
				reg_content = reg_dpt[index].content;
				if(reg_content  == tmp -> var_map_index)
				{
					v_info = get_info_from_index(tmp -> var_map_index);
					if(is_id_var(reg_dpt[index].content) && !is_array(reg_content) 
							&& !is_conststr_byno(cur_func_info -> func_symt, reg_content))
					{
						if(type == load)
							load_var(v_info, index);
						else
							store_var(v_info, index);
					}
				}
			}
			tmp = tmp -> next;
		}
	}
}
#if 0
static caller_save()

static flush_for_func(enum mem type, struct var_list * entity_list)
{
	struct var_info * v_info;
	int index;
	int reg_content;
	if(entity_list == NULL)
		return;
	else if(entity_list -> head == NULL)
	{
		for(index = 0; index < TOTAL_REG_NUM; index ++)
		{
			if(is_reg_disabled(index))
				continue;
			reg_content = reg_dpt[index].content;
			v_info = get_info_from_index(reg_content);
			if(is_global(reg_dpt[index].content) && !is_array(reg_content) 
				&& !is_conststr_byno(cur_func_info -> func_symt, reg_content))
			{
				if(type == load)
					load_var(v_info, index);
				else
					store_var(v_info, index);
			}
		}
	}
	else
	{
		struct var_list_node * tmp;
		tmp = entity_list -> head;
		while(tmp != entity_list -> tail -> next)
		{
			for(index = 0; index < TOTAL_REG_NUM; index ++)
			{
				if(is_reg_disabled(index))
					continue;
				reg_content = reg_dpt[index].content;
				if(reg_content  == tmp -> var_map_index)
				{
					v_info = get_info_from_index(tmp -> var_map_index);
					if(is_id_var(reg_dpt[index].content) && !is_array(reg_content) 
							&& !is_conststr_byno(cur_func_info -> func_symt, reg_content))
					{
						if(type == load)
							load_var(v_info, index);
						else
							store_var(v_info, index);
					}
				}
			}
			tmp = tmp -> next;
		}
	}
	while(focus != NULL && focus != expr -> actvar_list -> tail -> next)
                {
                    vinfo = get_info_from_index(focus->var_map_index);
                    if((vinfo->reg_addr >= 4 && vinfo->reg_addr <= 15) || vinfo->reg_addr == 28)
                    {
						/* the global var has been stored in flush global var, so here I
						   don't need to store the clear reg, just reload it later is OK. */
						if(is_array(focus->var_map_index) || is_conststr_byno(cur_func_info -> func_symt, focus->var_map_index))
							saved_reg[saved_reg_count++] = vinfo->reg_addr;
						else if(!is_global(focus->var_map_index))/* the array and string is clear global var, just store var */
						{
							if(is_id_var(focus->var_map_index))
								store_var(vinfo, vinfo -> reg_addr);
							else
								gen_mem_rri_code(store, vinfo -> reg_addr, REG_FP, -1, caller_save_index - saved_reg_count * WORD, WORD);
							saved_reg[saved_reg_count++] = vinfo->reg_addr;	
						}
                    }
					focus = focus -> next;
                }
}
#endif
/**************************** flush pointer entity end ***************************/


/************************************* gen code beg ******************************/

/*返回arg的map_id。其中，没有map_id的返回-1，立即数返回-2，Return -1这种情况返回-3*/
static inline int get_index_of_arg(struct triarg *arg)
{
     if(arg->type == IdArg)
          return (get_index_of_id(arg->idname));
     else if(arg->type == ExprArg)
     {
          if(arg->expr == -1)
               return -3;
          return (get_index_of_temp(arg->expr));
     }
     else
          return -2;
}

/* FIXME: Not sure if this is correct */
static inline enum Arg_Flag get_argflag(struct triarg *arg , struct var_info *arg_info)
{
     if(arg->type == ImmArg)
          return Arg_Imm;
     if(arg_info == NULL)
     {
          fprintf(stderr , "Error! At file minic_machinecode.c : get_argflag\n");
          return Arg_Mem;
     }
     if(arg_info->reg_addr >= 0)
          return Arg_Reg;
     return Arg_Mem;
}

static int get_reg_addr(struct var_info *info , enum Arg_Flag arg_flag)
{
     if(arg_flag != Arg_Reg)
          return -1;
     else
          return info->reg_addr;
}

/*******************************************针对三元式的代码生成函数***********************************************/
/*前一个比较数是立即数的话，将两个比较数互换，并且改变运算符。如果发现两个都是立即数，返回0；否则，返回1*/
static inline int prtrtm_cond_expr(struct triargexpr *cond_expr)
{
     if(cond_expr->arg1.type == ImmArg)
     {
          struct triarg temp_arg;
          memcpy(&temp_arg , &(cond_expr->arg1) , sizeof(struct triarg));//temp = arg1
          memcpy((&cond_expr->arg1) , &(cond_expr->arg2) , sizeof(struct triarg));//arg1 = arg2
          memcpy((&cond_expr->arg2) , &temp_arg , sizeof(struct triarg));//arg2 = arg1
          /*Eq<->Neq, Ge<->Nge, Le<->Nle*/
          switch(cond_expr->op)
          {
          case Eq :cond_expr->op = Eq ;break;
          case Neq:cond_expr->op = Neq;break;
          case Ge :cond_expr->op = Le ;break;
          case Nge:cond_expr->op = Nle;break;
          case Le :cond_expr->op = Ge ;break;
          case Nle:cond_expr->op = Nge;break;
          default :break;
          }
     }
     if(cond_expr->arg1.type == ImmArg)
     {
          fprintf(stderr , "Triarg Expression Error!\n    ");
          print_triargexpr(*cond_expr);
          return 0;
     }
     return 1;
}

/*生成CMPSUB.A指令，并将此过程中分配的临时寄存器的编号，从后往前放入数组restore_reg中*/
static inline void gen_cmp_code(struct triargexpr *cond_expr , int *restore_reg)
{
     /*遇到条件跳转的时候才生成比较语句，生成的步骤是：
       1、预处理条件三元式。调用prtrtm_cond_expr(cond_expr)，确保该条件三元式的立即数（如果有）是arg2；如果发现两个都是立即数，则报错；
       2、生成比较语句两个操作数的寄存器（立即数是-1）；
           <1>立即数则返回-1；
           <2>map_id分配寄存器的话返回该寄存器编号；
           <3>map_id在内存的话，调用gen_tempreg返回一个临时寄存器，并将此寄存器编号放入参数数组restore_reg中（注意是从后往前放的）。
       3、生成代码；
     */
     
     /*前一个比较数是立即数的话，将两个比较数互换，并且改变运算符*/
     if(prtrtm_cond_expr(cond_expr) == 0)
          return;

     /*开始生成CMPSUB.A arg1 , arg2*/
     int cond_arg_index[2] , i;
     cond_arg_index[0] = get_index_of_arg(&cond_expr->arg1);//非负数代表map_id，-1代表该编号没有被引用过，-2代表是立即数
     cond_arg_index[1] = get_index_of_arg(&cond_expr->arg2);
     enum Arg_Flag arg_flag[2];
     struct var_info *cond_arg_info[2];
     for(i = 0 ; i < 2 ; i ++)
          if(cond_arg_index[i] != -1)
               arg_flag[i] = mach_prepare_index(cond_arg_index[i] , &(cond_arg_info[i]) , 1);

     int arg_reg_index[2];
     restore_reg[0] = restore_reg[1] = -1;
     int except[2] , j , restore_reg_index = 1;
     for(i = 0 ; i < 2 ; i ++)
     {
          if(arg_flag[i] == Arg_Imm)//<1>立即数则返回-1；
          {
               arg_reg_index[i] = -1;
               continue;
          }
          if(arg_flag[i] == Arg_Reg)//<2>map_id分配寄存器的话返回该寄存器编号；
               arg_reg_index[i] = cond_arg_info[i]->reg_addr;
          else//<3>map_id在内存的话，调用gen_tempreg返回一个临时寄存器。
          {
               for(j = 0 ; j < i ; j++)
                    except[j] = arg_reg_index[j];
               for(; j < 2 ; j++)
               {
                    if(cond_arg_info[j] == NULL)
                         except[j] = -1;
                    else
                         except[j] = cond_arg_info[j]->reg_addr;
               }
               arg_reg_index[i] = gen_tempreg(except , 2);
               load_var(cond_arg_info[i] , arg_reg_index[i]);
               restore_reg[restore_reg_index--] = arg_reg_index[i];
          }
     }
     struct mach_arg arg1 , arg2;
     arg1.type = Mach_Reg;
     arg1.reg = arg_reg_index[0];
     if(cond_arg_index[1] == -2)//第二个是立即数
     {
          arg2.type = Mach_Imm;
          arg2.imme = cond_expr->arg2.imme;
     }
     else
     {
          arg2.type = Mach_Reg;
          arg2.reg = arg_reg_index[1];
     }
     insert_cmp_code(arg1 , arg2);
}

/*生成条件跳转的汇编代码*/
static inline void gen_cj_expr(struct triargexpr *expr)
{
     /*
       生成条件跳转的汇编代码。步骤如下：（默认条件为常数的已经优化过了）
       1、如果条件操作数没有map_id，说明它是个逻辑表达式。此时则：
           <1>获得该条件对应的三元式，调用gen_cmp_code生成条件指令、填充临时寄存器编号；
           <2>生成label和跳转指令（默认arg2不是立即数和变量）；
           <3>调用restore_tempreg恢复临时寄存器。
       2、如果条件操作数有map_id，说明它是个变量抑或算数运算式。此时则：
           <1>生成三元式（Neq expr->arg1 , 0），调用gen_cmp_code生成条件指令、填充临时寄存器编号；
           <2>生成label和跳转指令（默认arg2不是立即数和变量）；
           <3>调用restore_tempreg恢复临时寄存器。
      */
     int arg1_index = get_index_of_arg(&(expr->arg1));
     if(arg1_index == -1)//如果条件操作数没有map_id，说明它是个逻辑表达式。
     {
          /*先得到作为条件的三元式，生成它的代码。*/
          struct triargexpr_list *cond_expr_node = cur_table->index_to_list[expr->arg1.expr];
          struct triargexpr *cond_expr = get_tae_entity(cond_expr_node, cur_func_index);
          int restore_reg[2] , i;
          gen_cmp_code(cond_expr , restore_reg);
          
          /*生成label，并且生成跳转代码*/
          int label_num = ref_jump_dest(expr->arg2.expr);
          char *dest_label_name = gen_new_label(label_num);
          enum condition_type cond_type;
          if(expr->op == TrueJump)
          {
               switch(cond_expr->op)
               {
               case Eq :cond_type = Mach_EQ;break;
               case Neq:cond_type = Mach_NE;break;
               case Ge :cond_type = Mach_EG;break;
               case Nge:cond_type = Mach_SL;break;
               case Le :cond_type = Mach_EL;break;
               case Nle:cond_type = Mach_SG;break;
               default :break;
               }
          }
          else
          {
               switch(cond_expr->op)
               {
               case Eq :cond_type = Mach_NE;break;
               case Neq:cond_type = Mach_EQ;break;
               case Ge :cond_type = Mach_SL;break;
               case Nge:cond_type = Mach_EG;break;
               case Le :cond_type = Mach_SG;break;
               case Nle:cond_type = Mach_EL;break;
               default :break;
               }
          }
          insert_bcond_code(dest_label_name , cond_type);
          
          for(i = 0 ; i < 2 ; i ++)/*恢复临时寄存器*/
               restore_tempreg(restore_reg[i]);
     }
     else
     {
          struct var_info *arg1_info = get_info_from_index(arg1_index);
          mach_prepare_arg(arg1_index, arg1_info, 1);//**不可能是立即数而且被引用了，肯定有map_id
          /*生成CMPSUB.A expr->arg1 , #0。先生成临时三元式Neq expr->arg1 , #0*/
          struct triargexpr cond_expr;
          cond_expr.op = Neq;
          memcpy(&(cond_expr.arg1) , &(expr->arg1) , sizeof(struct triarg));//cond_expr.arg1 = expr->arg1
          cond_expr.arg2.type = ImmArg;
          cond_expr.arg2.imme = 0;
          int restore_reg[2] , i;
          gen_cmp_code(&cond_expr , restore_reg);//*************error
          
          /*TrueJump则是BNE LABEL；FalseJump则是BEQ LABEL*/
          int label_num = ref_jump_dest(expr->arg2.expr);
          char *dest_label_name = gen_new_label(label_num);
          enum condition_type cond_type;
          if(expr->op == TrueJump)
               cond_type = Mach_NE;
          else
               cond_type = Mach_EQ;
          insert_bcond_code(dest_label_name , cond_type);
          
          for(i = 0 ; i < 2 ; i ++)/*恢复临时寄存器*/
               restore_tempreg(restore_reg[i]);
     }
}

/*
  数组下标代码生成。步骤如下：
  1、计算寻址步长。char数组为1，其他为4；
  2、取得目的数的寄存器，在内存中的话要申请临时寄存器；
  3、寻址载入目的数；（因为数组一般会循环使用，此处重点优化）
      载入方式：
      a)、arg1是全局数组（如果没有第一步的优化，意味着每次使用全局数组都要去load，太浪费了）
          <1>如果arg1在寄存器内，这意味着进入函数的时候数组首地址已经被放到了该寄存器里面，直接使用该寄存器变址寻址；
          <2>如果arg1在内存，调用函数获得首地址，然后使用寄存器变址寻址。
      b)、arg1是局部数组（局部数组对应var_info里面，记录了其首地址对fp的偏移）
          <1>arg2是立即数的话，直接根据数组首地址计算出相对fp的偏移，利用fp寄存器相对寻址；
          <2>arg2是寄存器或内存的话先把它左移，然后移入目的寄存器里，利用fp寄存器变址寻址。
  4、恢复所有临时寄存器，注意顺序。
*/
static int gen_array_code(enum mem type, struct triargexpr *expr, struct var_info *dest_info, enum Arg_Flag dest_flag, int imme)
{
     int width_shift = 2;
     struct var_info *arg1_info , *arg2_info;
     int arg1_index = get_index_of_arg(&(expr->arg1));
     int arg2_index = get_index_of_arg(&(expr->arg2));
     enum Arg_Flag arg1_flag = mach_prepare_index(arg1_index , &arg1_info , 1);
     enum Arg_Flag arg2_flag = mach_prepare_index(arg2_index , &arg2_info , 1);
     /*计算指针寻址的步长*/
     if(get_stride_from_index(arg1_index) == 1)
          width_shift = 0;
     
     /*获得arg2的寄存器*/
     int arg1_reg = get_reg_addr(arg1_info , arg1_flag);
     int arg2_reg = get_reg_addr(arg2_info , arg2_flag);
     int dest_reg = get_reg_addr(dest_info , dest_flag);

     int temp_reg[4] , temp_reg_size = -1 , i;
     for(i = 0 ; i < 4 ; i++)
          temp_reg[i] = -1;
     int except[4];
     except[0] = dest_reg;
     except[1] = arg1_reg;
     except[2] = arg2_reg;
     except[3] = -1;
     
     if(dest_flag != Arg_Reg)
     {
          dest_reg = gen_tempreg(except , 4);
          if(type == store)
          {
               if(dest_flag == Arg_Imm)
                    gen_mov_rsim_code(dest_reg , imme);
               else
                    load_var(dest_info , dest_reg);
          }
     }
     except[0] = dest_reg;
     
     if(arg2_flag == Arg_Mem)
     {
          temp_reg[++ temp_reg_size] = arg2_reg = gen_tempreg(except , 4);
          int arg2_width = get_width_from_index(arg2_index);
          gen_mem_rri_code(load , arg2_reg , REG_FP , -1 , arg2_info->mem_addr , arg2_width);
     }
     except[2] = arg2_reg;
     
     /*寻址载入目的数；*/
     if(is_global(arg1_index) == 1)//arg1是全局数组
     {
          if(arg1_flag == Arg_Reg)//如果arg1在寄存器内，这意味着进入函数的时候数组首地址已经被放到了该寄存器里面，直接使用该寄存器寻址；
          {
               if(arg2_flag == Arg_Imm)//MEM dest_reg , [arg1_reg+] , (expr->arg2.imme)<<width_shift
                    gen_mem_rri_code(type, dest_reg, arg1_reg, 1, (expr->arg2.imme) << width_shift, 1 << width_shift);
               else//MEM dest_reg , [arg1_reg+] , arg2_reg << #width_shift
                    gen_mem_lshft_code(type , dest_reg , arg1_reg , 1 , arg2_reg , width_shift , 1 << width_shift);
			   
			   /*对于全局数组，如果目的操作数和数组名变量都在寄存器，而且下标不在内存中，可能可以优化*/
			   if(dest_flag == Arg_Reg && arg2_flag != Arg_Mem)
			   {
				   struct var_info *expr_info = get_info_from_index(expr->index);
				   if(expr_info != NULL && expr_info->ref_mark == 0)
					   set_optmz(cur_code_index - 1 , optmz);
			   }
		  }
          else
          {
               arg1_reg = gen_tempreg(except , 4);
               temp_reg[++ temp_reg_size] = arg1_reg;
               except[1] = arg1_reg;
               load_pointer(arg1_index , arg1_reg , 0 , -1);//将全局变量首地址载入临时寄存器
               
               if(arg2_flag == Arg_Imm)//MEM dest_reg , [arg1_reg+] , (expr->arg2.imme)<<width_shift
                    gen_mem_rri_code(type , dest_reg , arg1_reg , 1 , (expr->arg2.imme)<<width_shift, 1 << width_shift);
               else//MEM dest_reg , [arg1_reg+] , arg2_reg << #width_shift
                    gen_mem_lshft_code(type , dest_reg , arg1_reg , 1 , arg2_reg , width_shift , 1 << width_shift);
          }
		  
     }
     else if(is_array(arg1_index) == 1)//arg1是局部数组
     {
          if(arg2_flag == Arg_Imm)//MEM dest_reg , [fp-] , arg1_info->mem_addr + expr->arg2.imme << width_shift
          {
               int fp_off = arg1_info->mem_addr;
               fp_off -= (expr->arg2.imme << width_shift);
               gen_mem_rri_code(type , dest_reg , REG_FP , -1 , fp_off , 1 << width_shift);
          }
          else//arg2是寄存器或内存的话先把它左移，然后移入寄存器里，利用fp寄存器变址寻址
          {
               int addr_reg = dest_reg;
               if(type == store)
               {
                    addr_reg = gen_tempreg(except , 4);
                    temp_reg[++ temp_reg_size] = addr_reg;
                    except[3] = addr_reg;
               }
               //MOV addr_reg , arg2_reg << width_shift
               //RSUB addr_reg , addr_reg , arg1_info->mem_addr
               //MEM dest_reg , [fp-] , addr_reg
               gen_mov_lshft_code(addr_reg , arg2_reg , width_shift);
               gen_dp_rri_code(RSUB , addr_reg , addr_reg , arg1_info->mem_addr);
               gen_mem_rrr_code(type , dest_reg , REG_FP , -1 , addr_reg , 1 << width_shift);
          }
     }
     else
     {
          /*arg1是寄存器或者内存，比如
            int a[10] , *b;
            b = a;
            b[1] = b[0];
            获得arg1寄存器，直接寄存器变址寻址。
           */
          if(arg1_flag == Arg_Reg)
               arg1_reg = arg1_info->reg_addr;
          else if(arg1_flag == Arg_Mem)
          {
               arg1_reg = gen_tempreg(except , 4);
               temp_reg[++ temp_reg_size] = arg1_reg;
               int arg1_width = get_width_from_index(arg1_index);
               gen_mem_rri_code(load , arg1_reg , REG_FP , -1 , arg1_info->mem_addr , arg1_width);
          }
          except[1] = arg1_reg;
          if(arg2_flag == Arg_Imm)//MEM dest_reg , [arg1_reg+] , (expr->arg2.imme)<<width_shift
               gen_mem_rri_code(type , dest_reg , arg1_reg , 1 , (expr->arg2.imme)<<width_shift, 1 << width_shift);
          else//MEM dest_reg , [arg1_reg+] , arg2_reg << #width_shift
               gen_mem_lshft_code(type , dest_reg , arg1_reg , 1 , arg2_reg , width_shift , 1 << width_shift);
     }
     if(dest_flag == Arg_Mem)
          store_var(dest_info , dest_reg);

     for(i = temp_reg_size ; i >= 0 ; i--)//恢复所有临时寄存器，注意顺序。
          restore_tempreg(temp_reg[i]);

     /*如果没有产生临时寄存器，可能可以优化*/
     if(is_combine == 1)
     {
          if(temp_reg_size == -1)
          {
               int index = get_index_of_temp(expr->index);
               struct var_info *expr_info = get_info_from_index(index);
               if(expr_info != NULL && expr_info->ref_mark == 0)
                    set_optmz(cur_code_index - 1 , optmz);
          }
     }
     
     return dest_reg;
}

/*
  解除引用。步骤如下：
  1、如果该三元式是引用，需要在load之前将所有该指针可能指向的变量存入内存；如果该三元式是定值，在store之前要将所有该指针可能指向的变量存入内存，之后将所有该指针可能指向的变量存入寄存器；
  2、计算寻址步长；
  3、针对arg1的下面几种情况，不同操作：
      a)局部数组首地址。寄存器相对寻址；
      b)其他情况都可以将地址统一装到目的寄存器中，寄存器间接寻址。
          装入地址方式：
              a)全局变量。如果在寄存器内则不操作；没有的话，调用函数获得地址；
              b)其他，包括指针型ident、三元式编号临时变量。如果在寄存器内则不操作；没有的话，直接从内存装入。
  4、恢复临时寄存器。
 */
static int gen_deref_code(enum mem type, struct triargexpr *expr, struct var_info *dest_info, enum Arg_Flag dest_flag, int imme)
{
     struct triargexpr_list *expr_node = cur_table->index_to_list[expr->index];
//     printf("**********");var_list_print(expr_node->pointer_entity);
     flush_pointer_entity(store , expr_node->pointer_entity);
     int width_shift = 2;
     int arg1_index = get_index_of_arg(&(expr->arg1));
     struct var_info *arg1_info;
     enum Arg_Flag arg1_flag = mach_prepare_index(arg1_index , &arg1_info , 1);
     int arg1_reg = get_reg_addr(arg1_info , arg1_flag);
     int dest_reg = get_reg_addr(dest_info , dest_flag);
     
     /*计算指针寻址的步长*/
     if(get_stride_from_index(arg1_index) == 1)
          width_shift = 0;

     int temp_reg[2] , temp_reg_size = -1 , i;
     for(i = 0 ; i < 2 ; i++)
          temp_reg[i] = -1;
     int except[2];
     except[0] = dest_reg;
     except[1] = arg1_reg;
     if(dest_flag != Arg_Reg)
     {
          dest_reg = gen_tempreg(except , 2);
          if(type == store)
          {
               if(dest_flag == Arg_Imm)
                    gen_mov_rsim_code(dest_reg , imme);
               else
                    load_var(dest_info , dest_reg);
          }
     }
     except[0] = dest_reg;
     
     int arg1_is_g = is_global(arg1_index);
     int arg1_is_array = is_array(arg1_index);
     if(arg1_is_array == 1 && arg1_is_g != 1)//是局部数组，LDB/LDW dest_reg , [fp-] , arg1_info->mem_addr
          gen_mem_rri_code(load , dest_reg , REG_FP , -1 , arg1_info->mem_addr , 1 << width_shift);
     else
     {
          if(arg1_flag != Arg_Reg)//不在寄存器的一定要分配寄存器
          {
               temp_reg[++temp_reg_size] = arg1_reg = gen_tempreg(&dest_reg , 1);
               if(arg1_is_g == 1)//全局数组
                    load_pointer(arg1_index , arg1_reg , 0 , -1);//将全局数组首地址载入临时寄存器
               else//LDW arg1_reg , [fp-] , arg1_info->mem_addr
                    gen_mem_rri_code(load , arg1_reg , REG_FP , -1 , arg1_info->mem_addr , 4);
          }
          
          //MEM dest_reg , [arg1_reg]
          gen_mem_rr_code(type , dest_reg , arg1_reg , 1 << width_shift);
     }
     if(dest_flag == Arg_Mem)
          store_var(dest_info , dest_reg);
     
     for(i = temp_reg_size ; i >= 0 ; i--)//恢复所有临时寄存器，注意顺序。
          restore_tempreg(temp_reg[i]);
     if(type == store)
          flush_pointer_entity(load , expr_node->pointer_entity);

     /*如果没有产生临时寄存器，而且是load类型，可能可以优化*/
     if(is_combine == 1)
     {
          if(temp_reg_size == -1 && type == load)
          {
               int index = get_index_of_temp(expr->index);
               struct var_info *expr_info = get_info_from_index(index);
               if(expr_info != NULL && expr_info->ref_mark == 0)
                    set_optmz(cur_code_index - 1 , optmz);
          }
     }
     
     return dest_reg;
}

/*将一个寄存器的值赋给一个三元式编号*/
static void gen_assign_expr_code(int expr_num , int arg2_reg)
{
     struct var_info *arg1_info;
     int dest_index = get_index_of_temp(expr_num);
     if(dest_index < 0)
          return;
     enum Arg_Flag arg1_flag = mach_prepare_index(dest_index , &arg1_info , 0);
     
     /*
       <1>arg1是寄存器，MOV；
       <2>arg1是内存，store；
      */
     if(arg1_flag == Arg_Reg)//arg1是寄存器
          gen_mov_rsrd_code(arg1_info->reg_addr , arg2_reg);
     else
          store_var(arg1_info , arg2_reg);
}

/*生成代码过程中，要多次在变量间赋值，expr的作用是根据运算类型判断是否对临时寄存器做其他操作*/
static void gen_assign_arg_code(struct triarg *arg1 , struct triarg *arg2 , struct triargexpr *expr)
{
     /*
       1、赋值操作
           <1>arg1和arg2是寄存器，或者arg1是寄存器而arg2是立即数，MOV；
           <2>arg1是寄存器，arg2是内存，load；
           <3>arg1是内存，arg2是寄存器，store；
           <4>arg1是内存，arg2是立即数，申请临时寄存器，存入arg2，store；
           <5>arg1和arg2都是内存，先将arg2导入临时寄存器，然后将临时寄存器的值存入内存。
       2、如果arg1是三元式编号，还要看一下它对应的三元式是否是*p或者a[i]，如果是的话，还要对内存操作。
      */
     const int nothing = 0 , assign = 1 , uminus = 2;
     int expr_flag;
     if(expr == NULL)
          expr_flag = nothing;
     else
     {
          if(expr->op == Uminus)
               expr_flag = uminus;
          else if(expr->op == Assign)
               expr_flag = assign;
     }
     
     int arg1_index = get_index_of_arg(arg1);//
     int arg2_index = get_index_of_arg(arg2);//立即数、
     //int dest_index = get_index_of_temp(expr->index);
     
     if(is_assign(expr , arg1_index) == 0)//没必要赋值
     {
          if(expr_flag == nothing)
               return;
          else if(expr_flag == assign)
          {//如果是赋值语句而arg1又是三元式，并且它不活跃，说明他是个没有被引用的三元式编号变量，要考虑数组或指针实体赋值
               if(arg1->type == ExprArg)
               {
                    struct triargexpr_list *last_expr_node = cur_table->index_to_list[arg1->expr];
                    struct triargexpr *last_expr = get_tae_entity(last_expr_node, cur_func_index);
                    
                    if(last_expr->op == Subscript)
                    {
                         struct var_info *arg2_info = NULL;
                         enum Arg_Flag arg2_flag = mach_prepare_index(arg2_index, &arg2_info, 1); 
						 int dest_reg = gen_array_code(store , last_expr , arg2_info , arg2_flag , arg2->imme);/* MARK TAOTAOTHERIPPER */
                         gen_assign_expr_code(expr->index , dest_reg);
                         if(arg2_flag != Arg_Reg)
                              restore_tempreg(dest_reg);
                         return;
                    }
                    else if(last_expr->op == Deref)
                    {
                         struct var_info *arg2_info = NULL;
                         enum Arg_Flag arg2_flag = mach_prepare_index(arg2_index, &arg2_info, 1);
                         int dest_reg = gen_deref_code(store , last_expr , arg2_info , arg2_flag , arg2->imme);
                         gen_assign_expr_code(expr->index , dest_reg);
                         if(arg2_flag != Arg_Reg)
                              restore_tempreg(dest_reg);
                         return;
                    }
               }
               struct triarg arg;
               arg.type = ExprArg;
               arg.expr = expr->index;
               gen_assign_arg_code(&arg , arg2 , NULL);
               return;
          }
          return;
     }
     
     struct var_info *arg1_info, *arg2_info;
     enum Arg_Flag arg1_flag = mach_prepare_index(arg1_index, &arg1_info, 0);
     enum Arg_Flag arg2_flag = mach_prepare_index(arg2_index, &arg2_info, 1);

     int dest_reg = arg1_info->reg_addr;
     int temp_reg = -1;
     
     if(arg2_flag == Arg_Imm)//arg2是立即数
     {
          int imme = arg2->imme;
          if(expr_flag == uminus)//一元减
               imme = 0 - imme;
          if(arg1_flag == Arg_Reg)//arg1在寄存器
               gen_mov_rsim_code(dest_reg , arg2->imme);
          else
          {
               dest_reg = gen_tempreg(NULL , 0);
               gen_mov_rsim_code(dest_reg , arg2->imme);
               temp_reg = dest_reg;
               store_var(arg1_info , dest_reg);
               //restore_reg(imme_reg);
          }
     }
     else if(arg1_flag == Arg_Reg)//arg1是寄存器
     {
          int is_arg2_array = is_array(arg2_index);
          int is_arg2_g = is_global(arg2_index);
          int is_arg2_cstr = is_conststr_byno(cur_func_info->func_symt , arg2_index);
          if(arg2_flag == Arg_Reg)//arg2是寄存器
          {
               if(expr_flag == uminus)
                    gen_rsub_rri_code(arg1_info->reg_addr , arg2_info->reg_addr , 0);
               else
                    gen_mov_rsrd_code(arg1_info->reg_addr , arg2_info->reg_addr);
          }
          else if(is_arg2_array == 1 && is_arg2_g == 0)//局部数组
               gen_dp_rri_code(SUB , arg1_info->reg_addr , REG_FP , arg2_info->mem_addr);
          else if(is_arg2_cstr == 1 || (is_arg2_array == 1 && is_arg2_g == 1))//常量字符串或者全局数组，把首地址给它
               load_pointer(arg2_index , arg1_info->reg_addr , 0 , -1);
          else
          {
               store_var(arg2_info , arg1_info->reg_addr);
               if(expr_flag == uminus)
                    gen_rsub_rri_code(arg1_info->reg_addr , arg1_info->reg_addr , 0);
          }
     }
     else
     {
          if(arg2_flag == Arg_Reg)
          {
               int rd = arg2_info->reg_addr;
               dest_reg = rd;
               if(expr_flag == uminus)
               {
                    int except[1];
                    except[0] = rd;
                    rd = gen_tempreg(except , 1);
                    temp_reg = rd;
                    load_var(arg2_info , rd);
                    gen_rsub_rri_code(rd , rd , 0);
               }
               store_var(arg1_info , rd);
               //if(expr != NULL && expr->op == Uminus)
               //restore_reg(rd);
          }
          else
          {
               int rd = gen_tempreg(NULL , 0);
               int is_arg2_array = is_array(arg2_index);
               int is_arg2_g = is_global(arg2_index);
               int is_arg2_cstr = is_conststr_byno(cur_func_info->func_symt , arg2_index);
               if(is_arg2_array == 1 && is_arg2_g == 0)//局部数组
                    gen_dp_rri_code(SUB , rd , REG_FP , arg2_info->mem_addr);
               if(is_arg2_cstr == 1 || (is_arg2_array == 1 && is_arg2_g == 1))//常量字符串或者全局数组，把首地址给它
                    load_pointer(arg2_index , rd , 0 , -1);
               else
                    load_var(arg2_info , rd);
               dest_reg = rd;
               if(expr_flag == uminus)
                    gen_rsub_rri_code(rd , rd , 0);
               store_var(arg1_info , rd);
          }
     }

     if(expr_flag == assign)
     {
          gen_assign_expr_code(expr->index , dest_reg);
     }
     
     restore_tempreg(temp_reg);
}

void gen_ref_code(struct triargexpr * expr, int dest_index, struct var_info * dest_info, enum Arg_Flag dest_flag)
{
    int dest_reg, tmp_mark = 0, tmp_inner_mark = 0 , arg1_index;
	if(dest_flag == Arg_Reg)
		dest_reg = dest_info -> reg_addr;
	else
	{
		dest_reg = gen_tempreg(NULL, 0);
		tmp_mark = 1;
	}

	if(expr -> arg1.type == IdArg)
	{
		arg1_index = get_index_of_id(expr -> arg1.idname);
		/* the global var may hasn't been loaded yet */
		load_pointer(arg1_index, dest_reg, 0, -1);
	}
	else if(expr -> arg1.type == ExprArg)
	{
		struct triargexpr refed_expr = cur_table -> table[(expr->arg1).expr];
		switch(refed_expr.op)
		{
			case Subscript:
				{
					/* 
					   if id_head in Reg just ADD
					   if id_head in Mem use load_pointer
					*/
					int id_index = get_index_of_id(refed_expr.arg1.idname);//must be id
					struct var_info * id_info = get_info_from_index(id_index);
					int id_flag = mach_prepare_arg(id_index, id_info, 1);
					
					int tmp_arg_index, tmp_arg_flag;
					struct var_info * tmp_arg_info;
					if(refed_expr.arg2.type == ImmArg)
						tmp_arg_flag = Arg_Imm;
					else
					{
						tmp_arg_index = get_index_of_arg(&refed_expr.arg2);
						tmp_arg_info = get_info_from_index(tmp_arg_index);
						tmp_arg_flag = mach_prepare_arg(tmp_arg_index, tmp_arg_info, 1);
					}

					if(id_flag == Arg_Reg)
					{
						struct mach_arg array_head, array_offset;
						array_head.type = Mach_Reg;
						array_head.reg = alloc_reg.result[id_index];
						if(tmp_arg_flag == Arg_Imm)
						{
							array_offset.type = Mach_Imm;
							array_offset.imme = refed_expr.arg2.imme;
						}
						else
						{
							array_offset.type = Mach_Reg;
							if(tmp_arg_flag == Arg_Reg)
								array_offset.reg = alloc_reg.result[tmp_arg_index];
							else
							{
								tmp_inner_mark = 1;
								int except[2];
								except[0] = dest_reg;
								except[1] = alloc_reg.result[id_index];
								array_offset.reg = gen_tempreg(except, 2);
								load_var(tmp_arg_info, array_offset.reg);
							}
						}
						if(get_stride_from_index(id_index) == WORD)
						{
							if(array_offset.type == Mach_Reg)
								insert_dp_code(ADD, dest_reg, array_head, array_offset, 2, LL);
							else
							{
								array_offset.imme *= WORD;
								insert_dp_code(ADD, dest_reg, array_head, array_offset, 0, NO);
							}
						}
						else insert_dp_code(ADD, dest_reg, array_head, array_offset, 0, NO);
						if(tmp_inner_mark == 1)
							restore_tempreg(array_offset.reg);
					}
					else
					{
						if(refed_expr.arg2.type == ImmArg)
							load_pointer(id_index, dest_reg, refed_expr.arg2.imme, 0);
						else
						{
							if(refed_expr.arg2.type == IdArg)
							{
								int offset_reg;
								if(tmp_arg_flag == Arg_Reg)
									offset_reg = alloc_reg.result[tmp_arg_index];
								else
								{
									tmp_inner_mark = 1;
									offset_reg = gen_tempreg(&dest_reg, 1);
									load_var(tmp_arg_info, offset_reg);
								}
								load_pointer(id_index, dest_reg, 0, offset_reg);
								if(tmp_inner_mark == 1)
									restore_tempreg(offset_reg);
							}
						}
					}
					break;
				}
			case Deref:
				{
					/*
						if in reg just mov
						if in mem 
							for array load_pointer
							for other load
					 */
					int p_index = get_index_of_arg(&refed_expr.arg1);
					struct var_info * p_info = get_info_from_index(p_index);
					enum Arg_Flag p_flag = mach_prepare_arg(p_index, p_info, 1);
					if(p_flag == Arg_Reg)//the temp array head can't be in reg
						gen_mov_rsrd_code(dest_reg, alloc_reg.result[p_index]);
					else
					{
						if(is_array(p_index))
							load_pointer(p_index, dest_reg, 0, -1);
						else
							load_var(p_info, dest_reg);
					}
					break;
				}
			default:
				fprintf(stderr, "error in gen code for ref\n");
				break;
		}
	}
	if(tmp_mark == 1)
	{
		store_var(dest_info, dest_reg);
		restore_tempreg(dest_reg);
	}
    /*如果没有产生临时寄存器，可能可以优化*/
     if(is_combine == 1)
     {
          if(tmp_mark == 0)
          {
               int index = get_index_of_temp(expr->index);
               struct var_info *expr_info = get_info_from_index(index);
               if(expr_info != NULL && expr_info->ref_mark == 0)
                    set_optmz(cur_code_index - 1 , optmz);
          }
     }
}

static int is_2_power(int num)
{
     int i;
     for(i = 0 ; i < 32 ; i++)
          if(num == (1 << i))
               return i;
     return -1;
}

static int get_2_power(int num , int *remain)
{
     int res;
     if((res = is_2_power(num)) != -1 )
     {
          (*remain) = 0;
          return res;
     }
     if((res = is_2_power(num + 1)) != -1 )
     {
          (*remain) = -1;
          return res;
     }
     if((res = is_2_power(num - 1)) != -1 )
     {
          (*remain) = 1;
          return res;
     }
     return -1;
}

static void gen_per_code(struct triargexpr * expr)
{
	check_is_jump_dest(expr -> index);

	update_reg_content(expr);

	int dest_index, arg1_index, arg2_index;
	struct var_info * dest_info, * arg1_info, * arg2_info; 
	enum Arg_Flag dest_flag, arg1_flag, arg2_flag;

#ifdef MACH_DEBUG
    print_triargexpr(*expr);
#endif
	switch(expr -> op)
	{
		case Ref:						/* &  */
		case Plus:                       /* +  */
		case Minus:                      /* -  */	
		case Mul:                        /* *  */
		case Plusplus:                   /* ++ */
		case Minusminus:                 /* -- */
		case Uplus:                      /* +  */
		case Uminus:                     /* -  */
		case BigImm:
			{
				dest_index = get_index_of_temp(expr -> index);
				if(dest_index == -1)
				{
					if(expr -> op != Plusplus && expr -> op != Minusminus)
						return;
				}
				else
				{
					dest_info = get_info_from_index(dest_index);
                    if(dest_index > 0)
                         dest_flag = mach_prepare_arg(dest_index, dest_info, 0);
				}

				if(expr -> op == Ref)
				{
					gen_ref_code(expr, dest_index, dest_info, dest_flag);	
					break;
				}

				//ImmArg op ImmArg should be optimized before
				if(expr -> arg1.type == ExprArg && expr -> arg1.expr == -1)
					;
				else
				{
					if(expr -> arg1.type != ImmArg)
					{
						arg1_index = get_index_of_arg(&(expr->arg1));
						arg1_info = get_info_from_index(arg1_index);
						if(expr -> op == Plusplus || expr -> op == Minusminus)//these two unary ops will write arg1
							arg1_flag = mach_prepare_arg(arg1_index, arg1_info, 0);
						else
							arg1_flag = mach_prepare_arg(arg1_index, arg1_info, 1);
					}
					else
						arg1_flag = Arg_Imm;
				}

				if(expr -> op == Plus || expr -> op == Minus || expr -> op == Mul)
				{
					if(expr -> arg2.type != ImmArg)
					{
						arg2_index = get_index_of_arg(&(expr -> arg2));
						arg2_info = get_info_from_index(arg2_index);
						arg2_flag = mach_prepare_arg(arg2_index, arg2_info, 1);
					}
					else
						arg2_flag = Arg_Imm;
				}
				break;
			}	
		case Funcall:
			{
				dest_index = get_index_of_temp(expr -> index);
				if(dest_index != -1)
				{
					dest_info = get_info_from_index(dest_index);
					dest_flag = mach_prepare_arg(dest_index, dest_info, 0);
				}
				break;
			}
		case Arglist:
		case Return:
			{
				if(expr -> arg1.type != ImmArg)
				{
                     arg1_index = get_index_of_arg(&(expr -> arg1));
                     if(arg1_index < 0)
                          break;
					 arg1_info = get_info_from_index(arg1_index);
					 arg1_flag = mach_prepare_arg(arg1_index, arg1_info, 1);
				}
				else
					arg1_flag = Arg_Imm;
				break;
			}
		default:
			break;
	}

	switch(expr -> op)
	{
		case Assign:                     /* =  */
			{
				/*
				   arg1 = arg2的翻译方式。
				   1、赋值。
				       a)赋值操作
                           <1>arg1和arg2是寄存器，或者arg1是寄存器而arg2是立即数，MOV；
                           <2>arg1是寄存器，arg2是内存，load；
                           <3>arg1是内存，arg2是寄存器，store；
                           <4>arg1是内存，arg2是立即数，申请临时寄存器，存入arg2，store；
                           <5>arg1和arg2都是内存，先将arg2导入临时寄存器，然后将临时寄存器的值存入内存。
                       b)如果arg1是三元式编号，还要看一下它对应的三元式是否是*p或者a[i]，如果是的话，还要对内存操作。
				   2、指针相关操作。
				       <1>如果arg1是（*arg）的形式，需要把（*arg）对应的三元式的pointer_entity中所有map_id对应的寄存器都写入内存。
				 */
				gen_assign_arg_code(&(expr->arg1) , &(expr->arg2) , expr);
				break;				
			}
			/* cond op translated when translating condjump */

		case Plus:                       /* +  */
		case Minus:                      /* -  */	
		case Mul:                        /* *  */
			{
				int tempreg1, tempreg2;
				int mark1 = 0, mark2 = 0;
				int except[3];
				int ex_size = 0;
				if(dest_flag == Arg_Reg)
					except[ex_size++] = dest_info -> reg_addr;
				if(arg1_flag == Arg_Reg)
				{
					tempreg1 = arg1_info -> reg_addr;
					except[ex_size++] = tempreg1;
				}

				if(arg2_flag == Arg_Reg)
				{
					tempreg2 = arg2_info -> reg_addr;
					except[ex_size++] = tempreg2;
				}

				if(arg1_flag == Arg_Mem)//There can only be one immediate number
				{
					tempreg1 = gen_tempreg(except, ex_size);
					mark1 = 1;
					except[ex_size++] = tempreg1;
					if(is_array(arg1_index))
						load_pointer(arg1_index, tempreg1, 0, -1);
					else
						load_var(arg1_info, tempreg1);
				}

				if(arg2_flag == Arg_Mem)
				{
					tempreg2 = gen_tempreg(except, ex_size);
					mark2 = 1;
					except[ex_size++] = tempreg2;
					if(is_array(arg1_index))
						load_pointer(arg2_index, tempreg2, 0, -1);
					else
						load_var(arg2_info, tempreg2);
				}

				/*
					Plus => 
						reg + reg	ADD
						Imme + reg	ADD
						reg + Imme	ADD
						preg + reg	ADD reg<<2
						preg + Imme ADD Imme<<2
						reg + preg	ADD reg<<2
						Imme + preg ADD imme<<2
					Minus =>
						reg - reg	SUB
						imme - reg	RSUB
						reg - imme	SUB
						preg - reg	SUB reg<<2
						preg - imme SUB imme<<2
					Mul =>
						reg * reg	MUL
						imme * reg	MUL
						reg * imme	MUL
				*/
				enum dp_op_type op_type;
				switch(expr -> op)
				{
					case Plus:
						op_type = ADD;			
						break;
					case Minus:
						if(arg1_flag == Arg_Imm)
							op_type = RSUB;/* The arg can't be a pointer for RSUB */
						else
							op_type = SUB;
						break;
					case Mul:
						op_type = MUL;
						break;
					default:
						fprintf(stderr, "error when gen plus sub and mul\n");
						break;
				}

				struct mach_arg binary_arg1, binary_arg2;
				if(arg1_flag == Arg_Imm)//only one Imme one time
				{
					binary_arg1.type = Mach_Imm;
					binary_arg1.imme = expr -> arg1.imme;
				}
				else
				{
					binary_arg1.type = Mach_Reg;
					binary_arg1.reg = tempreg1;
				}

				if(arg2_flag == Arg_Imm)
				{
					binary_arg2.type = Mach_Imm;
					binary_arg2.imme = expr -> arg2.imme;
				}
				else
				{
					binary_arg2.type = Mach_Reg;
					binary_arg2.reg = tempreg2;
				}

				int tempdest;
				if(dest_flag == Arg_Reg)
					tempdest = dest_info -> reg_addr;
				else
				{
					if(mark1)
						tempdest = tempreg1;
					else if(mark2)
						tempdest = tempreg2;
					else tempdest = gen_tempreg(except, ex_size);
				}

                /*error!! 立即数不能逻辑移位*/
				if(arg1_flag != Arg_Imm && get_stride_from_index(arg1_index) == WORD)//only add and sub
				{
					if(binary_arg2.type == Mach_Imm)
					{
						binary_arg2.imme *= WORD;
						insert_dp_code(op_type, tempdest, binary_arg1, binary_arg2, 0, NO);
					}
					else
						insert_dp_code(op_type, tempdest, binary_arg1, binary_arg2, 2, LL);
				}
				else if(arg2_flag != Arg_Imm && get_stride_from_index(arg2_index) == WORD)//only add
				{
					if(binary_arg1.type == Mach_Imm)
					{
						binary_arg1.imme *= WORD;
						insert_dp_code(op_type, tempdest, binary_arg2, binary_arg1, 0, NO);
					}
					else
						insert_dp_code(op_type, tempdest, binary_arg2, binary_arg1, 2, LL);
				}
				else
				{
					if(op_type == RSUB)
						insert_dp_code(op_type, tempdest, binary_arg2, binary_arg1, 0, NO);
					else
					{
						if(arg1_flag == Arg_Imm)
                        {
                             int power , remain;
                             power = get_2_power(binary_arg1.imme , &remain);
                             if(op_type == MUL && power != -1)
                             {
                                  if(remain == 0)//MOV tempdest , reg2 << power
                                       insert_dp_code(MOV , tempdest , null , binary_arg2 , power , LL);
                                  else if(remain == 1)//ADD tempdest , reg2 , reg2 << power
                                       insert_dp_code(ADD , tempdest , binary_arg2 , binary_arg2 , power , LL);
                                  else if(remain == -1)//RSUB tempdest , reg2 , reg2 << power
                                       insert_dp_code(RSUB , tempdest , binary_arg2 , binary_arg2 , power , LL);
                             }
                             else
							 {
								 if(op_type == MUL)
								 {
									 tempreg1 = gen_tempreg(except, ex_size);
									 mark1 = 1;
									 except[ex_size++] = tempreg1;
									 gen_mov_rsim_code(tempreg1, binary_arg1.imme);
									 binary_arg1.type = Arg_Reg;
									 binary_arg1.reg = tempreg1;
								 }
								 insert_dp_code(op_type, tempdest, binary_arg2, binary_arg1, 0, NO);
							 }
                        }
						else
                        {
                             if(arg2_flag == Arg_Imm)
                             {
                                  int power , remain;
                                  power = get_2_power(binary_arg2.imme , &remain);
                                  if(op_type == MUL && power != -1)
                                  {
                                       if(remain == 0)//MOV tempdest , reg1 << power
                                            insert_dp_code(MOV , tempdest , null , binary_arg1 , power , LL);
                                       else if(remain == 1)//ADD tempdest , reg1 , reg1 << power
                                            insert_dp_code(ADD , tempdest , binary_arg1 , binary_arg1 , power , LL);
                                       else if(remain == -1)//RSUB tempdest , reg1 , reg1 << power
                                            insert_dp_code(RSUB , tempdest , binary_arg1 , binary_arg1 , power , LL);
                                  }
                                  else
								  {
									  if(op_type == MUL)
									  {
										  tempreg2 = gen_tempreg(except, ex_size);
										  mark2 = 1;
										  except[ex_size++] = tempreg2;
										  gen_mov_rsim_code(tempreg2, binary_arg2.imme);
										  binary_arg2.type = Arg_Reg;
										  binary_arg2.reg = tempreg2;
									  }
									  insert_dp_code(op_type, tempdest, binary_arg1, binary_arg2, 0, NO);
								  }
                             }
                             else
                                  insert_dp_code(op_type, tempdest, binary_arg1, binary_arg2, 0, NO);
                        }
					}
				}

				if(dest_flag != Arg_Reg)
					store_var(dest_info, tempdest);
				
				if(dest_flag == Arg_Mem && !mark1 && !mark2)
					restore_tempreg(tempdest);
				if(mark2)
					restore_tempreg(tempreg2);
				if(mark1)
					restore_tempreg(tempreg1);
                
                /*三个操作数都是寄存器或立即数，可能可以优化*/
                if(is_combine == 1)
                {
                     if(dest_flag == Arg_Reg && mark1 == 0 && mark2 == 0)
                     {
                          int index = get_index_of_temp(expr->index);
                          struct var_info *expr_info = get_info_from_index(index);
                          if(expr_info != NULL && expr_info->ref_mark == 0)
                               set_optmz(cur_code_index - 1 , optmz);
                     }
                }
				break;
			}
            
        case Plusplus:                   /* ++ */
        case Minusminus:                 /* -- *//*优先级高于二元或赋值操作*/
			{
                 /*
                   自加自减。步骤：
                   1、获得arg1的寄存器，在内存的话要申请临时寄存器装入；
                   2、如果dest在寄存器，使用MOV指令；否则store；
                   3、将arg1自加或自减；
                   4、如果arg1在内存，写回内存，恢复临时寄存器。
                  */
				int arg1_reg , dest_reg;
				int stride = get_stride_from_index(arg1_index);
                int width = get_width_from_index(arg1_index);

                /*获得arg1的寄存器，在内存的话要申请临时寄存器装入*/
                if(arg1_flag == Arg_Reg)
                     arg1_reg = arg1_info->reg_addr;
                else if(arg1_flag == Arg_Mem)//can't be immd
                {
                     arg1_reg = gen_tempreg(&dest_reg , 1);
                     load_var(arg1_info , arg1_reg);
                }

                /*如果dest在寄存器，使用MOV指令；否则store；*/
				if(dest_index != -1)
				{
					dest_reg = dest_info->reg_addr;//MARK TAOTAOTHERIPPER
					if(dest_flag == Arg_Reg)//MOV dest_reg , arg1_reg
						gen_mov_rsrd_code(dest_reg , arg1_reg);
					else//STW/STB arg1_reg , [fp-] , dest_info->mem_addr
						gen_mem_rri_code(store , arg1_reg , REG_FP , -1 , dest_info->mem_addr , width);
				}
                
				/*操作数据*/
				/*ADD/SUB arg1_reg , arg1_reg , #stride*/
				enum dp_op_type dp_op;
				if(expr->op == Plusplus)
					dp_op = ADD;
				else
					dp_op = SUB;
				gen_dp_rri_code(dp_op , arg1_reg , arg1_reg , stride);
                
				/*如果arg1在内存，写回内存，恢复临时寄存器*/;
				if(arg1_flag == Arg_Mem)
                {
                     store_var(arg1_info , arg1_reg);
                     restore_tempreg(arg1_reg);
                }
				break;
			}
        case TrueJump:
        case FalseJump://immed cond has been optimized before, so the cond can only be expr or id
                gen_cj_expr(expr);
                break;
            
		case UncondJump:
				{
					int label_num = ref_jump_dest(expr -> arg1.expr);
					char * dest_label_name = gen_new_label(label_num);
					insert_buncond_code(dest_label_name, 0);
					break;
				}

        case Uplus:                      /* +  */
            break;
        case Uminus:                     /* -  */
			{
				int except[3];
				int ex_size = 0;
				int temp_reg, dest_reg;
				struct mach_arg arg1, arg2;
				if(dest_flag == Arg_Mem)
				{
					if(arg1_flag == Arg_Reg)
						except[ex_size++] = arg1_info->reg_addr;
					dest_reg = gen_tempreg(except, ex_size);
					ex_size = 0;
				}
				else
					dest_reg = dest_info->reg_addr;

				if(arg1_flag == Arg_Mem)
				{
					/* lod tempreg */;
					except[ex_size++] = dest_reg;
					temp_reg = gen_tempreg(except, ex_size);
					load_var(arg1_info, temp_reg);
					/* -x = ~x + 1 */
					arg1.type = Mach_Reg;
					arg1.reg = temp_reg;
					arg2.type = Mach_Imm;
					arg2.imme = 0;
					insert_dp_code(RSUB, dest_reg, arg1, arg2, 0, NO);
					/* restore for arg1 */;
					restore_tempreg(temp_reg);
				}
				else
				{
					if(arg1_flag == Arg_Reg)
					{
						arg1.type = Mach_Reg;
						arg1.reg = arg1_info->reg_addr;
						arg2.type = Mach_Imm;
						arg2.imme = 0;
						insert_dp_code(RSUB, dest_reg, arg1, arg2, 0, NO);
					}
					else    /* arg1_flag == Arg_Imm */ //MARK TAOTAOTHERIPPER need some optimizing here
					{
						arg2.type = Mach_Imm;
						arg2.imme = expr->arg1.imme;
						insert_dp_code(MVN, dest_reg, null, arg2, 0, NO);
						arg2.imme = 1;
						arg1.type = Mach_Reg;
						arg1.reg = dest_reg;
						insert_dp_code(ADD, dest_reg, arg1, arg2, 0, NO);
					}
				}
				if(dest_flag == Arg_Mem)
				{
					store_var(dest_info, dest_reg);
					restore_tempreg(dest_reg);
				}
                /*两个操作数都是寄存器或立即数，可能可以优化*/
                if(is_combine == 1)
                {
                     if(dest_flag == Arg_Reg && arg1_flag != Arg_Mem)
                     {
                          int index = get_index_of_temp(expr->index);
                          struct var_info *expr_info = get_info_from_index(index);
                          if(expr_info != NULL && expr_info->ref_mark == 0)
                               set_optmz(cur_code_index - 1 , optmz);
                     }
                }
				break;
			}

        case Subscript:                  /* [] */
			{
                 /*
                   数组下标引用代码：c = a[i]
                   1、三元式编号的准备工作；
                   2、编号变量在内存中的话要申请临时寄存器；
                   3、调用函数生成代码；
                   4、如果编号变量在内存中，要写回内存并且恢复临时寄存器。
                  */
                 
                 /*三元式编号的准备工作*/
                 dest_index = get_index_of_temp(expr -> index);
                 if(dest_index == -1)
                      break;
                 dest_info = get_info_from_index(dest_index);
                 dest_flag = mach_prepare_arg(dest_index, dest_info, 0);

                 /*编号变量在内存中的话要申请临时寄存器*/
                 /*int dest_reg;
                 int except[2];
                 except[0] = arg1_info->reg_addr;
                 except[1] = arg2_info->reg_addr;
                 if(dest_flag == Arg_Reg)
                      dest_reg = dest_info->reg_addr;
                 else if(dest_flag == Arg_Mem)
                 dest_reg = gen_tempreg(except , 2);*/

                 /*调用函数生成代码*/
                 gen_array_code(load , expr , dest_info , dest_flag , 0);

                 /*如果编号变量在内存中，要写回内存并且恢复临时寄存器*/
                 /*if(dest_flag == Arg_Mem)
                 {
                      (1)回写内存；(2)恢复临时寄存器
                      store_var(dest_info , dest_reg);
                      restore_tempreg(dest_reg);
                      }*/
				break;	
			}

		case Ref:                        /* &  */
			break;//should have gen code before

		case Deref:                      /* '*' */
			{
                 /*
                   解除引用的三元式引用代码：a = (*p)
                   1、三元式编号的准备工作；
                   2、编号变量在内存中的话要申请临时寄存器；
                   3、调用函数生成代码；
                   4、如果编号变量在内存中，要写回内存并且恢复临时寄存器。
                  */

                 /*三元式编号的准备工作*/
                 dest_index = get_index_of_temp(expr -> index);
                 if(dest_index == -1)
                      break;
                 dest_info = get_info_from_index(dest_index);
                 dest_flag = mach_prepare_arg(dest_index, dest_info, 0);

                 /*编号变量在内存中的话要申请临时寄存器*/
                 /*int rd;
                 rd = dest_info->reg_addr;
                 int except[2];
                 except[0] = rd;
                 except[1] = arg1_info->reg_addr;
                 if(dest_flag != Arg_Reg)
                      rd = gen_tempreg(except , 2);
                      except[0] = rd;*/

                 /*生成代码*/
                 gen_deref_code(load , expr , dest_info , dest_flag , 0);
                 
                 /*目的数在内存的话要先写回内存；恢复临时寄存器。*/
                 /*if(dest_flag == Arg_Mem)
                 {
                      store_var(dest_info , rd);
                      restore_tempreg(rd);
                      }*/
				break;
			}
		
		case Funcall:                    /* () */
			{
                flush_pointer_entity(store, expr -> arg2.func_flush_list);//MARK TAOTAOTHERIPPER
				/* caller save */
                struct var_list_node * focus = expr -> actvar_list -> head;
                struct var_info * vinfo = NULL;
                char saved_reg[32];
                int saved_reg_count = 0;
                while(focus != NULL && focus != expr -> actvar_list -> tail -> next)
                {
                    vinfo = get_info_from_index(focus->var_map_index);
                    if((vinfo->reg_addr >= 4 && vinfo->reg_addr <= 15) || vinfo->reg_addr == 28)
                    {
						/* the global var has been stored in flush global var, so here I
						   don't need to store the clear reg, just reload it later is OK. */
						if(is_array(focus->var_map_index) || is_conststr_byno(cur_func_info -> func_symt, focus->var_map_index))
							saved_reg[saved_reg_count++] = vinfo->reg_addr;
						else if(!is_global(focus->var_map_index))/* the array and string is clear global var, just store var */
						{
							if(is_id_var(focus->var_map_index))
								store_var(vinfo, vinfo -> reg_addr);
							else
								gen_mem_rri_code(store, vinfo -> reg_addr, REG_FP, -1, caller_save_index - saved_reg_count * WORD, WORD);
							saved_reg[saved_reg_count++] = vinfo->reg_addr;	
						}
                    }
					focus = focus -> next;
                }

                insert_buncond_code(expr->arg1.idname, 1);
                /* restore caller save */

				struct mach_arg caller_arg1, caller_arg2;
				for(saved_reg_count = saved_reg_count - 1; saved_reg_count >= 0; saved_reg_count --)/* can be optimized some */
				{
					int reg_num = saved_reg[saved_reg_count];
					int var_index = reg_dpt[reg_num].content;/* the reg_dpt won't change during funcall */
					vinfo = get_info_from_index(var_index);

					if(is_array(var_index) || is_conststr_byno(cur_func_info -> func_symt, var_index))/* load clear global var, may be array and const string */
					{
						load_pointer(var_index, reg_num, 0, -1);
						continue;
					}
					if(is_id_var(var_index))/* there won't be any array or const string in local reg */
					{
						load_var(vinfo, reg_num);
						continue;
					}/* or else the temp vars are all in caller save zoon, just load them */
					caller_arg1.type = Mach_Reg;
					caller_arg2.type = Mach_Imm;
					caller_arg1.reg = REG_FP;
					caller_arg2.imme = caller_save_index - saved_reg_count * WORD;
					insert_mem_code(LDW , saved_reg[saved_reg_count], caller_arg1 , caller_arg2 , 0, -1, NO, 0);
				}
				
                flush_pointer_entity(store, expr -> arg2.func_flush_list);//MARK TAOTAOTHERIPPERif(dest_index != -1)//The r0 won't be changed during the restore above	
				if(dest_index != -1)
				{
					if(dest_flag == Arg_Reg)
						gen_mov_rsrd_code(dest_info -> reg_addr, 0);
					else
						store_var(dest_info, 0);
				}
				if(arglist_num_mark > 4)
					pop_param(arglist_num_mark - 4);
                arglist_num_mark = 0;
				break;
			}

		case Arglist:
			{
				if(arg1_flag == Arg_Reg)
                {
                    if(arglist_num_mark < 4) /* r0-r3 available */
                        gen_mov_rsrd_code(arglist_num_mark, arg1_info->reg_addr);
                    else
                        push_param(arg1_info->reg_addr); /* push into stack */
                }
				else if(arg1_flag == Arg_Mem)
				{
                    /* lod tempreg, arg1 */;
					/* push tempreg */;
					/* restore tempreg */;

                    if(arglist_num_mark < 4)
					{
						if(is_array(arg1_index))
							load_pointer(arg1_index, arglist_num_mark, 0, -1);
						else
							load_var(arg1_info, arglist_num_mark);
					}
                    else
                    {
                        int tempreg = gen_tempreg(NULL, 0);
						if(is_array(arg1_index))
							load_pointer(arg1_index, tempreg, 0, -1);
						else
							load_var(arg1_info, tempreg);
                        push_param(tempreg); /* push into stack */
                        restore_tempreg(tempreg);
                    }
				}
                else            /* arg1_flag == Arg_Imm */
                {
                    if(arglist_num_mark < 4)
                        gen_mov_rsim_code(arglist_num_mark, expr->arg1.imme);
                    else
                    {
                        int tempreg = gen_tempreg(NULL, 0);
                        gen_mov_rsim_code(tempreg, expr->arg1.imme);
                        push_param(tempreg); /* push into stack */
                        restore_tempreg(tempreg);
                    }
                }
                ++arglist_num_mark;
				break;
			}

        case BigImm:
			{
				int reg;
				uint32_t src = expr->arg1.imme;
				struct mach_arg mach_src, mach_base;
				mach_src.type = Mach_Imm;
				mach_base.type = Mach_Reg;
				if(dest_flag == Arg_Reg)
					reg = dest_info->reg_addr;
				else                /* dest_flag == Arg_Mem */
					reg = gen_tempreg(NULL, 0);
				if((~src) <= 0x1ff)
                {
                    mach_src.imme = ~src;
					insert_dp_code(MVN, reg, mach_src, null, 0, NO);
                }
				else
				{
					int lsb = 0;
					while(((src >> lsb) & 1) == 0)
						++lsb;
					mach_src.imme = (src >> lsb) & 0x1ff;
                    mach_src.imme = ((uint64_t)(mach_src.imme) << 32 | mach_src.imme) >> (32 - lsb);
					insert_dp_code(MOV, reg, mach_src, null, 0, NO);
					lsb += 9;

					mach_base.reg = reg;
					while(lsb < 32)
					{
						while(lsb < 32 && ((src >> lsb) & 1) == 0)
							++lsb;
						if(lsb >= 32)
							break;
						mach_src.imme = (src >> lsb) & 0x1ff;
                        mach_src.imme = ((uint64_t)(mach_src.imme) << 32 | mach_src.imme) >> (32 - lsb);
						insert_dp_code(OR, reg, mach_base, mach_src, 0, NO);
						lsb += 9;
					}
				}
				if(dest_flag == Arg_Mem)
				{
					store_var(dest_info, reg);
					restore_tempreg(reg);
				}
                /*目的操作数是寄存器，可能可以优化*/
                if(is_combine == 1)
                {
                     if(dest_flag == Arg_Reg)
                     {
                          int index = get_index_of_temp(expr->index);
                          struct var_info *expr_info = get_info_from_index(index);
                          if(expr_info != NULL && expr_info->ref_mark == 0)
                               set_optmz(cur_code_index - 1 , optmz);
                     }
                }
				break;
			}
        
		case Return:
			{
                 if(arg1_index != -3)
                 {
                      if(arg1_flag == Arg_Reg)
                           gen_mov_rsrd_code(0, arg1_info -> reg_addr);
                      else if(arg1_flag == Arg_Mem)
                           load_var(arg1_info, 0);
                      else            /* arg1_flag == Arg_Imm */
                           gen_mov_rsim_code(0, expr->arg1.imme);
                 }
				flush_global_var();
				free_temp_var_inmem();	
				if(strcmp(cur_func_info -> name, "main") != 0)/* TAOTAOTHERIPPER MARK */
					callee_save_pop();
				leave_func_pop();
                insert_jump_code(REG_LR);
				break;
			}
		default:
			break;
	}

}

static void reset_reg_number()//
{//可用的寄存器：R4－R15，R17－R26，R28
     int i;
	 for(i = 0; i < cur_ref_var_num; i++)
	 {
		 if(alloc_reg.result[i] == -1)
			 ;
		 else if(alloc_reg.result[i] <= 12)
			 alloc_reg.result[i] += 3;
		 else if(alloc_reg.result[i] <= 22)
			 alloc_reg.result[i] += 4;
		 else if(alloc_reg.result[i] == 23)
			 alloc_reg.result[i] = 28;
     }
}

static void print_mach_header(FILE * out_buf)
{
	int str_align = 3;
	int nor_align = 2;
	if(cur_func_info -> func_symt -> str_num != 0)
	{
		fprintf(out_buf, "\t.section\t.rodata\n");
		int str_index;
		for(str_index = 0; str_index < cur_func_info -> func_symt -> str_num; str_index++)
		{
			fprintf(out_buf, "\t.align\t%d\n", str_align);
			fprintf(out_buf, "%s:\n", cur_func_info -> func_symt -> const_str[str_index].vl_info -> name);
			char * tmp_string = cur_func_info -> func_symt -> const_str[str_index].string;
			tmp_string[strlen(tmp_string) - 1] = 0;
			fprintf(out_buf, "\t.ascii\t%s\\000\"\n", cur_func_info -> func_symt -> const_str[str_index].string);
		}
	}
	fprintf(out_buf, "\t.text\n");
	fprintf(out_buf, "\t.align\t%d\n", nor_align);
	fprintf(out_buf, "\t.global\t%s\n", cur_func_info -> name);
	fprintf(out_buf, "\t.type\t%s,function\n", cur_func_info -> name);
	fprintf(out_buf, "%s:\n", cur_func_info -> name);
}

static void print_mach_tail(FILE * out_buf)
{
	int nor_align = 2;
	if(ref_g_var_num > 0)
	{
		fprintf(out_buf, "\t.align\t%d\n", nor_align);
		fprintf(out_buf, "%s:\n", global_var_label);
		int g_var_index;
		for(g_var_index = 0; g_var_index < ref_g_var_num; g_var_index ++)
		{
			struct value_info * var_value_info = get_valueinfo_byno(cur_func_info -> func_symt, global_var_label_offset[g_var_index]);
			char * var_name = var_value_info -> name;
			fprintf(out_buf, "\t.word\t%s\n", var_name);
		}
	}
	fprintf(out_buf, "\t.size\t%s, .-%s\n", cur_func_info -> name, cur_func_info -> name);
}

#ifdef MACH_DEBUG
static void print_mach_code(FILE * out_buf)
{
	int index;
	for(index = 0; index < code_table_list[cur_func_index].code_num; index++)
		mcode_out(&code_table_list[cur_func_index].table[index], out_buf);
}
#endif

void print_file_header(FILE * out_buf, char * filename)
{
	fprintf(out_buf, "\t.file\t\"%s\"\n", filename);
	g_global_id_num = get_globalvar_num();
	if(g_global_id_num > 0)
	{
		int global_var_index;
		for(global_var_index = 0; global_var_index < g_global_id_num; global_var_index ++)//global var's index is 0 ~ g_global_id_num
		{
			struct value_info * g_value_info = get_valueinfo_byno(simb_table, global_var_index);
			if(g_value_info -> modi != Extern)
			{
				fprintf(out_buf, "\t.comm\t%s, ", g_value_info -> name);
				switch(g_value_info -> type -> type)
				{
					case Array:
						if(g_value_info -> type -> base_type -> type == Char)
							fprintf(out_buf, "%d, %d\n", g_value_info -> type -> size, BYTE);
						else
							fprintf(out_buf, "%d, %d\n", g_value_info -> type -> size * WORD, WORD);
						break;
					case Char:
						fprintf(out_buf, "%d, %d\n", BYTE, BYTE);
						break;
					default:
						fprintf(out_buf, "%d, %d\n", WORD, WORD);
						break;
				}
			}
		}
	}
}

void print_file_tail(FILE * out_buf)
{
	char * edition = "MINICC: (UC_1.0_20101227) 0.0.0";
	fprintf(out_buf, "\t.ident\t\"%s\"\n", edition);
}

void new_code_table_list()
{
	code_table_list = calloc(g_table_list_size, sizeof(struct mach_code_table)); 
	total_label_num = 0;
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


void gen_machine_code(int func_index, FILE * out_buf)//Don't forget NULL at last
{
	int var_list_size;
	/********************************* data flow analyse ***************************************/
	struct basic_block * block_head = make_fd(func_index);
	pointer_analyse(func_index);	
	struct var_list * active_var_array = analyse_actvar(&var_list_size, func_index);//活跃变量分析

	/********************************* gen mach code beg ***************************************/
	set_cur_function(func_index);
	alloc_reg = reg_alloc(active_var_array, var_list_size, cur_ref_var_num, max_reg_num);//current now
	recover_triargexpr(block_head);		
	reset_reg_number();//reset the reg number
#ifdef MACH_DEBUG
    int i;
    for(i = 0 ; i < cur_ref_var_num ; i++)
         printf("%d->r%d\t" , i , alloc_reg.result[i]);
    printf("\n");
#endif
	enter_func_push();
	if(strcmp(cur_func_info -> name, "main") != 0)/* TAOTAOTHERIPPER MARK */
		callee_save_push();
	prepare_temp_var_inmem();//alloc mem for temp var in stack
	struct triargexpr_list * tmp_node = cur_table -> head;
	while(tmp_node != NULL)//make tail's next to be NULL
	{
		struct triargexpr * expr = get_tae_entity(tmp_node, cur_func_index);
		gen_per_code(expr);
		tmp_node = tmp_node -> next;
	}
	print_mach_header(out_buf);
#ifdef PEEPHOLE
	peephole(func_index, out_buf);
#else
	print_mach_code(out_buf);//MARK TAOTAOTHERIPPER
#endif
	print_mach_tail(out_buf);

	free_all(active_var_array);
	leave_cur_function();
}

