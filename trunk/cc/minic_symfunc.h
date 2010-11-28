#ifndef __MINIC_SYMFUNC_H__
#define __MINIC_SYMFUNC_H__

#include "minic_symtable.h"
#include "minic_typedef.h"
#define Simpletype 0

extern void decl_retype_insert(struct symbol_stack * type_stack, struct symbol_table * curr_table,
                               enum data_type curr_type, enum data_type curr_modi);
extern void decl_add_pointer(struct value_type * newtype);
extern void decl_push_array(char * name, int size, struct symbol_stack * type_stack);
extern struct value_type * decl_push_simple(char * name, struct symbol_stack * type_stack);
extern struct value_type * decl_push_func(char * name, struct symbol_stack * symbol_stack, struct symbol_stack * parm_stack);
extern void def_insert_func(char * name, struct symbol_stack * parm_stack, struct symbol_table * simb_table,
							struct symbol_table * curr_table, enum data_type type_name, enum data_type pointer_type);
extern void dd_push_parm(char * name, struct symbol_stack * parm_stack, enum data_type new_parm_type, enum data_type pointer_type);
extern struct typetree * cal_lookup_var(char * name, struct symbol_table * simb_table, struct symbol_table * curr_table);

#endif
