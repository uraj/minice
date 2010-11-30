#ifndef __MINIC_SYMTABLE_H__
#define __MINIC_SYMTABLE_H__

#include "minic_typetree.h"
#include "minic_typedef.h"

struct symbol_table;
struct symt_node;
struct syms_node;

extern int g_var_id_num;
extern struct symbol_table *curr_table;

struct value_info
{
     char *name;
     enum modifier modi;
     struct typetree *type;
     struct symbol_table *func_symt;//if it is a function,the member head is its symbol table.Or,it is NULL
     int no;
};

struct symt_node
{
	struct value_info *value;
	struct symt_node *next;
};

struct symbol_table
{
     int size;
     struct symt_node *head;//hash表头
     int arg_no_min , arg_no_max;//如果是函数的话，要有参数，两个变量分别表示参数编号的最小和最大值加1（此处需要注意！）。如果没有则min=max=0
     struct value_info *func;//便于从符号表找到他是什么函数的
};

struct value_type
{
	struct value_info *value;
	struct typetree *cur_typenode;
};

struct syms_node
{
	struct value_type *value;
	struct syms_node *next;
};

struct symbol_stack
{
	struct syms_node *top;
};

extern int ELFhash(char *str);

extern struct symbol_table * symt_new();/*modified*/
extern void var_no_update(struct value_info *value);
extern int symt_insert(struct symbol_table *t , struct value_info *value);
extern void symt_delete(struct symbol_table *t);
extern struct value_info * symt_search(struct symbol_table *t , char *name);
extern struct value_info * symbol_search(struct symbol_table *whole , struct symbol_table *curfunc , char *name);

extern struct value_info * new_valueinfo(char *name);/*modified*/
extern struct value_type * new_valuetype(char *name);/*modified*/
extern void delete_valuetype(struct value_type *v);
extern void delete_valueinfo(struct value_info *v);

extern struct symbol_stack * syms_new();/*modified*/
extern void syms_delete(struct symbol_stack *stack);
extern void syms_push(struct symbol_stack *stack , struct value_type *value);
extern struct value_type *syms_pop(struct symbol_stack *stack);
extern void syms_clear(struct symbol_stack *stack);

#endif
