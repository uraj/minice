#include "minic_flowanalyse.h"
#include "minic_varmapping.h"
#include "minic_triargexpr.h"
#include <stdlib.h>

/*extern basic_block **block_array;

typedef struct adjlist *point_list;//记录定值点的链表节点
extern point_list *gen;
extern point_list *kill;
extern point_list *in;
extern point_list *out;

typedef struct adjlist *expr_list;//记录三元式编号的链表节点
extern expr_list *expr_gen;
extern expr_list *expr_kill;
extern expr_list *expr_in;
extern expr_list *expr_out;


extern var_list *var_in;
extern var_list *var_out;
extern var_list *def;
extern var_list *use;*/

void var_list_printbynode(struct var_list_node *head)
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

void var_list_print(struct var_list *list)
{
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

int compare (const void * a, const void * b) 
{
	return ( *(int*)a - *(int*)b );
}

void var_list_sort(struct var_list *list_array , int size)//将一个定值点链表或者变量编号链表排序
{
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

void var_list_free_bynode(struct var_list_node *head)
{
	if(head == NULL)
		return;
	var_list_free_bynode(head->next);
	free(head);
}

void var_list_free(struct var_list *list)
{
	if(list->head == NULL)
		list->head = list->tail;
	var_list_free_bynode(list->head);
}

void var_list_clear(struct var_list *list)
{
	if(list->head == NULL)
		return;
	list->tail = list->head;
	list->head = NULL;
}

void var_list_append(struct var_list *list , int num)
{
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
	struct var_list_node *cur_node = (struct var_list_node *)malloc(sizeof(struct var_list_node));
	cur_node->var_map_index = num;
	cur_node->next = NULL;
	if(list->head == NULL)
	{
		list->head = list->tail = cur_node;
		return;
	}
	list->tail->next = cur_node;
	list->tail = cur_node;
}

void var_list_insert(struct var_list_node *cur , int num)
{
	struct var_list_node *in_node = (struct var_list_node *)malloc(sizeof(struct var_list_node));
	in_node->var_map_index = num;
	in_node->next = NULL;
	if(cur == NULL)
	{
		cur = in_node;
		return;
	}
	struct var_list_node *temp = cur->next;
	cur->next = in_node;
	in_node->next = temp;
}

void var_list_delete(struct var_list_node *former , struct var_list_node *del_node)//删除节点，删除后，被删除节点指针不可用
{
	if(del_node == NULL || former == NULL)
		return;
	former->next = del_node->next;
	free(del_node);
}

int var_list_isequal(struct var_list *l1 , struct var_list *l2)
{
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
     if(source == NULL)
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
     if(adder->head == NULL)
          return;
     if(dest->head == NULL)
     {
          var_list_copy(adder , dest);
          return;
     }
     
     struct var_list_node *adder_pointer = adder->head;
     struct var_list_node *dest_pointer = dest->head;
     struct var_list_node *former_dest_p = dest_pointer;
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
               var_list_insert(former_dest_p , adder_pointer->var_map_index);
               adder_pointer = adder_pointer->next;
               former_dest_p = former_dest_p->next;
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
	if(inter->head == NULL)
	{
		var_list_clear(dest);
		return;
	}
	if(dest->head == NULL)
		return;
	struct var_list_node *inter_pointer = inter->head;
	struct var_list_node *dest_pointer = dest->head;
	struct var_list_node *dest_former = dest->head;
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
			var_list_delete(dest_former , dest_pointer);
			dest_pointer = dest_former->next;
		}
		else
			inter_pointer = inter_pointer->next;
		if(inter_pointer == inter->tail->next)
		{
			dest->tail = dest_former;
			break;
		}
		if(dest_pointer == dest->tail->next)
			break;
	}
}

void malloc_active_var()
{
     var_in = (struct var_list *)(malloc(sizeof(struct var_list)) * g_block_num);
     var_out = (struct var_list *)(malloc(sizeof(struct var_list)) * g_block_num);
     def = (struct var_list *)(malloc(sizeof(struct var_list)) * g_block_num);
     use = (struct var_list *)(malloc(sizeof(struct var_list)) * g_block_num);
     def_size = (int *)malloc(sizeof(int) * g_block_num);
     use_size = (int *)malloc(sizeof(int) * g_block_num);
     
     int i;
     for(i = 0 ; i < g_block_num ; i++)
     {
          var_in[i]->head = var_in[i]->tail = NULL;
          var_out[i]->head = var_out[i]->tail = NULL;
          def[i]->head = def[i]->tail = NULL;
          use[i]->head = use[i]->tail = NULL;
          def_size[i] = use_size[i] = 0;
     }
}

void analyse_arg(struct triarg *arg , int type , int block_index)
{
     int i;
     if(arg->type == IdArg)
          i = get_index_of_id(arg->idname);
     else if(arg->type == ExprArg)
          i = get_global_index_of_temp(arg->expr);
     else
          return;
     
     if(type == DEFINE)
     {
          var_info_table[i]->is_define = block_index;
          if(var_info_table[i]->is_use != block_index)
          {
               var_list_append(def + block_index , i);
               def_size[block_index]++;
          }
     }
     else if(type == USE)
     {
          var_info_table[i]->is_use = block_index;
          if(var_info_table[i]->is_define != block_index)
          {
               var_list_append(use + block_index , i);
               use_size[block_index]++;
          }
     }
}

void analyse_arg(int expr_index , int type , int block_index)
{
     int i = get_global_index_of_temp(expr_index);
     
     if(type == DEFINE)
     {
          var_info_table[i]->is_define = block_index;
          if(var_info_table[i]->is_use != block_index)
          {
               var_list_append(def + block_index , i);
               def_size[block_index]++;
          }
     }
     else if(type == USE)
     {
          var_info_table[i]->is_use = block_index;
          if(var_info_table[i]->is_define != block_index)
          {
               var_list_append(use + block_index , i);
               use_size[block_index]++;
          }
     }
}

void initial_active_var()//目前完成活跃变量分析的初始化部分
{
     int i , j;
     malloc_active_var();//活跃变量分析相关的数组空间分配
     struct triargexpr_list *temp;
     for(i = 0 ; i < g_block_num ; i++)
     {
          temp = DFS_array[i]->head;
          while(temp->pre != DFS_array[i]->tail)
          {
               switch(temp->entity->op)//没考虑指针和函数调用时的参数部分
               {
               case Assign:
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
                    analyse_arg(temp->entity->index , DEFINE , i);
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
                    analyse_arg(temp->entity->index , DEFINE , i);
                    analyse_arg(&(temp->entity->arg1) , USE , i);
                    break;
               default:
                    break;
               }
               temp = temp->next;
          }
          var_list_sort(def + i , def_size[i]);//when DEFs and USEs are made
          var_list_sort(use + i , use_size[i]);//sort them so that we can op
     }
}

void solve_equa_ud()//求解活跃变量方程组
{
     initial_active_var();
     int i;
     int change = 1 , next;
     struct basic_block_list *temp_block;
     struct var_list *temp_list;
     temp_list = (struct var_list *)malloc(sizeof(struct var_list));

     while(change)
     {
          change = 0;
          for(i = g_block_num ; i >= 1 ; i--)
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
}

