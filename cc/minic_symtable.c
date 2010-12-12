#include"minic_symtable.h"
#include"minic_typedef.h"
#include"minic_typetree.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<malloc.h>

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
	free(t);
}

void var_no_update(struct value_info *value)
{
     if(value == NULL)
          return;
     if(value->type->type == Function)
          return;
     
	value->no = g_var_id_num;
//    printf("%s->%d\n" , value->name , value->no);
	g_var_id_num ++;
}

int symt_insert(struct symbol_table *t , struct value_info *value)
{
	if(t == NULL || value == NULL)
		return 0;
	int index = ELFhash(value->name)%(t->size);
	if(t->head[index].value == NULL)
	{
		var_no_update(value);
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
	var_no_update(value);
	temp_node->value = value;
	temp_node->next = NULL;
	temp->next = temp_node;
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
	return i;
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
