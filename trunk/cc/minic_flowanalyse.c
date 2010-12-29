#include "minic_flowanalyse.h"
#include "minic_varmapping.h"
#include "minic_triargexpr.h"
#include "minic_symtable.h"
#include <stdio.h>
#include <stdlib.h>

enum change_type
{
     Add,
     Del
};

struct actvar_change
{
     int var_map_index;
     enum change_type type;
};

static struct symbol_table *cur_func_sym_table;
static struct value_info *cur_func_vinfo;
static struct triargtable *cur_func_triarg_table;

static struct var_list *var_in;
static struct var_list *var_out;
static struct var_list *def;
static struct var_list *use;
static int *def_size;
static int *use_size;
static int *is_actvar;
static int s_expr_num;//the num of the tri-expressions
static int sg_max_func_varlist = 0;//每条Funcall语句都会接有一条当前的活跃变量链

struct var_list begin_var_list;//程序开头处活跃的变量
struct var_list def_g_list;//当前函数中所有可能被定值过的全局变量
int def_g_num;//当前函数中所有可能被定值过的全局变量

static inline int compare (const void * a, const void * b) 
{
	return ( *(int*)a - *(int*)b );
}

inline void var_list_printbynode(struct var_list_node *head)
{
	if(head == NULL)
	{
		printf("EMPTY\n");
		return;
	}
	struct var_list_node *temp = head;
	while(temp != NULL)
	{
		printf("%d ",temp->var_map_index);
		temp = temp->next;
	}
	printf("\n");
}

inline void var_list_print(struct var_list *list)
{
	if(list == NULL)
	{
		printf("NULL\n");
		return;
	}

	if(list->head == NULL)
	{
		printf("EMPTY\n");
		return;
	}
	struct var_list_node *temp = list->head;
	while(temp != list->tail->next)
	{
		printf("%d ",temp->var_map_index);
		temp = temp->next;
	}
	printf("\n");
}

struct var_list *var_list_new()
{
     struct var_list *new_list = (struct var_list *)malloc(sizeof(struct var_list));
     new_list->head = new_list->tail = NULL;
     return new_list;
}

struct var_list_node *var_list_node_new(int num)
{
     struct var_list_node *new_node = (struct var_list_node *)malloc(sizeof(struct var_list_node));
     new_node->var_map_index = num;
     new_node->next = NULL;
     return new_node;
}

void var_list_sort(struct var_list *list_array , int size)//将一个定值点链表或者变量编号链表排序
{
     if(list_array == NULL)
          return;
	int j;
	int *trans_array;
	struct var_list_node *temp_node;
	trans_array = (int *)malloc(sizeof(int) * size);
	temp_node = list_array->head;
	for(j = 0 ; j < size ; j++)
	{
		if(temp_node == NULL)
		{
			printf("WARNING!Your var_list is not complete(Should have %d,but only have %d).Please check!\n" , size , j);
            size = j;
            break;
		}
		trans_array[j] = temp_node->var_map_index;
		temp_node = temp_node->next;
	}
	qsort(trans_array , size , sizeof(int) , compare);
	temp_node = list_array->head;
	for(j = 0 ; j < size ; j++)
	{
		temp_node->var_map_index = trans_array[j];
		temp_node = temp_node->next;
	}
	free(trans_array);
}

int var_list_count(struct var_list *list)
{
     if(list == NULL)
          return 0;
     if(list->head == NULL)
          return 0;
     struct var_list_node *temp = list->head;
     int count = 0;
     while(temp != list->tail->next)
     {
          count++;
          temp = temp->next;
     }
     return count;
}

void var_list_del_repeate(struct var_list *list)//将排好序的链去重
{
     if(list == NULL)
          return;
     if(list->head == NULL)
          return;
     if(list->head->next == NULL)
          return;
     struct var_list_node *cur , *former;
     former = list->head;
     cur = former->next;
     while(cur != list->tail->next)
     {
          if(cur->var_map_index == former->var_map_index)
          {
               var_list_delete(list , former , cur);
               cur = former->next;
               continue;
          }
          former = cur;
          cur = cur->next;
     }
}

inline void var_list_free_bynode(struct var_list_node *head)
{
	if(head == NULL)
		return;
	var_list_free_bynode(head->next);
	free(head);
}

inline void var_list_free(struct var_list *list)
{
     if(list == NULL)
          return;
     if(list->head == NULL)
          list->head = list->tail;
     var_list_free_bynode(list->head);
     free(list);
}

inline void var_list_clear(struct var_list *list)
{
     if(list == NULL)
          list = var_list_new();
	if(list->head == NULL)
		return;
	list->tail = list->head;
	list->head = NULL;
}

inline struct var_list *var_list_append(struct var_list *list , int num)
{
//     printf("*append%d* " , num);//*********************************
     if(list == NULL)
          list = var_list_new();
	if(list->head == NULL && list->tail != NULL)
	{
		list->head = list->tail;
		list->tail->var_map_index = num;
		return list;
	}
	if(list->head != NULL && list->tail->next != NULL)//如果list的tail后面还有节点，则直接利用，避免浪费
	{
		list->tail = list->tail->next;
		list->tail->var_map_index = num;
		return list;
	}
	struct var_list_node *cur_node = var_list_node_new(num);
	if(list->head == NULL)
	{
		list->head = list->tail = cur_node;
		return list;
	}
	list->tail->next = cur_node;
	list->tail = cur_node;
    return list;
}

struct var_list_node *var_list_insert(struct var_list *list , struct var_list_node *cur , int num)//若cur＝NULL，默认插在表头，返回指针指向新插入节点
{
     if(list == NULL)
          return NULL;
     struct var_list_node *in_node = var_list_node_new(num);
     if(cur == NULL)
     {
          if(list->head == NULL)
          {
               list->head = in_node;
               list->head->next = list->tail;
               return in_node;
          }
          in_node->next = list->head;
          list->head = in_node;
          return in_node;
     }
     struct var_list_node *temp = cur->next;
     cur->next = in_node;
     in_node->next = temp;
     return in_node;
}

struct var_list_node *var_list_delete(struct var_list *list , struct var_list_node *former , struct var_list_node *del_node)//删除节点，删除后被删除节点指针不可用，返回指针为删除节点的后一个节点
{
     if(list == NULL)
          return NULL;
     if(list->head == NULL)//链表为空
     {
          fprintf(stderr , "Delete error!The list is EMPTY!\n");
          return NULL;
     }
	if(del_node == NULL)
		return NULL;
    if(del_node == list->head)//删除的链表的第一个节点
    {
         if(list->head == list->tail)//链表只有一个元素，需要head＝NULL，tail后移，释放头节点
         {
              list->tail = list->head->next;
              list->head = NULL;
              free(del_node);
              return list->tail;//***************************此处待定
         }
         else
         {
              list->head = list->head->next;
              free(del_node);
              return list->head;
         }
    }
    else if(del_node == list->tail)//删除的是链表最后一个节点
    {
         if(former == NULL)
         {
              fprintf(stderr , "Delete node error!Can't find the former node.\n");
              return NULL;
         }
         list->tail = former;
         list->tail->next = del_node->next;
         free(del_node);
         return list->tail->next;
    }
    else
    {
         if(former == NULL)
         {
              fprintf(stderr , "Delete node error!Can't find the former node.\n");
              return NULL;
         }
         former->next = del_node->next;
         free(del_node);
         return former->next;
    }
}

int var_list_isequal(struct var_list *l1 , struct var_list *l2)
{
     if(l1 == NULL || l2 == NULL)
          return -1;
     struct var_list_node *temp1 = l1->head;
     struct var_list_node *temp2 = l2->head;
     if(temp1 == NULL && temp2 == NULL)//考虑链表为空的情况
          return 1;
     if(temp1 == NULL || temp2 == NULL)
          return 0;
     while(1)
     {
          if(temp1 == l1->tail->next && temp2 == l2->tail->next)
               return 1;
          if(temp1 == l1->tail->next || temp2 == l2->tail->next)
               return 0;
          if(temp1->var_map_index != temp2->var_map_index)
               return 0;
          temp1 = temp1->next;
          temp2 = temp2->next;
     }
     return 1;
}

struct var_list *var_list_copy(struct var_list *source , struct var_list *dest)
{
     if(source == NULL)
          return NULL;
     if(dest == NULL)
          dest = var_list_new();
     var_list_clear(dest);
     if(source->head == NULL)
          return dest;
     struct var_list_node *temp = source->head;
     while(temp != source->tail)
     {
          var_list_append(dest , temp->var_map_index);
          temp = temp->next;
     }
     var_list_append(dest , temp->var_map_index);
     return dest;
}

struct var_list_node *var_list_find(struct var_list *list , int key)
{
     if(list == NULL)
          return NULL;
     if(list->head == NULL)
          return NULL;
     struct var_list_node *temp = list->head;
     while(temp != list->tail->next)
     {
          if(temp->var_map_index == key)
               return temp;
          temp = temp->next;
     }
     return NULL;
}

struct var_list *var_list_merge(struct var_list *adder , struct var_list *dest)//将变量链adder和dest合并，并写入dest内
{
     if(adder == NULL)
          return dest;
     if(dest == NULL)
          dest = var_list_new();
     if(adder->head == NULL)
          return dest;
     if(dest->head == NULL)
          return var_list_copy(adder , dest);
     
     struct var_list_node *adder_pointer = adder->head;
     struct var_list_node *dest_pointer = dest->head;
     struct var_list_node *former_dest_p = NULL;
     while(1)
     {
          if(adder_pointer->var_map_index == dest_pointer->var_map_index)
          {
               adder_pointer = adder_pointer->next;
               former_dest_p = dest_pointer;
               dest_pointer = dest_pointer->next;
          }
          else if(adder_pointer->var_map_index > dest_pointer->var_map_index)
          {
               former_dest_p = dest_pointer;
               dest_pointer = dest_pointer->next;
          }
          else
          {
               former_dest_p = var_list_insert(dest , former_dest_p , adder_pointer->var_map_index);//***********
               adder_pointer = adder_pointer->next;
          }
          if(adder_pointer == adder->tail->next)
               break;
          if(dest_pointer == dest->tail->next)
          {
               while(adder_pointer != adder->tail->next)
               {
                    var_list_append(dest , adder_pointer->var_map_index);
                    adder_pointer = adder_pointer->next;
               }
               break;
          }
     }
     return dest;
}

struct var_list *var_list_sub(struct var_list *sub , struct var_list *subed , struct var_list *dest)//dest = sub - subed
{
     if(sub == NULL || subed == NULL)
          return NULL;
	var_list_clear(dest);
    if(dest == NULL)
         dest = var_list_new();
	if(sub->head == NULL)
		return dest;
	if(subed->head == NULL)
		return var_list_copy(sub , dest);
    
	struct var_list_node *sub_pointer = sub->head;
	struct var_list_node *subed_pointer = subed->head;
	while(1)
	{
		if(sub_pointer->var_map_index == subed_pointer->var_map_index)
		{
			sub_pointer = sub_pointer->next;
			subed_pointer = subed_pointer->next;
		}
		else if(sub_pointer->var_map_index > subed_pointer->var_map_index)
		{
			subed_pointer = subed_pointer->next;
		}
		else
		{
			var_list_append(dest , sub_pointer->var_map_index);
			sub_pointer = sub_pointer->next;
		}
		if(sub_pointer == sub->tail->next)
			break;
		if(subed_pointer == subed->tail->next)
		{
			while(sub_pointer != sub->tail->next)
			{
				var_list_append(dest , sub_pointer->var_map_index);
				sub_pointer = sub_pointer->next;
			}
			break;
		}
	}
    return dest;
}

struct var_list *var_list_inter(struct var_list *inter , struct var_list *dest)//dest = inter & dest
{
     if(inter == NULL)
          return NULL;
     if(dest == NULL)//return NULL or return new()
          return NULL;
	if(inter->head == NULL)
	{
		var_list_clear(dest);
		return dest;
	}
	if(dest->head == NULL)
		return dest;
	struct var_list_node *inter_pointer = inter->head;
	struct var_list_node *dest_pointer = dest->head;
	struct var_list_node *dest_former = NULL;
	while(1)
	{
		if(inter_pointer->var_map_index == dest_pointer->var_map_index)
		{
			inter_pointer = inter_pointer->next;
			dest_former = dest_pointer;
			dest_pointer = dest_pointer->next;
		}
		else if(inter_pointer->var_map_index > dest_pointer->var_map_index)
		{
            dest_pointer = var_list_delete(dest , dest_former , dest_pointer);
		}
		else
			inter_pointer = inter_pointer->next;
		if(inter_pointer == inter->tail->next)
		{
			dest->tail = dest_former;
			break;
		}
        if(dest->head == NULL)
             break;
		if(dest_pointer == dest->tail->next)
			break;
	}
    return dest;
}

static inline int is_mapid_available(int map_id)
{
     if(map_id == -1)
          return 0;
     if(is_global(map_id) == 1)
          return 1;
     if(is_array(map_id) == 1)
          return 0;
     return 1;
}

static inline int is_local_array(int mapid)
{
     if(mapid < 0)
          return 0;
     if(is_global(mapid) == 1)
          return 0;
     if(is_array(mapid) == 1)
          return 1;
     return 0;
}

static inline void analyse_map_index(int i , int type , int block_index)
{
     if(is_mapid_available(i) == 0)
          return;
     if(option_show_flow_debug == 1)
     {
          if(type == DEFINE)
               printf("DEFINE ");
          else
               printf("USE ");
     }
     struct var_info *temp;
     temp = get_info_from_index(i);
     if(temp == NULL)
     {
//          printf("Can't get var_info of map_id %d\n" , i);//*********************8
          return;
     }
     if(type == DEFINE)
     {
          temp->is_define = block_index;
          if(temp->is_use != block_index)
          {
               var_list_append(def + block_index , i);
               def_size[block_index]++;
          }
     }
     else if(type == USE)
     {
          temp->is_use = block_index;
          if(temp->is_define != block_index)
          {
               var_list_append(use + block_index , i);
               use_size[block_index]++;
          }
     }
     if(option_show_flow_debug == 1)
          printf("\n");
}

static inline void analyse_arg(struct triarg *arg , int type , int block_index)//活跃变量分析中的内存分配函数
{
     int i;
     if(arg->type == IdArg)
     {
          i = get_index_of_id(arg->idname);
          if(is_local_array(i) == 1)//局部数组，不分析
               return;
          if(option_show_flow_debug == 1)
               printf("      mapid:%d->idname:%s " , i , arg->idname);
     }
     else if(arg->type == ExprArg)
     {
          if(arg->expr == -1)
               return;
          i = get_index_of_temp(arg->expr);
//          struct triargexpr_list *temp_expr_node = cur_func_triarg_table->index_to_list[arg->expr];
//          struct triargexpr *temp_expr = temp_expr_node->entity;
//          int change = 0;
//          if(temp_expr->op == Deref)//*p
//          {
//               struct var_list *temp_point_list = temp_expr_node->pointer_entity;
//               printf("//************************(%d)" , temp_expr->index);
//               var_list_print(temp_point_list);
/*               if(temp_point_list != NULL
                  && temp_point_list->head != NULL
                  &&temp_point_list->head == temp_point_list->tail)//只有一个元素，直接将该指针换成对应实体变量
               {
                    struct var_info *temp_var_info = get_info_from_index(temp_point_list->head->var_map_index);
                    if(temp_var_info->ref_point == NULL)//此指针为数组
                    {
                         arg->type = IdArg;
                         arg->idname=(get_valueinfo_byno(cur_func_sym_table,temp_point_list->head->var_map_index))->name;
                         i = get_index_of_id(arg->idname);
                         change = 1;
                    }
                    }
                    }*/
          if(option_show_flow_debug == 1)
          {
//               if(change == 0)
               printf("      mapid:%d->expr:%d " , i , arg->expr);//*************************
               if(i == -1)
                    printf("\n");
//               else
//                    printf("      mapid:%d->idname:%s " , i , arg->idname);//*****************************
          }
     }
     else
          return;
     if(i == -1)
          return;
     analyse_map_index(i ,type , block_index);
}

static inline void analyse_expr_index(int expr_index , int type , int block_index)
{
     int i = get_index_of_temp(expr_index);
     if(i == -1)
          return;
     if(option_show_flow_debug == 1)
          printf("      mapid:%d->expr:%d " , i , expr_index);
     analyse_map_index(i ,type , block_index);
}

static void malloc_active_var()
{
     var_in = (struct var_list *)malloc((sizeof(struct var_list)) * g_block_num);
     var_out = (struct var_list *)malloc((sizeof(struct var_list)) * g_block_num);
     def = (struct var_list *)malloc((sizeof(struct var_list)) * g_block_num);
     use = (struct var_list *)malloc((sizeof(struct var_list)) * g_block_num);
     def_size = (int *)malloc((sizeof(int)) * g_block_num);
     use_size = (int *)malloc((sizeof(int)) * g_block_num);
     is_actvar = (int *)calloc(get_ref_var_num() , sizeof(int));
     
     int i;
     for(i = 0 ; i < g_block_num ; i++)
     {
          var_in[i].head = var_in[i].tail = NULL;
          var_out[i].head = var_out[i].tail = NULL;
          def[i].head = def[i].tail = NULL;
          use[i].head = use[i].tail = NULL;
          def_size[i] = use_size[i] = 0;
     }
}

static void initial_func_var(int func_index)//通过函数index获得当前函数对应的函数信息，符号表和三元式链表
{
     cur_func_vinfo = symt_search(simb_table ,table_list[func_index]->funcname);
     cur_func_sym_table = cur_func_vinfo->func_symt;
     cur_func_triarg_table = table_list[func_index];
     
     /*由于一个数组变量的所有数组元素对应定值点的map_id构成的链ref_point在形成时没有排序去重，所以这里补充了这个工作。*/
     /*int total_id_num = simb_table->id_num + cur_func_sym_table->id_num;
     int i;
     struct var_info *temp_var_info;
     for(i = 0 ; i < total_id_num ; i++)
     {
          temp_var_info = get_info_from_index(i);
          if(temp_var_info == NULL)
               continue;
//          printf("%s  " , get_valueinfo_byno(cur_func_sym_table , i)->name);
          if(temp_var_info->ref_point == NULL)
               continue;
//          printf("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$");
//          var_list_print(temp_var_info->ref_point);
          struct var_list_node *cur_node = temp_var_info->ref_point->head;
          struct var_list_node *former_node = NULL;
          if(cur_node == NULL)
               continue;
          int map_id;
          while(cur_node != temp_var_info->ref_point->tail->next)
          {
               map_id = get_index_of_temp(cur_node->var_map_index);
               if(map_id < 0)
               {
                    cur_node = var_list_delete(temp_var_info->ref_point , former_node , cur_node);
                    continue;
               }
               cur_node->var_map_index = map_id;
               former_node = cur_node;
               cur_node = cur_node->next;
          }
          var_list_sort(temp_var_info->ref_point , var_list_count(temp_var_info->ref_point));
          var_list_del_repeate(temp_var_info->ref_point);
//          var_list_print(temp_var_info->ref_point);
     }*/
}

static int get_arg_index(struct triarg arg)
{
     if(arg.type == ExprArg)
     {
          if(arg.expr == -1)
               return -3;
          return get_index_of_temp(arg.expr);
     }
     if(arg.type == IdArg)
          return get_index_of_id(arg.idname);
     if(arg.type == ImmArg)
          return -2;
     return -1;
}

static void initial_active_var()//活跃变量分析的初始化部分def和use
{
     /*将本函数中所有可能定值的全局变量拉成一个链*/
     def_g_num = 0;
     s_expr_num = 0;
     def_g_list.head = def_g_list.tail = NULL;
     int global_var_num = get_globalvar_num();
     int i;
     for(i = 0 ; i < global_var_num ; i++)
     {
          if(defed_gvar[i] == 1)
          {
               def_g_num++;
               var_list_append(&def_g_list , i);
          }
     }
     var_list_sort(&def_g_list , def_g_num);
     
     struct triargexpr_list *temp;
     for(i = 0 ; i < g_block_num ; i++)
     {
          if(option_show_flow_debug == 1)
               printf("block:%d\n" , i);
/*          if(i == 0)//第一个块要把函数参数和全局变量放入def[0]当中
          {
               //int start = cur_func_sym_table->arg_no_min;
               //int end = cur_func_sym_table->arg_no_max;
               int global_var_num = get_globalvar_num();
               int j , start , end;
               get_arg_interval(cur_func_sym_table , &start , &end);
               for(j = 0 ; j < global_var_num ; j++)
               {
                    if(is_mapid_available(j) == 0)
                         continue;
                    def_size[0] ++;
                    var_list_append(def , j);
               }
               for(j = start ; j < end ; j++)
               {
                    def_size[0] ++;
                    var_list_append(def , j);
               }
          }
*/
          temp = DFS_array[i]->head;
          /*
            单条语句分析步骤：
            1、对于赋值。arg1=arg2
                a)首先，如果arg1是标号(k)而且k没有map_id，需要向上看一步，如果k是*p或者a[i]，要先分析语句k；
                b)分析本条赋值语句。
            2、对于条件跳转。
                a)首先，如果arg1是标号(k)而且k没有map_id，这意味着这个位置是一个逻辑条件指令，需要向上看一步，分析该三元式；
                b)分析本条条件跳转指令。
            3、对于a+b、a-b、*p、a[i]、+a、-a、&a以及所有逻辑指令;
                a)如果expr->entity->index没有map_id，说明这一句话从来没被引用过，没必要生成代码，也就没必要分析；
                b)分析本条语句。
            4、其他语句，根据要分析的操作数个数和位置，分以下几种情况：
                a)a++、a--
                b)Return、Arglist
           */
          while(temp != NULL)
          {
               /**/
//               print_triargexpr(*(temp->entity));
//               int k = get_index_of_temp(temp->entity->index);
               s_expr_num ++;
//               printf("      mapid:%d->expr:%d\n" , k , temp->entity->index);
               /**/
               switch(temp->entity->op)
               {
                    /*赋值语句*/
               case Assign:
                    analyse_expr_index(temp->entity->index , DEFINE , i);
                    analyse_arg(&(temp->entity->arg1) , DEFINE , i);
                    analyse_arg(&(temp->entity->arg2) , USE , i);
                    if(temp->entity->arg1.type == ExprArg && get_index_of_temp(temp->entity->arg1.expr) == -1)
                    {
                         int last_index = temp->entity->arg1.expr;
                         struct triargexpr_list *last_expr_node = cur_func_triarg_table->index_to_list[last_index];
                         struct triargexpr *last_expr = last_expr_node->entity;
                         if(last_expr->op == Subscript)
                         {
                              analyse_arg(&(last_expr->arg1) , USE , i);
                              analyse_arg(&(last_expr->arg2) , USE , i);
                         }
                         else if(last_expr->op == Deref)
                              analyse_arg(&(last_expr->arg1) , USE , i);
                    }
                    break;

                    /*二元操作*/
               case Land:
               case Lor:
               case Eq:
               case Neq:
               case Ge:
               case Le:
               case Nge:
               case Nle:
               case Plus:
               case Minus:
               case Mul:
               case Subscript:
                    if(get_index_of_temp(temp->entity->index) == -1)
                         break;
                    analyse_expr_index(temp->entity->index , DEFINE , i);
                    analyse_arg(&(temp->entity->arg1) , USE , i);
                    analyse_arg(&(temp->entity->arg2) , USE , i);
                    break;

                    /*一元操作*/
               case Lnot:
               case Uplus:
               case Uminus:
               case Ref:
               case Deref:
                    if(get_index_of_temp(temp->entity->index) == -1)
                         break;
               case Plusplus:
               case Minusminus:
                    analyse_expr_index(temp->entity->index , DEFINE , i);
                    analyse_arg(&(temp->entity->arg1) , USE , i);
                    break;

                    /*跳转及函数相关语句*/
               case TrueJump:
               case FalseJump://逻辑跳转要往上看一步
                    if(get_arg_index(temp->entity->arg1) == -1)
                    {
                         int last_index = temp->entity->arg1.expr;
                         struct triargexpr_list *last_expr_node = cur_func_triarg_table->index_to_list[last_index];
                         struct triargexpr *last_expr = last_expr_node->entity;
                         analyse_arg(&(last_expr->arg1) , USE , i);
                         analyse_arg(&(last_expr->arg2) , USE , i);
                    }
                    else
                         analyse_arg(&(temp->entity->arg1) , USE , i);
                    break;
               case Arglist:
               case Return:
                    analyse_arg(&(temp->entity->arg1) , USE , i);
                    break;

                    /*大立即数等等*/
               default:
                    analyse_expr_index(temp->entity->index , DEFINE , i);
                    break;
               }
               if(option_show_flow_debug == 1)
               {
                    print_triargexpr(*(temp->entity));
                    printf("\n");
               }
               temp = temp->next;
          }
          
          /*由于全局变量的最后一次赋值必须要写回内存，所以为了统一，必须让所有可能被定值的全局变量在程序出口处活跃，例如：
            (1) = a , 1;
            (2) = a , 0;
            (3) Return 0;
            如果a是局部变量，(1)和(2)都没必要做，这也符合活跃变量的做法；但如果a是全局变量，(1)不用做但(2)必须做，这按照普通的活跃变量分析是无法解决的，所以只需要让a在(3)之后也是活跃的，就可以解决这个问题。当然这么做会加大寄存器分配的负担，但是本着全局变量尽量少用的原则，又考虑到可用寄存器十分充足，所以折中了一下。
           */
          var_list_sort(def + i , def_size[i]);//when DEFs and USEs are made
          var_list_sort(use + i , use_size[i]);//sort them so that we can op
          var_list_del_repeate(def + i);
          var_list_del_repeate(use + i);
          if(option_show_flow_debug == 1)
               printf("\n");
     }
     free(def_size);
     free(use_size);
}

static void solve_equa_ud()//求解活跃变量方程组
{
     int i;
     int change = 1 , next;
     struct basic_block_list *temp_block;
     struct var_list *temp_list;
     temp_list = var_list_new();

     while(change)
     {
          change = 0;
          for(i = g_block_num - 1; i >= 0 ; i--)
          {
               //var_out[Bi] = +var_in[s];s属于NEXT[Bi]
               temp_block = DFS_array[i]->next;
               var_list_clear(var_out + i);//clear the v_out of the block before op
               
               if(is_end_block(i) == 1)//该block可能是结尾block，此时将该函数中所有可能被定值的全局变量都放到var_out里面
               {
                    var_list_copy(&def_g_list , var_out + i);
               }
               
               while(temp_block != NULL)
               {
                    next = temp_block->entity->index;
                    var_list_merge(var_in + next , var_out + i);
                    temp_block = temp_block->next;
               }
               //var_in[Bi] = use[Bi] + (var_out[Bi] - def[Bi]);
               var_list_sub(var_out + i , def + i , temp_list);
               var_list_merge(use + i , temp_list);
               if(var_list_isequal(var_in + i , temp_list) == 0)
               {
                    change = 1;
                    var_list_copy(temp_list , var_in + i);
               }
          }
     }
     begin_var_list.head = begin_var_list.tail = NULL;
	 var_list_copy(var_in , &begin_var_list);
     for(i = 0 ; i < g_block_num ; i++)//解方程组后，def，use和var_in都没用了
     {
          if(option_show_active_var == 1)
          {
               printf("DEF %d :" , i);
               var_list_print(def + i);
               printf("USE %d :" , i);
               var_list_print(use + i);
               printf("VIN %d :" , i);
               var_list_print(var_in + i);
               printf("OUT %d :" , i);
               var_list_print(var_out + i);
               printf("\n");
          }
          var_list_free_bynode(def[i].head);
          var_list_free_bynode(use[i].head);
          var_list_free_bynode(var_in[i].head);
     }
     free(def);
     free(use);
     free(var_in);
}

int get_max_func_varlist()
{
     return sg_max_func_varlist;
}

int is_active_var(int mapid)
{
     if(mapid >= 0)
          return is_actvar[mapid];
     return 0;
}

static int add_actvar_info(struct var_list *list)
{
     if(list == NULL)
          return 0;
     if(list->head == NULL)
          return 0;
     struct var_list_node *temp = list->head;
     int count = 0;
     while(temp != list->tail->next)
     {
          count++;
          if(temp->var_map_index >= 0)
               is_actvar[temp->var_map_index] = 1;
          temp = temp->next;
     }
     return count;
}

/*
  get the map_index of arg.
 */
static inline int get_index_of_arg(struct triarg *arg , struct var_list **dest)
{
     if(arg->type == IdArg)
     {
          int i = get_index_of_id(arg->idname);
          if(is_local_array(i) == 1)
               return -1;
          return i;
     }
     else if(arg->type == ExprArg)
     {
          if(arg->expr == -1)
               return -1;
/*          struct triargexpr_list *temp_expr_node = cur_func_triarg_table->index_to_list[arg->expr];
          struct triargexpr *temp_expr = temp_expr_node->entity;
          if(temp_expr->op == Deref)p
          {
               struct var_list *temp_point_list = temp_expr_node->pointer_entity;
               if(dest == NULL)//此arg为引用编号(temp_point_list == NULL)
               {
                    if(temp_point_list == NULL)
                         return -2;
                    if(temp_point_list->head == NULL)
                         return -2;
                    if(temp_point_list->head == temp_point_list->tail)//只有一个元素
                    {
                         struct var_info *temp_var_info = get_info_from_index(temp_point_list->head->var_map_index);
                         if(temp_var_info->ref_point != NULL)//此指针为数组
                         {
                              var_list_free(temp_point_list);
                              //temp_expr_node->pointer_entity = temp_var_info->ref_point;//实体链指向数组的定点链
                              //     (*dest) = temp_var_info->ref_point;
                              return get_index_of_temp(arg->expr);
                         }
                         arg->type = IdArg;
                         arg->idname=(get_valueinfo_byno(cur_func_sym_table,temp_point_list->head->var_map_index))->name;
                         return temp_point_list->head->var_map_index;
                    }
                    return get_index_of_temp(arg->expr);
               }
               if(temp_point_list == NULL)//按说所有变量在这一句都活跃，但是这种现象的出现意味着程序可能不正确，为了简单不做处理，返回-2
                    return -2;
               if(temp_point_list->head == NULL)
                    return -2;//按说所有变量在这一句都活跃，但是这种现象的出现意味着程序可能不正确，为了简单，不做处理，返回-2
               if(temp_point_list->head == temp_point_list->tail)//只有一个元素
               {
                    struct var_info *temp_var_info = get_info_from_index(temp_point_list->head->var_map_index);
                    if(temp_var_info->ref_point != NULL)//此指针为数组
                    {
                         var_list_free(temp_point_list);
                         //temp_expr_node->pointer_entity = temp_var_info->ref_point;//实体链指向数组的定点链
                         //(*dest) = temp_var_info->ref_point;//该数组中所有元素的三元式编号的map_id都需要从活跃变量中删除
                         return get_index_of_temp(arg->expr);
                    }
                    arg->type = IdArg;
                    arg->idname = (get_valueinfo_byno(cur_func_sym_table , temp_point_list->head->var_map_index))->name;
                    return temp_point_list->head->var_map_index;
               }
               (*dest) = temp_point_list;
               return (get_index_of_temp(arg->expr));
          }
*/
          return (get_index_of_temp(arg->expr));
     }
     else
          return -1;
}

static void make_change_list(int num1 , int num2 , struct var_list *dest)
{
     var_list_clear(dest);
     int temp;
     if(num1 > num2)
     {
          temp = num1;
          num1 = num2;
          num2 = temp;
     }
     if(num1 >= 0)
          var_list_append(dest , num1);
     if(num2 >= 0 && num1 != num2)
          var_list_append(dest , num2);
}

int is_assign(struct triargexpr *expr , int dest_index)//判断一个赋值语句是否有必要分析
{
     if(expr == NULL)
     {
          if(dest_index < 0)
               return 0;
          return 1;
     }
     if(dest_index < 0)
          return 0;
/*     if(is_global(dest_index) == 1)
     {
          if(arg_index == -2)
               return 1;
          if(alloc_reg.result[dest_index] == alloc_reg.result[arg_index])//两个变量分配了同一寄存器，该赋值无用
               return 0;
          return 1;
     }
*/
     if(var_list_find(expr->actvar_list , dest_index) == NULL)//
          return 0;
     return 1;
}

struct var_list *analyse_actvar(int *expr_num , int func_index)//活跃变量分析
{
     initial_func_var(func_index);//通过函数index获得当前函数信息，以便后续使用
     malloc_active_var();//活跃变量分析相关的数组空间分配
     initial_active_var();//完成活跃变量分析的初始化部分，生成def和use
     solve_equa_ud();//求解活跃变量方程组，得到var_in和var_out
     (*expr_num) = s_expr_num;//the num of expressions

     int i , add1 , add2 , del1 , del2 , flag_first;
     int act_list_index = 0 , count;
     struct var_list *actvar_list;//
     actvar_list = (struct var_list *)malloc(sizeof(struct var_list) * s_expr_num);
     for(i = 0 ; i < s_expr_num ; i++)
          actvar_list[i].head = actvar_list[i].tail = NULL;
     
     struct var_list flush_list;//temp list of active varible
     flush_list.head = flush_list.tail = NULL;
     struct var_list *del_list = var_list_new();
     struct var_list *next_del_list = var_list_new();
     struct var_list *add_list = var_list_new();
     struct var_list *point_list = NULL;//deal with *p,or it is NULL
     struct triargexpr_list *temp_expr;
     struct triargexpr *last_func_expr = NULL;
     /*
       分析步骤：
       1、对于赋值。arg1=arg2
           a)首先，如果arg1是标号(k)而且k没有map_id，需要向上看一步，如果k是*p或者a[i]，要先分析语句k；
           b)分析本条赋值语句。
       2、对于条件跳转。
           a)首先，如果arg1是标号(k)而且k没有map_id，这意味着这个位置是一个逻辑条件指令，需要向上看一步，分析该三元式；
           b)分析本条条件跳转指令。
       3、对于a+b、a-b、*p、a[i]、+a、-a、&a以及所有逻辑指令;
           a)如果expr->entity->index没有map_id，说明这一句话从来没被引用过，没必要生成代码，也就没必要分析；
           b)分析本条语句。
       4、其他语句，根据要分析的操作数个数和位置，分以下几种情况：
           a)a++、a--
           b)Return、Arglist
     */
/*
  思路：
  del_list——当前三元式要删除的变量的map_id
  add_list——当前三元式要增加的变量的map_id
  next_del_list——下一个三元式要删除变量的map_id，每处理一个三元式，都将它复制给del_list，然后再得出next_del_list
  1）获得得到add_list。
     每当遇到三元式中的一个argi，只要它不是被赋值的元素，则交由get_index_of_arg处理，其中第二个参数为NULL，返回值放到变量addi中。
         a）argi如果是立即数，get_index_of_arg返回-1；
         b）argi如果是ident，get_index_of_arg返回该ident的map_id；
         c）如果是三元式编号，get_index_of_temp返回该编号的map_id。
     如果只有一个操作数，则add2=-1。如此之后，便将add1和add2通过make_list制作成了添加链。如果该三元式是deref，还需要将其对应的pointer_entity中的所有变量置成活跃的。
  2）获得得到next_del_list。
     每当遇到三元式中的一个argi，如果是被赋值的元素，则交由get_index_of_arg处理，其中第二个参数为&point_list，返回值放到变量deli中。
         a）argi如果是立即数，get_index_of_arg返回-1；
         b）argi如果是ident，get_index_of_arg返回该ident的map_id；
         c）如果是三元式编号，get_index_of_temp返回该编号的map_id。
  然后再从上一条的活跃变量中删除del_list，添加add_list就可以了
*/
     for(i = 0 ; i < g_block_num ; i++)
     {
          temp_expr = DFS_array[i]->tail;
          del1 = del2 = -1;
          flag_first = 1;
          int is_continue;//本条语句没有进行活跃变量分析，则置为1
          var_list_clear(add_list);
          var_list_clear(next_del_list);
          var_list_clear(del_list);
          if(option_show_active_var == 1)
               printf("\nblock (%d)\n" , i);
          while(temp_expr != DFS_array[i]->head->prev)
          {
               is_continue = 0;
               temp_expr->entity->actvar_list = NULL;
               switch(temp_expr->entity->op)
               {
               case Assign:       //arg1 = arg2
                    add2 =  get_index_of_arg(&(temp_expr->entity->arg2) , NULL);
                    add1 = -1;
                    make_change_list(add1 , add2 , add_list);
                    if(temp_expr->entity->arg1.type == ExprArg && get_index_of_temp(temp_expr->entity->arg1.expr) == -1)
                    {//arg1是三元式编号，而且没被引用过。说明其对应三元式可能是a[i]或者*p
                         int last_index = temp_expr->entity->arg1.expr;
                         struct triargexpr_list *last_expr_node = cur_func_triarg_table->index_to_list[last_index];
                         struct triargexpr *last_expr = last_expr_node->entity;
                         if(last_expr->op == Subscript)//是a[i]或者*p的话，要往上看一步
                         {
                              add1 = get_index_of_arg(&(last_expr->arg1) , NULL);
                              add2 = get_index_of_arg(&(last_expr->arg2) , NULL);
                              struct var_list temp_list;
                              temp_list.head = temp_list.tail = NULL;
                              make_change_list(add1 , add2 , &temp_list);
                              add_list = var_list_merge(&temp_list , add_list);
                              var_list_del_repeate(add_list);
                         }
                         else if(last_expr->op == Deref)
                         {
                              add1 = -1;
                              add2 = get_index_of_arg(&(last_expr->arg1) , NULL);
                              struct var_list temp_list;
                              temp_list.head = temp_list.tail = NULL;
                              make_change_list(add1 , add2 , &temp_list);
                              add_list = var_list_merge(&temp_list , add_list);
                              var_list_del_repeate(add_list);
                         }
                    }
                    var_list_copy(next_del_list , del_list);
                    
                    del1 = get_index_of_arg(&(temp_expr->entity->arg1) , NULL);
                    del2 = get_index_of_temp(temp_expr->entity->index);
                    make_change_list(del1 , del2 , next_del_list);
                    //next_del_list = var_list_merge(point_list , next_del_list);
                    point_list = NULL;
                    break;
                    
                    /*binary op*/
               case Land:        //&&
               case Lor:         //||
               case Eq:          //==
               case Neq:         //!=
               case Ge:          //>=
               case Le:          //<=
               case Nge:         //<
               case Nle:         //>
               case Plus:        //+
               case Minus:       //-
               case Mul:         //*
               case Subscript:   //[]
                    if(get_index_of_temp(temp_expr->entity->index) == -1)
                    {
                         is_continue = 1;
                         break;
                    }
                    add1 = get_index_of_arg(&(temp_expr->entity->arg1) , NULL);
                    add2 = get_index_of_arg(&(temp_expr->entity->arg2) , NULL);
                    make_change_list(add1 , add2 , add_list);
                    var_list_copy(next_del_list , del_list);
                    
                    del1 = -1;
                    del2 = get_index_of_temp(temp_expr->entity->index);
                    make_change_list(del1 , del2 , next_del_list);
                    break;

                    /*Unary op or jump*/
               case FalseJump:   //if true jump
               case TrueJump:    //if false jump
                    add1 = -1;
                    add2 = get_index_of_arg(&(temp_expr->entity->arg1) , NULL);
                    make_change_list(add1 , add2 , add_list);
                    if(temp_expr->entity->arg1.type == ExprArg && get_index_of_temp(temp_expr->entity->arg1.expr) == -1)
                    {//条件是逻辑判断，需要向上看一步
                         int last_index = temp_expr->entity->arg1.expr;
                         struct triargexpr_list *last_expr_node = cur_func_triarg_table->index_to_list[last_index];
                         struct triargexpr *last_expr = last_expr_node->entity;
                         add1 = get_index_of_arg(&(last_expr->arg1) , NULL);
                         add2 = get_index_of_arg(&(last_expr->arg2) , NULL);
                         struct var_list temp_list;
                         temp_list.head = temp_list.tail = NULL;
                         make_change_list(add1 , add2 , &temp_list);
                         add_list = var_list_merge(&temp_list , add_list);
                         var_list_del_repeate(add_list);
                    }
                    var_list_copy(next_del_list , del_list);
                    
                    var_list_clear(next_del_list);
                    break;
                    
               case Lnot:        //!
               case Uplus:       //+
               case Uminus:      //-
               case Ref:         //&
               case Deref:       //*
                    if(get_index_of_temp(temp_expr->entity->index) == -1)
                    {
                         is_continue = 1;
                         break;
                    }
               case Plusplus:    //++
               case Minusminus:  //--
               case Arglist:     //parameters
               case Return:      //return
                    add1 = -1;
                    add2 = get_index_of_arg(&(temp_expr->entity->arg1) , NULL);
                    make_change_list(add1 , add2 , add_list);
                    if(temp_expr->entity->op == Deref)//引用的*p，需要把它所有可能对应的实体都置成活跃
                    {
                         point_list = temp_expr->pointer_entity;
                         add_list = var_list_merge(point_list , add_list);
                    }
                    var_list_copy(next_del_list , del_list);
                    
                    del1 = -1;
                    del2 = get_index_of_temp(temp_expr->entity->index);
                    make_change_list(del1 , del2 , next_del_list);

                    if(temp_expr->entity->op == Arglist)//函数所有参数的pointer_entity合成一个链，用于caller保存指针实体
                    {
                         var_list_merge(temp_expr->pointer_entity , &flush_list);
                    }
                    
                    break;

               case Funcall://将callee所有参数对应的pointer_entity以及def_g_list合成flush_list，放在该三元式的arg2里面
                    var_list_merge(&def_g_list , &flush_list);
                    if(last_func_expr != NULL)
                    {
                         last_func_expr->arg2.func_flush_list = var_list_new();
                         last_func_expr->arg2.func_flush_list->head = flush_list.head;
                         last_func_expr->arg2.func_flush_list->tail = flush_list.tail;
                         flush_list.head = flush_list.tail = NULL;
                    }
                    last_func_expr = temp_expr->entity;
                    
                    /*Funcall的addlist就是def_g_list*/
                    add_list = var_list_copy(&def_g_list , add_list);
                    var_list_copy(next_del_list , del_list);
                    var_list_clear(next_del_list);
                    break;
                    
                    /*大立即数*/
               default:
                    add1 = add2 = -1;
                    make_change_list(add1 , add2 , add_list);
                    var_list_copy(next_del_list , del_list);
                    
                    del1 = -1;
                    del2 = get_index_of_temp(temp_expr->entity->index);
                    make_change_list(del1 , del2 , next_del_list);
                    break;
               }
               if(is_continue == 0)
               {
                    if(flag_first == 1)
                    {
                         var_list_sub(var_out + i , del_list , actvar_list + act_list_index);
                         var_list_merge(add_list , actvar_list + act_list_index);
                         act_list_index++;
                         flag_first = 0;
                    }
                    else
                    {
                         var_list_sub(actvar_list + act_list_index - 1 , del_list , actvar_list + act_list_index);
                         var_list_merge(add_list , actvar_list + act_list_index);
                         act_list_index++;
                    }
                    //         after_make_actvarlist:
                    count = add_actvar_info(actvar_list + act_list_index -1);
                    temp_expr->entity->actvar_list = (actvar_list + act_list_index -1);
                    if(temp_expr->entity->op == Funcall)
                         if(count > sg_max_func_varlist)
                              sg_max_func_varlist = count;
               }
               
               if(option_show_flow_debug == 1)
               {
                    printf("del:");
                    var_list_print(del_list);
                    printf("add:");
                    var_list_print(add_list);
               }
               if(option_show_active_var == 1)
               {
                    print_triargexpr(*(temp_expr->entity));
                    printf("\t[%d]active varible:" , temp_expr->entity->index);
                    var_list_print(actvar_list + act_list_index -1);
               }
               temp_expr = temp_expr->prev;
          }
     }
     if(last_func_expr != NULL)
     {
          var_list_merge(&def_g_list , &flush_list);
          last_func_expr->arg2.func_flush_list = var_list_new();
          last_func_expr->arg2.func_flush_list->head = flush_list.head;
          last_func_expr->arg2.func_flush_list->tail = flush_list.tail;
          flush_list.head = flush_list.tail = NULL;
     }
     return actvar_list;
}

void free_all(struct var_list *all_var_lists)//free all memory
{
     int i;
     for(i = 0 ; i < g_block_num ; i++)//
          var_list_free_bynode(var_out[i].head);
     for(i = 0 ; i < s_expr_num ; i++)
          var_list_free_bynode(all_var_lists[i].head);
     free(all_var_lists);
     free(var_out);
     free(is_actvar);
}
