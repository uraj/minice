#ifndef __MINIC_MACHINECODE_H__
#define __MINIC_MACHINECODE_H__

enum mach_arg_type { Unused = 0, Mach_Reg, Mach_Imm};

struct mach_arg
{
    enum mach_arg_type type;//when not used, use -1
    union
    {
        int reg;
		int imme;
    };
};

enum mach_op_type
{
	DP,			/* data processing */
	MEM,		/* memory access */
	BRANCH		/* branch */
};

enum dp_op_type
{
	AND,
	XOR,
	SUB,
	RSB,
	ADD,
	ADC,
	ABC,
	RSC,
	ORR,
	CLB,
	
	MVN,
	MOV,/* no arg1 */
	
	MUL,
	MLA,

	CAND,
	CXOR,
	CSUB,
	CADD/* no dest, no sign */
};

enum mem_op_type//width?
{
	LOD,
	STR
};

enum branch_op_type
{
	BRX,/* no cond */
	BCOND
};

enum branch_cond
{
	EQ,	/*equal*/
	NE, /*not equal*/
	N,  /*negtive*/
	NN, /*not negtive*/
	OV, /*overflow*/
	NV, /*not overflow*/
	AL, /*all*/
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

struct mach_code//mach means machine
{
	enum mach_op_type op_type;
	union
	{
		enum dp_op_type dp_op;
		enum mem_op_type mem_op;
		enum brach_op_type branch_op;
	};

	struct mach_arg dest, arg1, arg2, arg3;

	union
	{
		char sign;								/* 1 change sign, 0 not */
		char link;								/* 1 jump and link, 0 not */
		char offet;								/* 1 is +, and 0 is no, -1 is - */
	};	

	union
	{
		enum shift_type shift;					/* LL LR AR RR */
		enum brach_cond cond;
	};

	char indexed;							/* 1 is pre_indexed
											   0 is post_indexed */
	char write;								/* 1 write base register
											   0 not */
};

extern struct triargtable ** table_list;
extern struct symbol_table ** simb_table;  
#endif

