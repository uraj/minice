#ifndef __MINIC_MACHINECODE_H__
#define __MINIC_MACHINECODE_H__
#include <stdint.h>
#include <stdio.h>
//#define MACH_DEBUG

#define REG_SP 29
#define REG_FP 27
#define REG_LR 30
#define REG_PC 31
enum mach_arg_type { Unused = 0, Mach_Reg, Mach_Imm, Mach_Label};

struct mach_arg
{
    enum mach_arg_type type;//when not used, use -1
    union
    {
        int reg;
		int imme;
		char * label;
    };
};

enum mach_op_type
{
	DP = 0,		/* data processing */
	CMP,
	MEM,		/* memory access */
	BRANCH,		/* branch */
	LABEL		/* label */
};

enum dp_op_type
{
	AND = 0,
	SUB,
    RSUB,
	ADD,
	OR,
    XOR,
    
	MVN,
	MOV,/* no arg1 */
	MOVCOND,//may be useful

	MUL
	//MLA,//may be useful
};

enum cmp_op_type
{
	CMPSUB_A
}; 

enum mem_op_type//width? the fields for memory are already to many..
{
	LDW = 0,
	STW,
	LDB,
	STB
};/* no arg3 for half word and signed data */

enum branch_op_type
{
	JUMP = 0,/* reg dest */
	B,		/* label dest */
	BCOND
};

enum condition_type
{
	Mach_EQ,	/*equal*/
	Mach_NE, /*not equal*/
	Mach_SL,/*signed less than*/
	Mach_SG,/*signed greater than*/
	Mach_EL,/*signed less equal*/
	Mach_EG/*signed greater equal*/
};

enum shift_type
{
	NO = 0,	/* no shift */
	LL,		/* logical left */
	LR,		/* logical right */
	AR,		/* arith right */
	RR		/* roll right */
};

enum index_type
{
	PRENW = 0,	/* pre indexed not write back register */
	PREW,	/* pre indexed write back register */
	POST	/* post indexed */
};

struct mach_code//mach means machine
{
	enum mach_op_type op_type;
	
	union
	{
		char * label;
		enum dp_op_type dp_op;
		enum cmp_op_type cmp_op;
		enum mem_op_type mem_op;
		enum branch_op_type branch_op;
	};

	union
	{
		int dest;//only reg
		char * dest_label;//for cond jump
	};

	union
	{
		struct mach_arg arg1;
		enum condition_type cond;					/* used in condition-jump */
	};
		
	struct mach_arg arg2;
	uint32_t arg3;
	
	union
	{
		char link;								/* 1 jump and link, 0 not *//* used in branch */
		char offset;								/* 1 is +, -1 is - and 0 is no*//* used in mem */
	};

	union
	{
		enum index_type indexed;							/* 1 => write_back, 2 => not write_back */
		char optimize;						/* only for dp op type, 1 => can be optimized 0 => can't*/
	};
	enum shift_type shift;					/* used in data-processing and memory-access*/
};

struct mach_code_table
{
	struct mach_code * table;
	int code_num;
};

extern int g_global_id_num;
extern int g_table_list_size; 
extern struct mach_code_table * code_table_list;
extern struct triargtable ** table_list;
extern struct symbol_table * simb_table;  

extern void print_file_header(FILE * out_buf, char * filename);
extern void print_file_tail(FILE * out_buf);
extern void gen_machine_code(int func_index, FILE * out_buf);
extern void new_code_table_list();
extern void free_code_table_list();
#endif

