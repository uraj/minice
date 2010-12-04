#include "minic_flowanalyse.h"
#include "minic_varmapping.h"
#include "minic_triargexpr.h"
#include "minic_symtable.h"
#include <stdio.h>
#include <stdlib.h>

#define SHOWACTVAR 1

static struct var_list *var_in;
struct var_list *var_out;
static struct var_list *def;
static struct var_list *use;
static int *def_size;
static int *use_size;

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
          return;
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
     struct var_list *new_list = (struct var_list *)malloc(sizeof(struct var_list *));
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

void var_list_del_repeate(struct var_list *list)//将排好序的链去重
{
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

inline void var_list_append(struct var_list *list , int num)
{
//     printf("*append%d* " , num);//*********************************
     if(list == NULL)
          list = var_list_new();
	if(list->head == NULL && list->tail != NULL)
	{
		list->head = list->tail;
		list->tail->var_map_index = num;
		return;
	}
	if(list->head != NULL && list->tail->next != NULL)//如果list的tail后面还有节点，则直接利用，避免浪费
	{
		list->tail = list->tail->next;
		list->tail->var_map_index = num;
		return;
	}
	struct var_list_node *cur_node = var_list_node_new(num);
	if(list->head == NULL)
	{
		list->head = list->tail = cur_node;
		return;
	}
	list->tail->next = cur_node;
	list->tail = cur_node;
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
          printf("Delete error!The list is EMPTY!\n");
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
              printf("Delete node error!Can't find the former node.\n");
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
              printf("Delete node error!Can't find the former node.\n");
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

void var_list_copy(struct var_list *source , struct var_list *dest)
{
     if(source == NULL || dest == NULL)
          return;
     var_list_clear(dest);
     if(source->head == NULL)
          return;
     struct var_list_node *temp = source->head;
     while(temp != source->tail)
     {
          var_list_append(dest , temp->var_map_index);
          temp = temp->next;
     }
     var_list_append(dest , temp->var_map_index);
     return;
}

void var_list_merge(struct var_list *adder , struct var_list *dest)//将变量链adder和dest合并，并写入dest内
{
     if(adder == NULL || dest == NULL)
          return;
     if(adder->head == NULL)
          return;
     if(dest->head == NULL)
     {
          var_list_copy(adder , dest);
          return;
     }
     
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
}

void var_list_sub(struct var_list *sub , struct var_list *subed , struct var_list *dest)//dest = sub - subed
{
     if(sub == NULL || subed == NULL)
          return;
	var_list_clear(dest);
	if(sub->head == NULL)
		return;
	if(subed->head == NULL)
	{
		var_list_copy(sub , dest);
		return;
	}
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
}

void var_list_inter(struct var_list *inter , struct var_list *dest)//dest = inter & dest
{
     if(inter == NULL || dest == NULL)
          return;
	if(inter->head == NULL)
	{
		var_list_clear(dest);
		return;
	}
	if(dest->head == NULL)
		return;
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
}

//void var_list_change(int change_array[4] , struct var_list *list)//给定增加和减少的变量，

static inline void analyse_map_index(int i , int type , int block_index)
{
     if(type == DEFINE)//**************************
          printf("DEFINE ");
     else
          printf("USE ");//**********************
     struct var_info *temp;
     temp = get_info_from_index(i);
     if(temp == NULL)
     {
          printf("Can't get var_info of map_id %d\n" , i);//*********************8
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
     printf("\n");
}

static inline void analyse_arg(struct triarg *arg , int type , int block_index)//活跃变量分析中的内存分配函数
{
     int i;
     if(arg->type == IdArg)
     {
          i = get_index_of_id(arg->idname);
          printf("      mapid:%d->idname:%s " , i , arg->idname);//*****************************
     }
     else if(arg->type == ExprArg)
     {
          i = get_index_of_temp(arg->expr);
          printf("      mapid:%d->expr:%d " , i , arg->expr);//*************************
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

static void initial_active_var()//活跃变量分析的初始化部分def和use
{
     int i;
     printf("id_num:%d\n" , g_var_id_num);
     struct triargexpr_list *temp;
     for(i = 0 ; i < g_block_num ; i++)
     {
          printf("block:%d\n" , i);
          if(i == 0)//第一个块要把函数参数放入def[0]当中
          {
               int start = curr_table->arg_no_min;
               int end = curr_table->arg_no_max;
               int j;
               for(j = start ; j < end ; j++)
                    var_list_append(def , j);
          }
          temp = DFS_array[i]->head;
          while(temp != NULL)
          {
               /**/
               print_triargexpr(*(temp->entity));
               int k = get_index_of_temp(temp->entity->index);
               printf("      mapid:%d->expr:%d\n" , k , temp->entity->index);
               /**/
               switch(temp->entity->op)//没考虑指针和函数调用时的参数部分
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
               
               temp = temp->next;
          }
          var_list_sort(def + i , def_size[i]);//when DEFs and USEs are made
          var_list_sort(use + i , use_size[i]);//sort them so that we can op
          var_list_del_repeate(def + i);
          var_list_del_repeate(use + i);
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

static inline int get_index_of_arg(struct triarg *arg)
{
     if(arg->type == IdArg)
          return (get_index_of_id(arg->idname));
     else if(arg->type == ExprArg)
          return (get_index_of_temp(arg->expr));
     else
          return -1;
}

static inline void push_changes_into_expr(struct triargexpr_list *expr , int del1 , int del2 , int add1 , int add2)
{
     int temp;
     if(del1 > del2)
     {
          temp = del1;
          del1 = del2;
          del2 = temp;
     }
     if(add1 > add2)
     {
          temp = add1;
          add1 = add2;
          add2 = temp;
     }
     expr->actvar_change[0] = del1;
     expr->actvar_change[1] = del2;
     expr->actvar_change[2] = add1;
     expr->actvar_change[3] = add2;
#ifdef SHOWACTVAR
     int i;
     printf("    (%d):" , expr->entity->index);
     for(i = 0 ; i < 4 ; i++)
     {
          if(i < 2)
               printf("del%d " , expr->actvar_change[i]);
          else
               printf("add%d " , expr->actvar_change[i]);
     }
     printf("\n");
#endif
}

void analyse_actvar()//活跃变量分析
{
     int i;
     malloc_active_var();//活跃变量分析相关的数组空间分配
     initial_active_var();//完成活跃变量分析的初始化部分，生成def和use
     solve_equa_ud();//求解活跃变量方程组，得到var_in和var_out
     int add1 , add2 , del1 , del2;
#ifdef SHOWACTVAR
     struct var_list show_list;
     show_list.head = show_list.tail = NULL;
#endif
     for(i = 0 ; i < g_block_num ; i++)
     {
          struct triargexpr_list *temp_expr = DFS_array[i]->tail;
          del1 = del2 = -1;
#ifdef SHOWACTVAR
          var_list_copy(var_out + i , &show_list);
          printf("\nblock (%d)\n" , i);
#endif
          while(temp_expr != NULL)
          {
               switch(temp_expr->entity->op)
               {
               case Assign:
                    add2 = get_index_of_arg(&(temp_expr->entity->arg2));
                    add1 = -1;
                    push_changes_into_expr(temp_expr , del1 , del2 , add1 , add2);
                    del1 = get_index_of_arg(&(temp_expr->entity->arg1));
                    del2 = get_index_of_temp(temp_expr->entity->index);
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
                    add1 = get_index_of_arg(&(temp_expr->entity->arg1));
                    add2 = get_index_of_arg(&(temp_expr->entity->arg2));
                    push_changes_into_expr(temp_expr , del1 , del2 , add1 , add2);
                    del1 = -1;
                    del2 = get_index_of_temp(temp_expr->entity->index);
                    break;
               case Lnot:
               case Uplus:
               case Plusplus:
               case Minusminus:
               case Ref:
               case Deref:
               case Arglist:
               case Return:
               case UncondJump:
               case FalseJump:
               case TrueJump:
                    add1 = -1;
                    add2 = get_index_of_arg(&(temp_expr->entity->arg1));
                    push_changes_into_expr(temp_expr , del1 , del2 , add1 , add2);
                    del1 = -1;
                    del2 = get_index_of_temp(temp_expr->entity->index);
                    break;
               default:
                    break;
               }
#ifdef SHOWACTVAR
               struct var_list_node *former , *cur;
               int d , a;
               cur = show_list.head;
               former = NULL;
               for(d = 0 ; d < 2 ; d++)
                    for(a = 2 ; a < 4 ; a++)
                         if(temp_expr->actvar_change[a] == temp_expr->actvar_change[d])//如果一个变量既要添加又要删除，就不删除了
                              temp_expr->actvar_change[d] = -1;
               if(temp_expr->actvar_change[1] == -1)
               {
                    temp_expr->actvar_change[1] = temp_expr->actvar_change[0];
                    temp_expr->actvar_change[0] = -1;
               }
               a = 2;
               d = 0;
               while(temp_expr->actvar_change[d] == -1 && d < 2)//跳过所有-1
                    d++;
               while(temp_expr->actvar_change[a] == -1 && a < 4)
                    a++;
               printf("(%d) active var: " , temp_expr->entity->index);
               if(cur == NULL)//链表为空
               {
                    printf("//");
                    for(; a < 4 ; a++)
                         var_list_append(&show_list , temp_expr->actvar_change[a]);
                    if((show_list.head) == NULL)
                         printf("NO ACTIVE VAR!\n");
                    else
                         var_list_print(&show_list);
               }
               else
               {
                    while(cur != show_list.tail->next)
                    {
                         if(d < 2)
                         {
                              if((temp_expr->actvar_change[d]) == (cur->var_map_index))
                              {
                                   cur = var_list_delete(&show_list , former , cur);
                                   d++;
                                   if(show_list.head == NULL)
                                        break;
                                   continue;
                              }
                         }
                         if(a < 4)
                         {
                              if((temp_expr->actvar_change[a]) == (cur->var_map_index))
                              {
                                   a++;
                                   continue;
                              }
                              else if((temp_expr->actvar_change[a]) < (cur->var_map_index))
                              {
                                   former = var_list_insert(&show_list , former , temp_expr->actvar_change[a]);
                                   printf("%d " , former->var_map_index);
                                   a++;
                                   continue;
                              }
                         }
                         printf("%d " , cur->var_map_index);
                         former = cur;
                         cur = cur->next;
                    }
                    for(; a < 4 ; a++)
                    {
                         printf("%d " , temp_expr->actvar_change[a]);
                         var_list_append(&show_list , temp_expr->actvar_change[a]);
                    }
                    printf("\n");
//                    var_list_print(&show_list);
               }
#endif
               temp_expr = temp_expr->prev;
          }
     }
}

