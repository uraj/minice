#ifndef __MINIC_MACHINECODE_H__
#define __MINIC_MACHINECODE_H__
#include <stdint.h>

/*I'm not sure whether it is good to divide the op into three types*/

enum mach_arg_type { Unused = 0, Mach_Reg, Mach_Imm, Mach_ID};

struct mach_arg
{
    enum mach_arg_type type;//when not used, use -1
    union
    {
        uint8_t reg;
		uint32_t imme;
		char * idname;
    };
};

enum mach_op_type
{
	DP = 0,		/* data processing */
	CMP,
	MEM,		/* memory access */
	BRANCH,		/* branch */
	TAG			/* tag */
};

enum dp_op_type
{
	AND = 0,
	SUB,
	ADD,
	OR,

	MVN,//may be useful
	MOV,/* no arg1 */
	MOVCOND//may be useful

	MUL,
	MLA,//may be useful
	MULSL
};

enum cmp_op_type
{
	CMPSUB.A
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
	BRX = 0,/* no cond */
	BCOND
};

enum condition_type
{
	NO = 0, /* no cond */
	EQ,	/*equal*/
	NE, /*not equal*/
	N,  /*negtive*/
	NN, /*not negtive*/
	OV, /*overflow*/
	NV, /*not overflow*/
	ULT,/*unsigned less than*/
	UGT,/*unsigned greater than*/
	ULE,/*unsigned less equal*/
	UGE,/*unsigned greater equal*/
	SLT,/*signed less than*/
	SGT,/*signed greater than*/
	SLE,/*signed less equal*/
	SGE,/*signed greater equal*/
}

enum shift_type
{
	NO = 0,	/* no shift */
	LL,		/* logical left */
	LR,		/* logical right */
	AR,		/* arith right */
	RR		/* roll right */
};

enum indexed_type
{
	PRENW = 0,/* pre indexed and not write base register */
	PREW, /* pre indexed and write base register */
	POST /* post indexed and write base register */
}

struct mach_code//mach means machine
{
	enum mach_op_type op_type;
	
	union
	{
		char * tag_name;
		enum dp_op_type dp_op;
		enum mem_op_type mem_op;
		enum branch_op_type branch_op;
	};

	union
	{
		int dest;//only reg
		char * dest_tag_name;//for cond jump
	};

	union
	{
		struct mach_arg arg1;
		enum condition_type cond;					/* used in condition-jump */
	};
		
	struct mach_arg arg2, arg3;
	
	union
	{
		char sign;								/* 1 change sign, 0 not *//* used in dp */
		char link;								/* 1 jump and link, 0 not *//* used in branch */
		char offet;								/* 1 is +, and 0 is no, -1 is - *//* used in mem */
	};	

	union
	{
		enum shift_type shift;					/* used in data-processing and memory-access*/
		enum special_width width;				/* used in l/d hw and l/d signed */
	};

	enum indexed_type indexed;					/* used in mem */	
};

struct mach_code_table
{
	struct mach_code * table;
	int code_num;
};

extern int g_table_list_size; 
extern struct mach_code_table * code_table_list;
extern struct triargtable ** table_list;
extern struct symbol_table ** simb_table;  

extern void gen_machine_code(int func_index);
extern void new_code_table_list();
extern void free_code_table_list();
#endif

