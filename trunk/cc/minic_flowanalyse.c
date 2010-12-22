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
static int s_expr_num;//the num of the tri-expressions
static const int gc_change_num = 4;

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

//void var_list_change(int change_array[4] , struct var_list *list)//给定增加和减少的变量，

static inline void analyse_map_index(int i , int type , int block_index)
{
#ifdef SHOW_FLOW_DEBUG
     if(type == DEFINE)//**************************
          printf("DEFINE ");
     else
          printf("USE ");//**********************
#endif
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
#ifdef SHOW_FLOW_DEBUG 
     printf("\n");
#endif
}

static inline void analyse_arg(struct triarg *arg , int type , int block_index)//活跃变量分析中的内存分配函数
{
     int i;
     if(arg->type == IdArg)
     {
          i = get_index_of_id(arg->idname);
#ifdef SHOW_FLOW_DEBUG
          printf("      mapid:%d->idname:%s " , i , arg->idname);//*****************************
#endif
     }
     else if(arg->type == ExprArg)
     {
          if(arg->expr == -1)
               return;
          i = get_index_of_temp(arg->expr);
          struct triargexpr_list *temp_expr_node = cur_func_triarg_table->index_to_list[arg->expr];
          struct triargexpr *temp_expr = temp_expr_node->entity;
          int change = 0;
          if(temp_expr->op == Deref)//*p
          {
               struct var_list *temp_point_list = temp_expr_node->pointer_entity;
//               printf("//************************(%d)" , temp_expr->index);
//               var_list_print(temp_point_list);
               if(temp_point_list != NULL
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
          }
#ifdef SHOW_FLOW_DEBUG
          if(i != -1)//***************************
          {
               if(change == 0)
                    printf("      mapid:%d->expr:%d " , i , arg->expr);//*************************
               else
                    printf("      mapid:%d->idname:%s " , i , arg->idname);//*****************************
          }
#endif
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
#ifdef SHOW_FLOW_DEBUG
	 printf("      mapid:%d->expr:%d " , i , expr_index);
#endif
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
     int total_id_num = simb_table->id_num + cur_func_sym_table->id_num;
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
     }
}

static void initial_active_var()//活跃变量分析的初始化部分def和use
{
     int i;
#ifdef SHOW_FLOW_DEBUG
     printf("id_num:%d\n" , g_var_id_num);
#endif
     struct triargexpr_list *temp;
     s_expr_num = 0;
     for(i = 0 ; i < g_block_num ; i++)
     {
#ifdef SHOW_FLOW_DEBUG
          printf("block:%d\n" , i);
#endif
          if(i == 0)//第一个块要把函数参数和全局变量放入def[0]当中
          {
               //int start = cur_func_sym_table->arg_no_min;
               //int end = cur_func_sym_table->arg_no_max;
               int global_var_num = get_globalvar_num();
               int j;
               for(j = 0 ; j < global_var_num ; j++)
               {
                    def_size[0] ++;
                    var_list_append(def , j);
               }
               //for(j = start ; j < end ; j++)
               //{
               //     def_size[0] ++;
               //     var_list_append(def , j);
               //}
          }
          temp = DFS_array[i]->head;
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
               case Assign:
                    analyse_expr_index(temp->entity->index , DEFINE , i);
                    analyse_arg(&(temp->entity->arg1) , DEFINE , i);
                    analyse_arg(&(temp->entity->arg2) , USE , i);
                    break;
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
               case Mul://二元操作
                    analyse_expr_index(temp->entity->index , DEFINE , i);
                    analyse_arg(&(temp->entity->arg1) , USE , i);
                    analyse_arg(&(temp->entity->arg2) , USE , i);
                    break;
               case Lnot:
               case Uplus:
               case Plusplus:
               case Minusminus:
               case Ref:
               case Deref:
               case Arglist:
               case Return:
                    analyse_expr_index(temp->entity->index , DEFINE , i);
                    analyse_arg(&(temp->entity->arg1) , USE , i);
                    break;
               case TrueJump:
               case FalseJump:
                    analyse_arg(&(temp->entity->arg1) , USE , i);
                    break;
               default:
                    analyse_expr_index(temp->entity->index , DEFINE , i);
                    break;
               }
#ifdef SHOW_FLOW_DEBUG
               print_triargexpr(*(temp->entity));printf("\n");//**************************************
#endif
               temp = temp->next;
          }
          var_list_sort(def + i , def_size[i]);//when DEFs and USEs are made
          var_list_sort(use + i , use_size[i]);//sort them so that we can op
          var_list_del_repeate(def + i);
          var_list_del_repeate(use + i);
#ifdef SHOW_FLOW_DEBUG
          printf("\n");
#endif
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
     for(i = 0 ; i < g_block_num ; i++)//解方程组后，def，use和var_in都没用了
     {
#ifdef SHOWACTVAR
          printf("DEF %d :" , i);
          var_list_print(def + i);
          printf("USE %d :" , i);
          var_list_print(use + i);
          printf("VIN %d :" , i);
          var_list_print(var_in + i);
          printf("OUT %d :" , i);
          var_list_print(var_out + i);
          printf("\n");
#endif
          var_list_free_bynode(def[i].head);
          var_list_free_bynode(use[i].head);
          var_list_free_bynode(var_in[i].head);
     }
     free(def);
     free(use);
     free(var_in);
}

static inline int get_index_of_arg(struct triarg *arg , struct var_list **dest)//get the map_index of arg,but if arg is a *p,we must do something about the pointer_entity list.Return -1 if we can't find the map_id or we find more than one idents in the pointer_entity list but dest is NULL.If pointer_entity list has only one ident,replace arg with it and return its map_id.If pointer_entity list has more than one idents and dest isn't NULL,it'll return the map_id of arg and put the pointer_entity list in dest.
{
     if(arg->type == IdArg)
          return (get_index_of_id(arg->idname));
     else if(arg->type == ExprArg)
     {
          if(arg->expr == -1)
               return -1;
          struct triargexpr_list *temp_expr_node = cur_func_triarg_table->index_to_list[arg->expr];
          struct triargexpr *temp_expr = temp_expr_node->entity;
          if(temp_expr->op == Deref)//*p
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
                              temp_expr_node->pointer_entity = temp_var_info->ref_point;//实体链指向数组的定点链
                              //     (*dest) = temp_var_info->ref_point;
                              return get_index_of_temp(arg->expr);
                         }
                         arg->type = IdArg;
                         arg->idname=(get_valueinfo_byno(cur_func_sym_table,temp_point_list->head->var_map_index))->name;
                         return temp_point_list->head->var_map_index;
                    }
                    return get_index_of_temp(arg->expr);
               }
               if(temp_point_list == NULL)//预示该代码可能会段错误，要把下一条的活跃变量全部清空，这里做不了，返回-2
                    return -2;
               if(temp_point_list->head == NULL)//预示该代码可能会段错误，要把下一条的活跃变量全部清空，这里做不了，返回-2
                    return -2;
               if(temp_point_list->head == temp_point_list->tail)//只有一个元素
               {
                    struct var_info *temp_var_info = get_info_from_index(temp_point_list->head->var_map_index);
                    if(temp_var_info->ref_point != NULL)//此指针为数组
                    {
                         var_list_free(temp_point_list);
                         temp_expr_node->pointer_entity = temp_var_info->ref_point;//实体链指向数组的定点链
                         (*dest) = temp_var_info->ref_point;//该数组中所有元素的三元式编号的map_id都需要从活跃变量中删除
                         return get_index_of_temp(arg->expr);
                    }
                    arg->type = IdArg;
                    arg->idname = (get_valueinfo_byno(cur_func_sym_table , temp_point_list->head->var_map_index))->name;
                    return temp_point_list->head->var_map_index;
               }
               (*dest) = temp_point_list;
               return (get_index_of_temp(arg->expr));
          }
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
     if(num2 >= 0)
          var_list_append(dest , num2);
}

struct var_list *analyse_actvar(int *expr_num , int func_index)//活跃变量分析
{
     initial_func_var(func_index);//通过函数index获得当前函数信息，以便后续使用
     malloc_active_var();//活跃变量分析相关的数组空间分配
     initial_active_var();//完成活跃变量分析的初始化部分，生成def和use
     solve_equa_ud();//求解活跃变量方程组，得到var_in和var_out
     
     (*expr_num) = s_expr_num;//the num of expressions

     int i , add1 , add2 , del1 , del2 , flag_first;
     int act_list_index = 0;
     struct var_list *actvar_list;//
     actvar_list = (struct var_list *)malloc(sizeof(struct var_list) * s_expr_num);
     for(i = 0 ; i < s_expr_num ; i++)
          actvar_list[i].head = actvar_list[i].tail = NULL;
     
     struct var_list show_list;//temp list of active varible
     show_list.head = show_list.tail = NULL;
     struct var_list *del_list = var_list_new();
     struct var_list *next_del_list = var_list_new();
     struct var_list *add_list = var_list_new();
     struct var_list *point_list = NULL;//deal with *p,or it is NULL
     struct triargexpr_list *temp_expr;
/*
  思路：
  del_list——当前三元式要删除的变量的map_id
  add_list——当前三元式要增加的变量的map_id
  next_del_list——下一个三元式要删除变量的map_id，每处理一个三元式，都将它复制给del_list，然后再得出next_del_list
  1）获得得到add_list。
     每当遇到三元式中的一个argi，只要它不是被赋值的元素，则交由get_index_of_arg处理，其中第二个参数为NULL，返回值放到变量addi中。
         a）argi如果是立即数，get_index_of_arg返回-1；
         b）argi如果是ident，get_index_of_arg返回该ident的map_id；
         c）如果是三元式编号，处理会复杂一些：
             1）如果编号对应三元式的操作类型不是Deref，直接返回该三元式编号的map_id；
             2）如果编号对应三元式操作类型为Deref，即解除引用，那么如果：
                 a）pointer_entity中只有1个元素。这种情况下，直接将argi替换成该元素，addi就是该元素的map_id；
                 b）pointer_entity中有好几个元素。addi还是原来的编号的map_id。
     如果只有一个操作数，则add2=-1。如此之后，便将add1和add2通过make_list制作成了添加链。add_list获得过程不会对point_list进行任何操作。
  2）获得得到next_del_list。
     每当遇到三元式中的一个argi，如果是被赋值的元素，则交由get_index_of_arg处理，其中第二个参数为&point_list，返回值放到变量deli中。
         a）argi如果是立即数，get_index_of_arg返回-1；
         b）argi如果是ident，get_index_of_arg返回该ident的map_id；
         c）如果是三元式编号，处理会复杂一些：
             1）如果编号对应三元式的操作类型不是Deref，直接返回该三元式编号的map_id；
             2）如果编号对应三元式操作类型为Deref，即解除引用，那么如果：
                 a）pointer_entity中只有1个元素。这种情况下，直接将argi替换成该元素，deli就是该元素的map_id；
                 b）pointer_entity中有好几个元素。此时将(*dest)也就是point_list指向pointer_entity，deli为原来编号的map_id。
  然后再从上一条的活跃变量中删除del_list，添加add_list就可以了
*/
     for(i = 0 ; i < g_block_num ; i++)
     {
          temp_expr = DFS_array[i]->tail;
          del1 = del2 = -1;
          flag_first = 1;
          var_list_copy(var_out + i , &show_list);
#ifdef SHOWACTVAR
          printf("\nblock (%d)\n" , i);
#endif
          while(temp_expr != DFS_array[i]->head->prev)
          {
               switch(temp_expr->entity->op)
               {
               case Assign:       //=
                    add2 =  get_index_of_arg(&(temp_expr->entity->arg2) , NULL);
                    add1 = -1;
                    make_change_list(add1 , add2 , add_list);
                    if(del1 == -2 || del2 == -2)//上一句中对指针实体赋值，而且该指针指向不明确，当前活跃变量链就是add_list
                    {
                         var_list_copy(add_list , actvar_list + act_list_index);
                         act_list_index++;
                         goto after_make_actvarlist;
                    }
                    var_list_copy(next_del_list , del_list);
                    
                    del1 = get_index_of_arg(&(temp_expr->entity->arg1) , &point_list);
                    del2 = get_index_of_temp(temp_expr->entity->index);
                    make_change_list(del1 , del2 , next_del_list);
                    next_del_list = var_list_merge(point_list , next_del_list);
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
                    add1 = get_index_of_arg(&(temp_expr->entity->arg1) , NULL);
                    add2 = get_index_of_arg(&(temp_expr->entity->arg2) , NULL);
                    make_change_list(add1 , add2 , add_list);
                    if(del1 == -2 || del2 == -2)//上一句中对指针实体赋值，而且该指针指向不明确，当前活跃变量链就是add_list
                    {
                         var_list_copy(add_list , actvar_list + act_list_index);
                         act_list_index++;
                         goto after_make_actvarlist;
                    }
                    var_list_copy(next_del_list , del_list);
                    
                    del1 = -1;
                    del2 = get_index_of_temp(temp_expr->entity->index);
                    make_change_list(del1 , del2 , next_del_list);
                    break;

                    /*Unary op or jump*/
               case Lnot:        //!
               case Uplus:       //+
               case Plusplus:    //++
               case Minusminus:  //--
               case Ref:         //&
               case Deref:       //*
               case Arglist:     //parameters
               case Return:      //return
               case FalseJump:   //if true jump
               case TrueJump:    //if false jump
                    add1 = -1;
                    add2 = get_index_of_arg(&(temp_expr->entity->arg1) , NULL);
                    make_change_list(add1 , add2 , add_list);
                    if(del1 == -2 || del2 == -2)//上一句中对指针实体赋值，而且该指针指向不明确，当前活跃变量链就是add_list
                    {
                         var_list_copy(add_list , actvar_list + act_list_index);
                         act_list_index++;
                         goto after_make_actvarlist;
                    }
                    var_list_copy(next_del_list , del_list);
                    
                    del1 = -1;
                    del2 = get_index_of_temp(temp_expr->entity->index);
                    make_change_list(del1 , del2 , next_del_list);
                    break;
               default:
                    add1 = add2 = -1;
                    make_change_list(add1 , add2 , add_list);
                    if(del1 == -2 || del2 == -2)//上一句中对指针实体赋值，而且该指针指向不明确，当前活跃变量链就是add_list
                    {
                         var_list_copy(add_list , actvar_list + act_list_index);
                         act_list_index++;
                         goto after_make_actvarlist;
                    }
                    var_list_copy(next_del_list , del_list);
                    
                    del1 = -1;
                    del2 = get_index_of_temp(temp_expr->entity->index);
                    make_change_list(del1 , del2 , next_del_list);
                    break;
               }
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
          after_make_actvarlist:
#ifdef SHOW_FLOW_DEBUG
               printf("del:");
               var_list_print(del_list);
               printf("add:");
               var_list_print(add_list);
#endif
#ifdef SHOWACTVAR
               print_triargexpr(*(temp_expr->entity));
               printf("(%d) active varible:" , temp_expr->entity->index);
               var_list_print(actvar_list + act_list_index -1);
#endif
               temp_expr = temp_expr->prev;
          }
     }
     free_all();
     return actvar_list;
}

void free_all()//free all memory
{
     int i;
     for(i = 0 ; i < g_block_num ; i++)//
          var_list_free_bynode(var_out[i].head);
     free(var_out);
}
