#ifndef __MINIC_AST_H__
#define __MINIC_AST_H__

#include "minic_typedef.h"
#include <stdio.h>

struct ast
{
    struct typetree * ast_typetree;
    enum operator op;
    int isleaf;
    struct ast* left;
    union
    {
        struct ast* right;
        struct leafval* val;
    };
};

enum leaftype { Iconstleaf, Sconstleaf, Idleaf };

struct leafval
{
    enum leaftype ast_leaf_type;
    union
    {
        char* sval;
        int ival;
    };
};

extern struct leafval* new_iconst(struct token_info ti);
extern struct leafval* new_cconst(struct token_info ti);
extern struct leafval* new_sconst(struct token_info ti);
extern struct leafval* new_var(const char* name);
extern struct ast* new_ast(enum operator op, int isleaf, void* left, void* right);
extern void free_ast(struct ast* p);
extern void print_ast(struct ast* tree);
extern void op_printer(enum operator op, FILE* output_buff);

#endif
