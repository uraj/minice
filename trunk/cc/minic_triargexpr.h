#ifndef __MINIC_TRIARGEXPR_H__
#define __MINIC_TRIARGEXPR_H__

#include "minic_ast.h"
#include "minic_typetree.h"
#include "minic_flowanalyse.h"

enum triarg_type { IdArg = 0, ImmArg, ExprArg };

struct triarg
{
    enum triarg_type type;
    union
    {
        char* idname;
        int expr; /* use index of the triargexpr, not the address */
        int imme;
		struct var_list *func_actvar_list;
    };
};

struct triargexpr
{
    int index;
    enum operator op;
    int width;
    int stride;
    struct triarg arg1, arg2;
};

struct triargexpr_list
{
    struct triargexpr* entity;
    struct var_list * pointer_entity;  
	struct triargexpr_list* prev;
    struct triargexpr_list* next;
};

struct patch_list
{
    int index;
    struct patch_list* next;
};
    
struct subexpr_info
{
    char arithtype;//0 for bool 1 for arithmetic
    struct triarg subexpr_arg;
    int begin, end;
    struct patch_list * truelist, * falselist;
};

/* global variables for triargexpr generater to use*/
extern struct triargexpr_list *ghead, *gtail;
extern struct triargexpr *gtriargexpr_table;
extern int gtriargexpr_table_index;
extern int gtriargexpr_table_bound;

extern struct subexpr_info triargexpr_gen(struct ast* root);
extern int subexpr_arithval_gen(const struct subexpr_info* subexpr);

extern struct patch_list* patch_list_append(struct patch_list* list, int val);
extern struct patch_list* patch_list_merge(struct patch_list* list1, struct patch_list* list2);
extern void patch_list_backpatch(struct patch_list* list, int val);

extern void print_triargexpr(struct triargexpr expr);

#endif
