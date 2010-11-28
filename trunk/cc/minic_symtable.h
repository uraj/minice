#ifndef __MINIC_SYMTABLE_H__
#define __MINIC_SYMTABLE_H__

#include "minic_typetree.h"
#include "minic_typedef.h"

struct symbol_table;
struct symt_node;
struct syms_node;


struct value_info
{
	char *name;
	enum modifier modi;
	struct typetree *type;
	struct symbol_table *func_symt;//if it is a function,the member head is its symbol table.Or,it is NULL
};

struct symt_node
{
	struct value_info *value;
	struct symt_node *next;
};

struct symbol_table
{
     int size;
     struct symt_node *head;
     struct value_info *func;
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
