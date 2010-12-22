#include"minic_symtable.h"
#include"minic_typedef.h"
#include"minic_typetree.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<malloc.h>
#include<assert.h>
#define TABLE_SIZE 31

int ELFhash(char *str)
{
	unsigned long g , h = 0;
	while(*str)
	{
		h = (h << 4) + (*str++);
		g = h & 0xF0000000L;
		if(g)
			h ^= g >> 24;
		h &= ~g;
	}
	return h;
}

static inline struct value_info **new_myid_array(int size)
{
     if(size <= 0)
          return NULL;
     struct value_info **new_myid = (struct value_info **)malloc(sizeof(struct value_info *) * size);
     int i;
     for(i = 0 ; i < size ; i++)
          new_myid[i] = NULL;
     return new_myid;
}

static inline struct string_info *new_conststr_array(int size)
{
     if(size <= 0)
          return NULL;
     struct string_info *new_conststr = (struct string_info *)malloc(sizeof(struct string_info) * size);
     int i;
     for(i = 0 ; i < size ; i++)
     {
          new_conststr[i].string = NULL;
          new_conststr[i].flag = 0;
     }
     return new_conststr;
}

struct symbol_table * symt_new()
{
	struct symbol_table * t = (struct symbol_table *)malloc(sizeof(struct symbol_table));
	t->size = TABLE_SIZE;
	t->head = (struct symt_node *)malloc((sizeof(struct symt_node) * (t->size)));
    int i;
	for(i = 0 ; i < (t->size) ; i++)
	{
		t->head[i].value = NULL;
		t->head[i].next = NULL;
	}
    t->arg_no_min = 0;
    t->arg_no_max = 0;
    t->func = NULL;
    
    t->id_num = 0;
    t->cur_idarray_size = 4;
    t->myid = new_myid_array(t->cur_idarray_size);
    
    t->str_num = 0;
    t->cur_strarray_size = 4;
    t->const_str = new_conststr_array(t->cur_strarray_size);

    g_const_str_num = get_globalstr_num();
    g_var_id_num = get_globalvar_num();
    
	return t;
}

void symt_delete(struct symbol_table *t)
{
	if(t == NULL)
		return;
	struct symt_node *temp , *last;
    int i;
	for(i = 0 ; i < (t->size) ; i++)
	{
		if(t->head[i].value == NULL)
			continue;
		temp = t->head[i].next;
		while(temp != NULL)
		{
			delete_valueinfo(temp->value);
			last = temp;
			temp = temp->next;
			free(last);
		}
	}
	free(t->head);
    free(t->myid);
    free(t->const_str);
	free(t);
}

static void var_no_update(struct symbol_table *t , struct value_info *value)//when an ident is pushed into the symbol
{//table,there is some information to be updated,including ident's No. , count of idents
     if(value == NULL)
          return;
     if(value->type->type == Function)
          return;
     if(t->id_num >= t->cur_idarray_size)//length_varible array's current size is not enough,we must twice it
     {
          struct value_info **temp = t->myid;
          int i;
          t->cur_idarray_size *= 2;
          t->myid = new_myid_array(t->cur_idarray_size);
          for(i = 0 ; i < (t->id_num) ; i++)
               t->myid[i] = temp[i];
          free(temp);
     }
     t->myid[t->id_num] = value;
     value->no = g_var_id_num;
//     printf("%s->%d\n" , t->myid[t->id_num]->name , value->no);
     t->id_num ++;
     g_var_id_num ++;
}

int symt_insert(struct symbol_table *t , struct value_info *value)
{
	if(t == NULL || value == NULL)
		return 0;
	int index = ELFhash(value->name)%(t->size);
	if(t->head[index].value == NULL)
	{
         var_no_update(t , value);
		t->head[index].value = value;
		return 1;
	}
	else
	{
		if((strcmp(t->head[index].value->name , value->name) == 0))//find the same id
		{
			if((t->head[index].value->type->type == Function) && (value->type->type == Function))
			{//if the former value is the declaration of the function,we must connect the symbol table of the function to the former value
				if(typet_type_equal(t->head[index].value->type , value->type) == 0)//the types don't match,the function is redefined
					return 0;
				if(t->head[index].value->func_symt != NULL)//the symbol table of the former function is not empty,it means the same function is defined before
					return 0;
				t->head[index].value->func_symt = value->func_symt;
				value->func_symt = NULL;
				delete_valueinfo(value);
				return 1;
			}
			return 0;
        }
	}
	struct symt_node *temp = t->head + index;
	while(temp->next != NULL)
	{
		if(strcmp(temp->next->value->name , value->name) == 0)
		{
			if((temp->next->value->type->type == Function) && (value->type->type == Function))
			{//if the former value is the declaration of the function,we must connect the symbol table of the function to the former value
				if(typet_type_equal(temp->next->value->type , value->type) == 0)//the types don't match,the function is redefined
					return 0;
				if(temp->next->value->func_symt != NULL)//the symbol table of the former function is not empty,it means the same function is defined before
					return 0;
				temp->next->value->func_symt = value->func_symt;
				value->func_symt = NULL;
				delete_valueinfo(value);
				return 1;
			}
			return 0;//the name is defined twiced
		}
		temp = temp->next;
	}
	struct symt_node *temp_node = (struct symt_node *)malloc(sizeof(struct symt_node));
	var_no_update(t , value);
	temp_node->value = value;
	temp_node->next = NULL;
	temp->next = temp_node;
	return 1;
}

struct token_info symt_insert_conststr(struct symbol_table *t ,char *str)//insert a constant string into symbol table
{
     char id_name[10];
     sprintf(id_name , ".LC%d" , g_const_str_num);
     struct value_info *new_vinfo = new_valueinfo(id_name);
     struct typetree *new_type = typet_new_type(Pointer);
     struct token_info tk;
     tk.sval = id_name;
     new_type->base_type = typet_new_type(Char);
     new_vinfo->type = new_type;
     if(symt_insert(t , new_vinfo) == 0)
     {
          printf("insert constant string error!");
          tk.sval = NULL;
          return tk;
     }
     if(t->str_num == t->cur_strarray_size)
     {
          t->cur_strarray_size *= 2;
          struct string_info *temp = t->const_str;
          t->const_str = new_conststr_array(t->cur_strarray_size);
          int i;
          for(i = 0 ; i < t->str_num ; i++)
               t->const_str[i] = temp[i];
          free(temp);
     }
     t->const_str[t->str_num].string = strdup(str);
     t->str_num ++;
     g_const_str_num++;
//     printf("%s->%s\n" , id_name , t->const_str[t->str_num - 1].string);
     
     return tk;
}

char *get_conststr(struct symbol_table *t , int i)
{
     if(t == NULL)
          return NULL;
     if(i >= t->str_num)
          return NULL;
     if(t->const_str[i].flag == 0)
          return NULL;
     return t->const_str[i].string;
}

int strinfo_setflag(struct symbol_table *t , char *id_name , int flag)
{
     int i = id_name[3] - '0';
     if(t == NULL)
          return 0;
     if(i >= t->str_num)
          return 0;
     t->const_str[i].flag = flag;
     return 1;
}

struct value_info *symt_search(struct symbol_table *t , char *name)
{
	if(t == NULL)
		return NULL;
	int index = ELFhash(name) % (t->size);
	if(t->head[index].value == NULL)
		return NULL;
	else
		if(strcmp(t->head[index].value->name , name) == 0)
			return t->head[index].value;
	struct symt_node *temp = t->head + index;
	while(temp->next != NULL)
	{
		if(strcmp(temp->next->value->name , name) == 0)
			return temp->next->value;
		temp = temp->next;
	}
	return NULL;
}

struct value_info *symbol_search(struct symbol_table *whole , struct symbol_table *curfunc , char *name)
{
	struct value_info *res;
	res = symt_search(curfunc , name);
	if(res != NULL)
		return res;
	res = symt_search(whole , name);
	return res;
}

struct value_info * new_valueinfo(char *name)
{
	struct value_info * i = (struct value_info *)malloc(sizeof(struct value_info));
	i -> name = strdup(name);
	i -> func_symt = NULL;
	i -> type = NULL;
	i -> modi = Epsilon;
    i -> no = 0;
	return i;
}

struct value_info *get_valueinfo_byno(struct symbol_table *cur, int no)//get value_info by no
{
     struct symbol_table *whole = simb_table;
     if(cur == NULL)
     {
          if(whole != NULL && no < whole->id_num)
               return (whole->myid[no]);
          return NULL;
     }
     if(whole == NULL)
     {
          if(no < cur->id_num)
               return (cur->myid[no]);
          return NULL;
     }
     if(no < whole->id_num)
          return (whole->myid[no]);
     no -= whole->id_num;
     if(no < cur->id_num)
          return (cur->myid[no]);
     return NULL;
}

struct value_type * new_valuetype(char *name)
{
	struct value_type * v = (struct value_type *)malloc(sizeof(struct value_type));
	v->value = (struct value_info *)malloc(sizeof(struct value_info));
	v->value->name = strdup(name);
	v->value->func_symt = NULL;
	v->value->type = NULL;
	v->value->modi = Epsilon;
	v->cur_typenode = NULL;
	return v;
}

void delete_valuetype(struct value_type *v)
{
	free(v);
}

void delete_valueinfo(struct value_info *v)
{
	free(v->name);
	typet_free_typetree(v->type);
	symt_delete(v->func_symt);
	free(v);
}

void start_arglist()//参数开始计数
{
     curr_table->arg_no_min = g_var_id_num;
}

void end_arglist()//参数计数结束
{
     curr_table->arg_no_max = g_var_id_num;
}

void get_arg_interval(struct symbol_table *table , int *start , int *end)//获得该符号表的参数开始结束标号
{
     assert((start != NULL) && (end != NULL));
     if(table == NULL)
          return;
     (*start) = table->arg_no_min;
     (*end) = table->arg_no_max - 1;
}

struct symbol_stack * syms_new()
{
    struct symbol_stack * stack = (struct symbol_stack *)malloc(sizeof(struct symbol_stack));
	stack->top = NULL;
	return stack;
}

void syms_delete(struct symbol_stack *stack)
{
	if(stack == NULL)
		return;
	syms_clear(stack);
	free(stack);
}

void syms_push(struct symbol_stack *stack , struct value_type *value)
{
	if(stack == NULL || value == NULL)
		return;
	struct syms_node *node = (struct syms_node *)malloc(sizeof(struct syms_node));
	node->value = value;
	node->next = stack->top;
	stack->top = node;
}

struct value_type *syms_pop(struct symbol_stack *stack)
{
	if(stack == NULL)
		return NULL;
	struct syms_node *node = stack->top;
	struct value_type *value = node->value;
	stack->top = node->next;
	free(node);
	return value;
}

void syms_clear(struct symbol_stack *stack)
{
	if(stack == NULL)
		return;
	struct syms_node *temp;
	while(stack->top != NULL)
	{
		temp = stack->top;
		stack->top = temp->next;
		free(temp);
	}
}

int is_arglist_byno(struct symbol_table *table , int no)//根据变量编号，判断是不是该函数的参数
{
     if(table == NULL)
          return 0;
     return (no >= table->arg_no_min) && (no < table->arg_no_max);
}

int is_globalvar_byno(int no)//根据变量编号，判断是不是全局变量
{
     if(simb_table == NULL)
          return 0;
     return (no >= 0) && (no < simb_table->str_num);
}

int is_conststr_byno(struct symbol_table *table , int no)//根据变量编号，判断是不是常量字符串
{
     struct value_info *temp_info = get_valueinfo_byno(table , no);
     if(temp_info == NULL)
          return 0;
     char *name = temp_info->name;
     if((name[0] == '.') && (name[1] == 'L') && (name[2] = 'C'))
          return 1;
     return 0;
}

int get_localvar_num(struct symbol_table *table)//获得该符号表（函数）的局部变量个数，前提是已经完成了语法分析
{
     if(table == NULL)
          return -1;
     return table->id_num;
}

int get_globalvar_num()//获得该程序的全局变量个数，前提是已经完成了语法分析
{
     return get_localvar_num(simb_table);
}

int get_localstr_num(struct symbol_table *table)//获得该符号表（函数）的常量字符串个数，前提是已经完成了语法分析
{
     if(table == NULL)
          return -1;
     return table->str_num;
}

int get_globalstr_num()//获得该程序的常量字符串个数，前提是已经完成了语法分析
{
     return get_localstr_num(simb_table);
}
