#ifndef __MINIC_TRIARGTABLE_H_
#define __MINIC_TRIARGTABLE_H_

/*TAOTAOTHERIPPER MARK*/
#include "minic_triargexpr.h"

#define INITIALFUNCSIZE 30
#define INITIALCODESIZE 2000//should be ajusted to triargexpr.h

struct subexpr_info;

struct triargtable
{
	char * funcname;
	struct triargexpr_list *head, *tail;
	struct triargexpr * table;
	struct triargexpr_list ** index_to_list;//This is one way.
	int expr_num;
	int var_id_num;//The number of the variables the function can see
	int table_bound;
};

struct taexpr_list_header/*the name is to long, 
					so use "taexpr" instead of "triargexpr"*//*may need a new header file*/
{
	struct triargexpr_list *head;
	struct triargexpr_list *tail;
	struct patch_list * nextlist;
};

extern struct triargexpr * gtriargexpr_table;
extern struct triargexpr_list *ghead, *gtail;
extern int gtriargexpr_table_bound;
extern int gtriargexpr_table_index;
extern int table_list_bound;
extern int table_list_index;
extern struct triargtable **table_list;
extern int g_var_id_num;//the visible id num for cur func
extern int g_global_id_num;//the global id num

extern int g_table_list_size;//the size of the table list
//function for index_to_list
extern struct triargexpr_list ** new_index_to_list();//Malloc and redirect

extern int insert_triargexpr(struct triargexpr expr);//insert triargexpr to the globaltable, should be in triargexpr.h

extern void new_table_list();
extern void insert_table(struct triargtable * newtable);
extern void free_table_list();//free the global table list
extern struct triargtable * new_table(char *name);//malloc a newtable
extern void free_table(struct triargtable * oldtable);

extern void new_global_table();
extern void free_global_table();
extern struct taexpr_list_header * new_taexprlist(int begin, int end);
extern void free_taexprlist(struct taexpr_list_header * old_list_header);

extern struct taexpr_list_header * expr_list_gen(struct subexpr_info * expr);
extern struct taexpr_list_header * value_list_append(struct subexpr_info * expr);
extern struct taexpr_list_header * if_else_list_merge(struct subexpr_info * condition, 
						struct taexpr_list_header * truestate, struct taexpr_list_header * falsestate);
extern struct taexpr_list_header * if_list_merge(struct subexpr_info * condition, struct taexpr_list_header * truestate);
extern struct taexpr_list_header * while_list_merge(struct subexpr_info * condition, struct taexpr_list_header * loopbody);
extern struct taexpr_list_header * for_list_merge(struct subexpr_info * init_expr, 
						struct subexpr_info * cond_expr, struct subexpr_info * change_expr, struct taexpr_list_header * loopbody);
extern struct taexpr_list_header * return_list_append(struct subexpr_info * value);
extern struct taexpr_list_header * stmt_list_merge(struct taexpr_list_header * pre, struct taexpr_list_header * sub);

extern void print_table(struct triargtable * table);
extern void print_list_header(struct taexpr_list_header * list);
#endif
