#ifndef __MINIC_SYMTABLE_H__
#define __MINIC_SYMTABLE_H__

#include "minic_typetree.h"
#include "minic_typedef.h"

struct symbol_table;
struct symt_node;
struct syms_node;

extern int g_var_id_num;
extern int g_const_str_num;
extern struct symbol_table *curr_table;
extern struct symbol_table *simb_table;

struct value_info//the information of a ident
{
     char *name;//the name of the ident
     enum modifier modi;//Epsilon,Extern,Register
     struct typetree *type;//the type of the ident
     struct symbol_table *func_symt;//if it is a function,the member head is its symbol table.Or,it is NULL
     int no;//the No. of the ident.We number the idents so that we can use it as the map_index 
};

struct string_info//the information of the constant strings
{
     char *string;//the value
     int flag;//if the string is contained in the assembler file,it is 1;or it is 0
};

struct symt_node//the list_node of the values
{
	struct value_info *value;
	struct symt_node *next;
};

struct symbol_table//the symbol table,used hash
{
     int size;//the size of the hash table
     struct symt_node *head;//hash表头
     int arg_no_min , arg_no_max;//如果是函数的话，要有参数，两个变量分别表示参数编号的最小和最大值加1（此处需要注意！）。如果没有则min=max=0
     struct value_info *func;//便于从符号表找到他是什么函数的

     int id_num;//the number of the idents
     int cur_idarray_size;//the array "myid" is a length_variable array,so we must have the current size
     struct value_info **myid;//the idents of the function

     int str_num;//the number of the idents(maybe needn't)
     int cur_strarray_size;//the array "myid" is a length_variable array,so we must have the current size
     struct string_info *const_str;//the constant strings in the function
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

/*string_info*/
extern char *get_conststr(struct symbol_table *t , int i);
extern int strinfo_setflag(struct symbol_table *t , char *id_name , int flag);

/*value_info*/
extern struct value_info * new_valueinfo(char *name);/*modified*/
extern void delete_valueinfo(struct value_info *v);

/*symbol_table*/
extern struct symbol_table * symt_new();/*modified*/
extern void symt_delete(struct symbol_table *t);

extern int symt_insert(struct symbol_table *t , struct value_info *value);
extern struct token_info symt_insert_conststr(struct symbol_table *t , char *str);//insert a constant string into the symbol table

extern struct value_info * symt_search(struct symbol_table *t , char *name);
extern struct value_info * symbol_search(struct symbol_table *whole , struct symbol_table *curfunc , char *name);

extern void start_arglist();//参数开始计数
extern void end_arglist();
extern struct value_info *get_valueinfo_byno(struct symbol_table *cur_table , int no);//get value_info by No.

/*value_type*/
extern struct value_type * new_valuetype(char *name);/*modified*/
extern void delete_valuetype(struct value_type *v);

/*symbol_stack*/
extern struct symbol_stack * syms_new();/*modified*/
extern void syms_delete(struct symbol_stack *stack);
extern void syms_push(struct symbol_stack *stack , struct value_type *value);
extern struct value_type *syms_pop(struct symbol_stack *stack);
extern void syms_clear(struct symbol_stack *stack);

/*other*/
extern int ELFhash(char *str);

#endif
