#include "minic_astcheck.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int yylineno;

int ast_type_check(struct ast* root, struct symbol_table* glob_table, struct symbol_table* loca_table)
{
    if(root == NULL)
        return 0;
    else if(root -> isleaf)
    {
        switch(root -> val -> ast_leaf_type)
        {
            case Iconstleaf:
                root -> ast_typetree = typet_new_type(Int);
                break;
            case Sconstleaf:
                root -> ast_typetree = typet_new_type(Pointer);
                root -> ast_typetree -> base_type = typet_new_type(Char);
                break;
            case Idleaf:
                root -> ast_typetree = typet_typetree_dup(cal_lookup_var(root -> val -> sval, glob_table, loca_table));
        }
        return 0;
    }
    else /* isleaf = 0 */
    {
        struct ast* left_child = root -> left;
        struct ast* right_child = root -> right;
        if(ast_type_check(left_child, glob_table, loca_table)||
           ast_type_check(right_child, glob_table, loca_table)
           )
            root -> ast_typetree = typet_new_type(Typeerror);
        else
            root -> ast_typetree = compat_typetree(root -> op, left_child, right_child);
        if(root -> ast_typetree -> type == Typeerror)
            return 1;
        else
            return 0;
    }
    return 1; /* never reached */
}

static int ast_lval_check(struct ast* root)
{
    if((root -> isleaf) && (root -> val -> ast_leaf_type == Idleaf))
        return 1;
    else if(root -> op == Deref) /* *pointer has lval */
        return 1;
    else if(root -> op == Subscript) /* array[i] has lval */
        return 1;
    else
        return 0;
}

static void type_err_print(enum operator op)
{
    fprintf(stderr, "line %d: incompatible operand(s) for operator ", yylineno);
    fprintf(stderr, ".\n");
    return;
}

static inline int arg_parm_compat(struct typetree* arg, struct typetree* parm)
{
    if(arg == NULL || parm == NULL)
        return 1;
    else if(parm -> type == Typeerror || arg -> type == Typeerror)
        return 1;
    else if((parm -> type >= Char && parm -> type <= Pointer) &&
            arg -> type >= Char && arg -> type <= Array)
    {
        if((parm -> type == Pointer) &&
           !(arg -> type == Pointer || arg -> type == Array))
            fprintf(stderr, "Line %d: (warning) parameter is a pointer while argument is a integer(char).\n", yylineno);
        else if((parm -> type != Pointer) &&
                (arg -> type == Pointer || arg -> type == Array))
            fprintf(stderr, "Line %d: (warning) parameter is a integer(char) while argument is a pointer.\n", yylineno);
        return 0;        
    }

    else
        return 1;
}

struct typetree* compat_typetree(enum operator op, struct ast* tree1, struct ast* tree2)
{
    struct typetree* ret;
    if(tree1 == NULL)
        return typet_new_type(Typeerror);
    enum data_type tree1_type = tree1 -> ast_typetree -> type;
    enum data_type tree2_type;
    struct ast* arg = tree2;
    struct typetree* parm = tree1 -> ast_typetree -> parm_list;
    int funcheck = 0;
    if(tree1_type == Typeerror)
        return typet_new_type(Typeerror);
    else if(tree2 == NULL)
        tree2_type = Typeerror;
    else
    {
        tree2_type = tree2 -> ast_typetree -> type;
        if(tree2_type == Typeerror)
            return typet_new_type(Typeerror);
    }
    switch(op)
    {
        case Uminus:            /* fall through */
        case Uplus:
            if(tree1_type == Int ||
               tree1_type == Char)
                ret = typet_new_type(tree1_type);
            else
                ret = typet_new_type(Typeerror);
            break;

        case Plusplus:          /* fall through */
        case Minusminus:
            if(tree1_type == Int ||
               tree1_type == Char ||
               tree1_type == Pointer)
                ret = typet_typetree_dup(tree1 -> ast_typetree);
            else
                ret = typet_new_type(Typeerror);
            break;
            
        case Lnot:
            if(tree1_type == Int ||
               tree1_type == Char ||
               tree1_type == Pointer)
                ret = typet_new_type(Int);
            else
                ret = typet_new_type(Typeerror);
            break;
            
        case Ref:
            if(!ast_lval_check(tree1))
            {
                ret = typet_new_type(Typeerror);
                fprintf(stderr, "Line %d: lvalue required as unary '&' operand.\n", yylineno);
                break;
            }
            ret = typet_new_type(Pointer);
            ret -> base_type = typet_typetree_dup(tree1 -> ast_typetree);
            break;

        case Deref:
            if(tree1_type == Pointer ||
               tree1_type == Array
               )
                ret = typet_typetree_dup(tree1 -> ast_typetree -> base_type);
            else
                ret = typet_new_type(Typeerror);
            break;

        case Plus:
            if((tree1_type == Int || tree1_type == Char) &&
               (tree2_type >= Char && tree2_type <= Array))
            {
                /* cast to higher type */
                ret = typet_typetree_dup(tree1_type > tree2_type ?
                                   tree1 -> ast_typetree : tree2 -> ast_typetree);
                if(ret -> type == Array)
                    ret -> type = Pointer;
            }
            else if((tree1_type == Pointer || tree1_type == Array) &&
                    (tree2_type == Char || tree2_type == Int))
            {
                ret = typet_typetree_dup(tree1 -> ast_typetree);
                ret -> type = Pointer;
            }
            else
                ret = typet_new_type(Typeerror);
            break;

        case Minus:
            if((tree2_type == Int || tree2_type == Char) &&
               (tree1_type >= Char && tree1_type <= Array))
            {
                /* cast to higher type */
                ret = typet_typetree_dup(tree1_type > tree2_type ?
                                   tree1 -> ast_typetree : tree2 -> ast_typetree);
                if(ret -> type == Array)
                    ret -> type = Pointer;
            }
            else
                ret = typet_new_type(Typeerror);
            break;

        case Mul:
            if((tree2_type == Int || tree2_type == Char) &&
               (tree1_type == Int || tree1_type == Char))
            {
                /* cast to higher type */
                ret = typet_typetree_dup(tree1_type > tree2_type ?
                                   tree1 -> ast_typetree : tree2 -> ast_typetree);
            }
            else
                ret = typet_new_type(Typeerror);
            break;

        case Subscript:
            if((tree1_type == Pointer || tree1_type == Array) &&
               (tree2_type == Int || tree2_type == Char))
                ret = typet_typetree_dup(tree1 -> ast_typetree -> base_type);
            else
                ret = typet_new_type(Typeerror);
            break;

        case Land:              /* fall through: Logical operators */
        case Lor:
        case Eq:
        case Neq:
        case Ge:
        case Le:
        case Nge:
        case Nle:
            if((tree1_type >= Char && tree1_type <= Array) &&
               (tree2_type >= Char && tree2_type <= Array))
            {
                if(tree1_type >= Pointer && tree2_type < Pointer)
                    fprintf(stderr, "Line %d: (warning) comparison between pointer and integer(char).\n", yylineno);
                else if(tree2_type >= Pointer && tree1_type < Pointer)
                    fprintf(stderr, "Line %d: (warning) comparison between integer(char) and pointer.\n", yylineno);
                ret = typet_new_type(Int);
            }
            else
                ret = typet_new_type(Typeerror);
            break;

        case Assign:
            if(!ast_lval_check(tree1))
            {
                ret = typet_new_type(Typeerror);
                fprintf(stderr, "Line %d: lvalue required as '=' operand.\n", yylineno);
                break;
            }
            if(tree2_type == Function)
                tree2_type = tree2 -> ast_typetree -> return_type -> type;
            if((tree1_type >= Char && tree1_type <= Pointer) &&
               (tree2_type >= Char && tree2_type <= Pointer))
            {
                if(tree1_type >= Pointer && tree2_type < Pointer)
                    fprintf(stderr, "Line %d: (warning) assignment makes pointer from integer(char).\n", yylineno);
                else if(tree2_type >= Pointer && tree1_type < Pointer)
                    fprintf(stderr, "Line %d: (warning) assignment makes integer(char) from pointer.\n", yylineno);
                ret = typet_new_type(tree1_type);
            }
            else
                ret = typet_new_type(Typeerror);
            break;

        case Arglist:
            ret = typet_typetree_dup(tree1 -> ast_typetree);
            break;

        case Funcall:
            while(arg && parm)
            {
                funcheck = arg_parm_compat(arg -> ast_typetree, parm);
                arg = arg -> right;
                parm = parm -> next_parm;
                if(funcheck)
                    break;
            }
            if(arg || parm || funcheck)
                ret = typet_new_type(Typeerror);
            else
                ret = typet_typetree_dup(tree1 -> ast_typetree -> return_type);
            break;
            
        default:
            fprintf(stderr, "Line %d: unexpected operator in ast.\n", yylineno);
            
            exit(1);
    }
    if(ret -> type == Typeerror)
        type_err_print(op);
    return ret;
}
