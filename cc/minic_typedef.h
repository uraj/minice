#ifndef __MINIC_TYPEDEF_H__
#define __MINIC_TYPEDEF_H__

enum operator /* do NOT change the order of the list */
{
    /* Binary op */
    Assign,                     /* =  */
    Land,                       /* && */
    Lor,                        /* || */
    Lnot,                       /* !  */
    Eq,                         /* == */
    Neq,                        /* != */
    Ge,                         /* >= */
    Le,                         /* <= */
    Nge,                        /* <  */
    Nle,                        /* >  */
    Plus,                       /* +  */
    Minus,                      /* -  */
    Mul,                        /* *  */
    Subscript,                  /* [] */
    
    /* For triargexpr only*/
    TrueJump,
    FalseJump,
    /* For triargexpr only*/
    UncondJump,
    
    /* Unary op */
    Funcall,                    /* () */
    Uplus,                      /* +  */
    Uminus,                     /* -  */
    Plusplus,                   /* ++ */
    Minusminus,                 /* -- */
    Ref,                        /* &  */
    Deref,                      /* *  */
    BigImm,

    /* Special */
    Arglist,
    Return,
    
    /* Empty */
    Temp,
    Nullop,
};

enum data_type					/* enum for different data types */
{
	Void = 1,
    Char,
	Int,
	Pointer,
	Array,
	Function,
	Typeerror
};

enum modifier
{
	Epsilon,
	Extern,
	Register
};

struct token_info
{
	union
	{
		int   ival;
		char  cval;
		char* sval;
	};
};

#endif
