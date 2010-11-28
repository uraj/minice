#include "minic_triargexpr.h"
#include "minic_triargtable.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* globals*/
extern struct triargexpr_list *ghead, *gtail;
extern struct triargexpr *gtriargexpr_table;
extern int gtriargexpr_table_index;
extern int gtriargexpr_table_bound;

static int get_opresult_width(const struct typetree* ttree)
{
    if(ttree == NULL)
    {
        fprintf(stderr, "Typetree damaged!\n");
        fflush(stderr);
        exit(1);
    }
    int width;
    switch(ttree -> type)
    {
        case Pointer:
        case Int:
            width = 4;
            break;        
        case Char:
            width = 1;
            break;
        case Array:
        case Void:
        case Function:
        case Typeerror:
            fprintf(stderr, "Wrong type when generating triargexpr.\n");
            fflush(stderr);
            width = -1;
            break;
    }
    return width;
}

static inline struct triargexpr unpatched_condjump_gen(enum operator op, struct triarg subexpr)
{
    struct triargexpr ret;
    
    ret.op = op;
    ret.width = -1;
    ret.arg1 = subexpr;
    ret.arg2.type = ExprArg;
    ret.arg2.expr = -1;
    return ret;
}

int subexpr_arithval_gen(const struct subexpr_info* subexpr)
{
    struct triargexpr expr;
    int ret = gtriargexpr_table_index;
    /* store logical expression's val in the temp var generated
     * (1) gentemp
     * (2) =          temp 1 (truelist redirected here)
     * (3) UncondJump (3+2)
     * (4) =          temp 0 (falselist redirected here)
     * the end, return (1)'s index
     */
    
    /* gen (1) */
    expr.op = Nullop;
    expr.width = 4;
    expr.index = insert_triargexpr(expr);
    
    /* gen (2) */
    expr.width = 4;
    expr.op = Assign;
    expr.arg1.type = ExprArg;
    expr.arg1.expr = ret;
    expr.arg2.type = ImmArg; /* assign it with 1*/
    expr.arg2.imme = 1;
    expr.index = insert_triargexpr(expr);
    patch_list_backpatch(subexpr -> truelist, expr.index); /* if true, temp = 1 */
    
    /* gen (3)*/
    expr.op = UncondJump;
    expr.arg1.type = ExprArg;
    expr.arg1.expr = expr.index + 3;/* should be + 3?*/
    expr.index = insert_triargexpr(expr);
    
    /* gen (4) */
    expr.op = Assign;
    expr.arg1.type = ExprArg;
    expr.arg1.expr = ret;
    expr.arg2.type = ImmArg;
    expr.arg2.imme = 0;
    expr.index = insert_triargexpr(expr); /* if false, temp = 0 */
    patch_list_backpatch(subexpr -> falselist, expr.index);
    
    return ret;
}


struct subexpr_info triargexpr_gen(struct ast* root)
{
    struct subexpr_info ret;
    ret.truelist = ret.falselist = NULL;
    ret.begin = gtriargexpr_table_index;
    
    if(root == NULL)
        exit(1);
    if(root -> isleaf)
    {
        switch(root -> val -> ast_leaf_type)
        {
            case Idleaf:
                ret.subexpr_arg.type = IdArg;
                ret.subexpr_arg.idname = strdup(root -> val -> sval);
                break;
            case Iconstleaf:
                ret.subexpr_arg.type = ImmArg;
                ret.subexpr_arg.imme = root -> val -> ival;
                break;
            case Sconstleaf:
                exit(1);
        }
        ret.arithtype = 1;
        ret.end = gtriargexpr_table_index;
        return ret;
    }
    
    struct triargexpr expr;
    struct subexpr_info lsub, rsub;
    int larith_index, rarith_index;
    
    switch(root -> op)
    {
        case Land:
            lsub = triargexpr_gen(root -> left);
            if(lsub.arithtype)
            {
                /* gen expr "FalseJump lsub.subexpr_arg ? " */
                expr = unpatched_condjump_gen(FalseJump, lsub.subexpr_arg);
                expr.index = insert_triargexpr(expr);
                /* adding it to lsub.falselist */
                lsub.falselist = patch_list_append(lsub.falselist, expr.index);
                lsub.truelist = NULL;
            }
            rsub = triargexpr_gen(root -> right);
            if(rsub.arithtype)
            {
                /* gen expr "TrueJump rsub.subexpr_arg ? "*/
                expr = unpatched_condjump_gen(TrueJump, rsub.subexpr_arg);
                expr.index = insert_triargexpr(expr);
                /* adding it to rsub.truelist */
                rsub.truelist = patch_list_append(rsub.truelist, expr.index);
                
                /* gen expr "UncondJump ? " */
                expr.op = UncondJump;
                expr.arg1.type = ExprArg;
                expr.arg1.expr = -1;
                expr.width = -1;
                expr.index = insert_triargexpr(expr);
                /* adding it to rsub.falselist */
                rsub.falselist = patch_list_append(rsub.falselist, expr.index);
            }
            patch_list_backpatch(lsub.truelist, rsub.begin);
            ret.truelist = rsub.truelist;
            ret.falselist = patch_list_merge(lsub.falselist, rsub.falselist);
            ret.arithtype = 0;
            break;
            
        case Lor:
            lsub = triargexpr_gen(root -> left);
            if(lsub.arithtype)
            {
                /* gen expr "TrueJump lsub.subexpr_arg ? " */
                expr = unpatched_condjump_gen(TrueJump, lsub.subexpr_arg);
                expr.index = insert_triargexpr(expr);
                /* adding it to lsub.falselist */
                lsub.truelist = patch_list_append(lsub.truelist, expr.index);
                lsub.falselist = NULL;
            }
            rsub = triargexpr_gen(root -> right);
            if(rsub.arithtype)
            {
                /* gen expr "TrueJump rsub.subexpr_arg ? " */
                expr = unpatched_condjump_gen(TrueJump, rsub.subexpr_arg);
                expr.index = insert_triargexpr(expr);
                /* adding it to rsub.truelist */
                rsub.truelist = patch_list_append(rsub.truelist, expr.index);
                
                /* gen expr "UncondJump ? ", adding it to rsub.falselist */
                expr.op = UncondJump;
                expr.arg1.type = ExprArg;
                expr.arg1.expr = -1;
                expr.width = -1;
                expr.index = insert_triargexpr(expr);
                rsub.falselist = patch_list_append(rsub.falselist, expr.index);
            }
            patch_list_backpatch(lsub.falselist, rsub.begin);
            ret.truelist = patch_list_merge(lsub.truelist, rsub.truelist);
            ret.falselist = rsub.falselist;
            ret.arithtype = 0;
            break;
            
        case Lnot:
            lsub = triargexpr_gen(root -> left);
            if(lsub.arithtype)
            {
                /* gen expr "TrueJump lsub.subexpr_arg ? " */
                expr = unpatched_condjump_gen(TrueJump, lsub.subexpr_arg);
                insert_triargexpr(expr);
                /* add it to lsub.truelist */
                lsub.truelist = patch_list_append(lsub.truelist, expr.index);
                
                /* gen expr "UncondJump ? ", adding it to rsub.falselist */
                expr.op = UncondJump;
                expr.arg1.type = ExprArg;
                expr.arg1.expr = -1;
                expr.width = -1;
                expr.index = insert_triargexpr(expr);
                lsub.falselist = patch_list_append(lsub.falselist, expr.index);
            }
            ret.truelist = lsub.falselist;
            ret.falselist = lsub.truelist;
            ret.arithtype = 0;
            break;
            
        case Eq:
        case Ge:
        case Le:
        case Neq:
        case Nle:
        case Nge:
            lsub = triargexpr_gen(root -> left);
            if(!lsub.arithtype)
                larith_index = subexpr_arithval_gen(&lsub);
            rsub = triargexpr_gen(root -> right);
            if(!rsub.arithtype)
                rarith_index = subexpr_arithval_gen(&rsub);
            
            expr.op = root -> op;
            expr.width = 4;
            expr.arg1 = lsub.subexpr_arg;
            expr.arg2 = rsub.subexpr_arg;
            expr.index = insert_triargexpr(expr);
            /* lsub not alvie any more, use as temp */
            lsub.subexpr_arg.type = ExprArg;
            lsub.subexpr_arg.expr = expr.index;
            expr = unpatched_condjump_gen(TrueJump, lsub.subexpr_arg);
            expr.index = insert_triargexpr(expr);
            ret.truelist = patch_list_append(ret.truelist, expr.index);
            
            expr.op = UncondJump;
            expr.width = -1;
            expr.arg1.type = ExprArg;
            expr.arg1.expr = -1;
            expr.index = insert_triargexpr(expr);
            ret.falselist = patch_list_append(ret.falselist, expr.index);
            
            /* ret.subexpr_arg not defined for arithtype = 0 */
            ret.arithtype = 0;
            break;
            
        case Plus:
        case Minus:
        case Mul:
        case Subscript:
        case Assign:
            lsub = triargexpr_gen(root -> left);
            if(lsub.arithtype)
                expr.arg1 = lsub.subexpr_arg;
            else
            {
                if(root -> op == Subscript || root -> op == Assign)
                    exit(1); /* op [] must have arith type as left operand */
                expr.arg1.type = ExprArg;
                expr.arg1.expr = subexpr_arithval_gen(&lsub);
            } 
            rsub = triargexpr_gen(root -> right);
            if(rsub.arithtype)
                expr.arg2 = rsub.subexpr_arg;
            else
            {
                expr.arg2.type = ExprArg;
                expr.arg2.expr = subexpr_arithval_gen(&rsub);
            }
            expr.op = root -> op;
            expr.width = get_opresult_width(root -> ast_typetree);
            /* ret.truelist = ret.falselist = NULL */
            ret.subexpr_arg.expr = insert_triargexpr(expr);
            ret.subexpr_arg.type = ExprArg;
            ret.arithtype = 1;
            break;

        case Uplus:
            ret = triargexpr_gen(root -> left);
            break;
            
        case Uminus:
            break;
            
        case Plusplus:
        case Minusminus:
        case Ref:
        case Deref:
        case Funcall:
            expr.op = root -> op;
            expr.width = get_opresult_width(root -> ast_typetree);
            lsub = triargexpr_gen(root -> left);
            if(lsub.arithtype)
                expr.arg1 = lsub.subexpr_arg;
            else
                exit(1);
            
            /* ret.truelist = ret.falselist = NULL */
            ret.subexpr_arg.expr = insert_triargexpr(expr);
            ret.subexpr_arg.type = ExprArg;
            ret.arithtype = 1;
            break;
            
        case Arglist: /* generate one arg expression, but right sub must be scanned */
            /* left subtree first <=> prepare arg form left to right */
            lsub = triargexpr_gen(root -> left);
            if(lsub.arithtype)
                expr.arg1 = lsub.subexpr_arg;
            else
            {
                expr.arg1.type = ExprArg;
                expr.arg1.expr = subexpr_arithval_gen(&lsub);
            }
            
            triargexpr_gen(root -> right);
            
            expr.op = root -> op;
            expr.width = get_opresult_width(root -> ast_typetree);

            /* ret.truelist = ret.falselist = NULL */
            ret.subexpr_arg.expr = insert_triargexpr(expr);
            ret.subexpr_arg.type = ExprArg;
            ret.arithtype = 1;
            break;
            
        default:
            exit(1);
    }
    ret.end = gtriargexpr_table_index - 1;
    return ret;
}

static void free_patch_list(struct patch_list* root)
{
    if(root == NULL)
        return;
    if(root -> next)
        free_patch_list(root -> next);
    free(root);
    return;
}

struct patch_list* patch_list_append(struct patch_list* list, int val)
{
    struct patch_list *new_list = (struct patch_list *)malloc(sizeof(struct patch_list));
    new_list->index = val;
    new_list->next = NULL;
    struct patch_list *temp = list;
    if(temp == NULL)
        return new_list;
    while(temp->next != NULL)
        temp = temp->next;
    temp->next = new_list;
    return list;
}

struct patch_list* patch_list_merge(struct patch_list* list1, struct patch_list* list2)
{
    struct patch_list *temp = list1;
    if(temp == NULL)
        return list2;
    while(temp->next != NULL)
        temp = temp->next;
    temp->next = list2;
    return list1;
}

void patch_list_backpatch(struct patch_list* list, int val)
{
    struct patch_list *temp_list = list;
    if(temp_list == NULL)
        return;
    do
    {
        struct triargexpr *temp_triexpr = gtriargexpr_table + (temp_list->index);
        if( (temp_triexpr->op == TrueJump) || (temp_triexpr->op == FalseJump) )
            temp_triexpr->arg2.expr = val;
        else if(temp_triexpr->op == UncondJump)
            temp_triexpr->arg1.expr = val;
    }
    while((temp_list->next != NULL) && (temp_list = temp_list->next));
    
    free_patch_list(list);
    return;
}

void print_triargexpr(struct triargexpr expr)
{
    printf("(%d) ", expr.index);
    op_printer(expr.op, stdout);
    printf(" ");
    switch(expr.arg1.type)
    {
        case IdArg:
            printf("%s ", expr.arg1.idname);
            break;
        case ImmArg:
            printf("%d ", expr.arg1.imme);
            break;
        case ExprArg:
            printf("(%d) ", expr.arg1.expr);
            break;
    }
    if(expr.op < UncondJump)
    {
        switch(expr.arg2.type)
        {
            case IdArg:
                printf("%s ", expr.arg2.idname);
                break;
            case ImmArg:
                printf("%d ", expr.arg2.imme);
                break;
            case ExprArg:
                printf("(%d) ", expr.arg2.expr);
                break;
        } 
    }
    printf("\n");
    return;
}
