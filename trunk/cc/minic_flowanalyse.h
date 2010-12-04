#ifndef __MINIC_FLOW_ANALYSE_H__
#define __MINIC_FLOW_ANALYSE_H__

#include "minic_basicblock.h"
#include "minic_symtable.h"

#define DEFINE 0
#define USE 1

struct var_list
{
     struct var_list_node *head;
     struct var_list_node *tail;
};

extern struct var_list *var_out;

extern struct symbol_table *curr_table;

extern struct var_list *var_list_new();
extern struct var_list_node *var_list_node_new();

extern void var_list_sort(struct var_list *list_array , int size);//将一个定值点链表或者变量编号链表排序
extern void var_list_del_repeate(struct var_list *list);//将排好序的链去重
extern void var_list_free_bynode(struct var_list_node *head);//给定一个节点，释放node以后的链
extern void var_list_free(struct var_list *list);//释放链表
extern void var_list_clear(struct var_list *list);//将链表清空（tail = head , head = NULL）
extern void var_list_append(struct var_list *list , int num);//添加元素到链尾
extern struct var_list_node *var_list_insert(struct var_list *list , struct var_list_node *cur , int num);//插入节点
extern struct var_list_node *var_list_delete(struct var_list *list , struct var_list_node *former , struct var_list_node *del_node);//删除节点，删除后被删除节点指针不可用
extern int var_list_isequal(struct var_list *l1 , struct var_list *l2);//判断两个链表是否相等
extern void var_list_copy(struct var_list *source , struct var_list *dest);//dest=source
extern void var_list_printbynode(struct var_list_node *head);//打印head以后的节点×××××有可能有用
extern void var_list_print(struct var_list *list);//打印链表

extern void var_list_merge(struct var_list *adder , struct var_list *dest);//将变量链adder和dest合并，并写入dest内
extern void var_list_sub(struct var_list *sub , struct var_list *subed , struct var_list *dest);//将变量链dest = sub-subed
extern void var_list_inter(struct var_list *inter , struct var_list *dest);//将inter和dest求交，结果放入dest中


//extern void malloc_active_var();//活跃变量分析中的内存分配函数
//extern void analyse_arg(struct triarg *arg , int type , int block_index);//活跃变量分析中，对一个标号分析
//extern void analyse_arg(int expr_index , int type , int block_index);//活跃变量分析中，对一个arg分析
//extern void initial_active_var()//完成活跃变量分析的初始化部分
//extern void solve_equa_ud();//解活跃变量分析方程组
extern void analyse_actvar();//活跃变量分析

#endif
