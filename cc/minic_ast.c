#include "minic_typedef.h"
#include "minic_ast.h"
#include "minic_typetree.h"
#include <memory.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct ast* new_ast(enum operator p_op, int p_isleaf, void* p_left, void* p_rightorval)
{
    struct ast* a = malloc(sizeof(struct ast));
    if((p_isleaf == 0) && (p_rightorval != NULL) &&
       (((struct ast *)p_left)->isleaf &&
        ((struct ast *)p_left)->val->ast_leaf_type == Iconstleaf &&
        ((struct ast *)p_rightorval)->isleaf &&
        ((struct ast *)p_rightorval)->val->ast_leaf_type == Iconstleaf)) /* comupte constant expressions during compilation */
    {        
        switch(p_op)
        {
            case Land:
                a->val = malloc(sizeof(struct leafval));
                a->val->ival =
                    ((struct ast *)p_left)->val->ival &
                    ((struct ast *)p_rightorval)->val->ival;
                break;
                
            case Lor:
                a->val = malloc(sizeof(struct leafval));
                a->val->ival =
                    ((struct ast *)p_left)->val->ival |
                    ((struct ast *)p_rightorval)->val->ival;
                break;
                
            case Eq:
                a->val = malloc(sizeof(struct leafval));
                a->val->ival =
                    ((struct ast *)p_left)->val->ival ==
                    ((struct ast *)p_rightorval)->val->ival;
                break;
                
            case Neq:
                a->val = malloc(sizeof(struct leafval));
                a->val->ival =
                    ((struct ast *)p_left)->val->ival !=
                    ((struct ast *)p_rightorval)->val->ival;
                break;
                
            case Ge:
                a->val = malloc(sizeof(struct leafval));
                a->val->ival =
                    ((struct ast *)p_left)->val->ival >=
                    ((struct ast *)p_rightorval)->val->ival;
                break;
                
            case Le:
                a->val = malloc(sizeof(struct leafval));
                a->val->ival =
                    ((struct ast *)p_left)->val->ival <=
                    ((struct ast *)p_rightorval)->val->ival;
                break;
                
            case Nge:
                a->val = malloc(sizeof(struct leafval));
                a->val->ival =
                    ((struct ast *)p_left)->val->ival <
                    ((struct ast *)p_rightorval)->val->ival;
                break;
                
            case Nle:
                a->val = malloc(sizeof(struct leafval));
                a->val->ival =
                    ((struct ast *)p_left)->val->ival >
                    ((struct ast *)p_rightorval)->val->ival;
                break;
                
            case Plus:
                a->val = malloc(sizeof(struct leafval));
                a->val->ival =
                    ((struct ast *)p_left)->val->ival +
                    ((struct ast *)p_rightorval)->val->ival;
                break;
                
            case Minus:
                a->val = malloc(sizeof(struct leafval));
                a->val->ival =
                    ((struct ast *)p_left)->val->ival -
                    ((struct ast *)p_rightorval)->val->ival;
                break;
                
            case Mul:
                a->val = malloc(sizeof(struct leafval));
                a->val->ival =
                    ((struct ast *)p_left)->val->ival *
                    ((struct ast *)p_rightorval)->val->ival;
                break;
                
            default:
                fprintf(stderr, "Invalid operand for opertor ");
                op_printer(p_op, stderr);
                exit(1);
        };
        a->isleaf = 1;
        a->op = Nullop;
        a->left = NULL;
        a->val->ast_leaf_type = Iconstleaf;
    }
    else
    {
        a -> op = p_op;
        a -> isleaf = p_isleaf;
        if(p_isleaf)
            a -> val = p_rightorval;
        else
            a -> right = p_rightorval;
        a -> left = p_left;
    }
    a -> ast_typetree = NULL;
    return a;
}

void free_ast(struct ast* p)
{
    if(p == NULL)
        return;
    else if(p -> isleaf)
    {
        if(p -> val -> ast_leaf_type == Sconstleaf || p -> val -> ast_leaf_type == Idleaf)
            free(p -> val -> sval);
        free(p -> val);
    }
    else
    {
        typet_free_typetree(p -> ast_typetree);
        if(p -> left)
            free_ast(p -> left);
        if(p -> right)
            free_ast(p -> right);
    }
    free(p);
    return;
}

struct leafval* new_iconst(struct token_info ti)
{
    struct leafval* l = malloc(sizeof(struct leafval));
    l -> ast_leaf_type = Iconstleaf;
    l -> ival = ti.ival;
    return l;
}

struct leafval* new_cconst(struct token_info ti)
{
    struct leafval* l = malloc(sizeof(struct leafval));
    l -> ast_leaf_type = Iconstleaf;
    l -> ival = ti.cval;
    return l;
}

struct leafval* new_sconst(struct token_info ti)
{
    struct leafval* l = malloc(sizeof(struct leafval));
    l -> ast_leaf_type = Sconstleaf;
    l -> sval = strdup(ti.sval);
    return l;
}

struct leafval* new_var(const char* name)
{
    struct leafval* l = malloc(sizeof(struct leafval));
    l -> ast_leaf_type = Idleaf;
    l -> sval = strdup(name);
    return l;
}

static void leaftype_printer(enum leaftype type)
{
    switch(type)
    {
        case Iconstleaf:
            fprintf(stderr, "Integer");
            break;
        case Sconstleaf:
            fprintf(stderr, "String");
            break;
        case Idleaf:
            fprintf(stderr, "Id");
            break;
    }
    return;
}

void op_printer(enum operator op, FILE* output_buff)
{
    switch(op)
    {
        case Deref:
            fprintf(output_buff, "unary *");
            break;
        case Ref:
            fprintf(output_buff, "unary &");
            break;
        case Minusminus:
            fprintf(output_buff, "--");
            break;
        case Plusplus:
            fprintf(output_buff, "++");
            break;
        case Uminus:
            fprintf(output_buff, "unary -");
            break;
        case Uplus:
            fprintf(output_buff, "unary +");
            break;
        case Assign:
            fprintf(output_buff, "=");
            break;
        case Land:
            fprintf(output_buff, "&&");
            break;
        case Lor:
            fprintf(output_buff, "||");
            break;
        case Lnot:
            fprintf(output_buff, "!");
            break;
        case Eq:
            fprintf(output_buff, "==");
            break;
        case Neq:
            fprintf(output_buff, "!=");
            break;
        case Ge:
            fprintf(output_buff, ">=");
            break;
        case Le:
            fprintf(output_buff, "<=");
            break;
        case Nge:
            fprintf(output_buff, "<");
            break;
        case Nle:
            fprintf(output_buff, ">");
            break;
        case Plus:
            fprintf(output_buff, "binary +");
            break;
        case Minus:
            fprintf(output_buff, "binary -");
            break;
        case Mul:
            fprintf(output_buff, "binary *");
            break;
        case Subscript:
            fprintf(output_buff, "[]");
            break;
        case Funcall:
            fprintf(output_buff, "()");
            break;
        case Arglist:
            fprintf(output_buff, "Arglist");
            break;
        case UncondJump:
            fprintf(output_buff, "UncondJump");
            break;
        case TrueJump:
            fprintf(output_buff, "TrueJump");
            break;
        case FalseJump:
            fprintf(output_buff, "FalseJump");
            break;
        case Return:
            fprintf(output_buff, "Return");
            break;
        case Temp:
            fprintf(output_buff, "Temp");
            break;
        case Nullop:
            fprintf(output_buff, "Nullop");
            break;
        default:
            ;
    }    
    return;
}

void print_ast(struct ast* tree)
{
    static int level = 0;
    int i;
    if(tree == NULL)
        return;
    else if(tree -> isleaf)
    {
        for(i = 0; i < level; ++i)
            fprintf(stderr, " ");
        leaftype_printer(tree -> val -> ast_leaf_type);
        printf("\n");
    }
    else
    {
        for(i = 0; i < level; ++i)
            fprintf(stderr, " ");
        op_printer(tree -> op, stderr);
        fprintf(stderr, "\n");
        ++level;
        if(tree -> left != NULL)
            print_ast(tree -> left);
        if(tree -> right != NULL)
            print_ast(tree -> right);
        --level;
    }
        
    return;
}
