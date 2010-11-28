#ifndef __MINIC_ASTCHECH_H__
#define __MINIC_ASTCHECH_H__

#include "minic_typedef.h"
#include "minic_ast.h"
#include "minic_typetree.h"
#include "minic_symtable.h"
#include "minic_symfunc.h"

extern int ast_type_check(struct ast* root, struct symbol_table* glob_table, struct symbol_table* loca_table);
extern struct typetree* compat_typetree(enum operator op, struct ast* tree1, struct ast* tree2);
#endif
