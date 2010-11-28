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

int compare (const void * a, const void * b) 
{
     return ( *(int*)a - *(int*)b );
}

void var_list_sort(struct var_list *list_array , int size)//将一个定值点链表或者变量编号链表排序
{
     int i , j;
     int *trans_array;
     var_list_node *temp_node;
     trans_array = (int *)malloc(sizeof(int) * size);
     temp_node = list_array->head;
     for(j = 0 ; j < size ; j++)
     {
          trans_array[j] = temp_node->var_map_index;
          temp_node = temp_node->next;
     }
     qsort(trans_array , size , sizeof(int) , compare);
     temp_node = list_array->head;
     for(j = 0 ; j < size ; i++)
     {
          temp_node->var_map_index = trans_array[j];
          temp_node = temp_node->next;
     }
     free(trans_array);
     free(size);//有可能要放到其他地方××××××××××××××××××××××
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

void var_list_copy(struct var_list *source , struct var_list *dest)
{
     if(source == NULL)//尽量不要出现这种情况
     {
          var_list_clear(dest);
          return;
     }
     struct var_list_node *temp = source->head;
     while(temp != source->tail)
     {
          var_list_append(dest , temp->var_map_index);
          temp = temp->next;
     }
     var_list_append(dest , temp->var_map_index));
     return;
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
          i = get_index_of_temp(arg->expr);
     else
          return;
     if(i < 0 || i >= map_bridge_cur_index)//如果不是变量，不与考虑
          return;
     
     i = map_bridge[i];
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
     int i = get_index_of_temp(expr_index);
     if(i < 0 || i >= map_bridge_cur_index)//如果不是变量，不与考虑
          return;
     
     i = map_bridge[i];
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
     for(i = 0 ; i < map_bridge_cur_index ; i++)
     {
          j = map_bridge[i];
          var_info_table[j]->is_define = -1;
          var_info_table[j]->is_use = -1;
     }
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

void var_list_merge(struct var_list *adder , struct var_list *dest)
{//将变量链adder和dest合并，并写入dest内
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
     while(true)
     {
          if(adder_pointer->var_map_index == dest_pointer->var_map_index)
          {
               adder_pointer = adder_pointer->next;
               former_dest_p = dest_pointer;
               dest_pointer = dest_pointer-next;
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

void var_list_sub(struct var_list *sub , struct var_list *subed , struct var_list *dest)
{//将变量链dest = sub - subed
     if(sub->head == NULL)
     {
          var_list_clear(dest);
          return;
     }
     if(subed->head == NULL)
     {
          var_list_copy(sub , dest);
          return;
     }
     struct var_list_node *sub_pointer = sub->head;
     struct var_list_node *subed_pointer = subed->head;
     while(true)
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

void solve_equa_ud()
{
     initial_active_var();
     int i;
     int change = 1 , next;
     struct basic_block_list *temp;
     for(i = g_block_num ; i >= 1 ; i--)
     {
          temp = DFS_array[i]->next;
          var_list_clear(var_out + i);//clear the v_out of the block before op
          while(temp != NULL)
          {
               next = temp->entity->index;
               var_list_merge(var_in + next , var_out + i);
               //if(var_in[next].head == NULL)//这个变量链什么都没有
               //var_list_copy(var_in[next] , var_out[i]);
               temp = temp->next;
          }
     }
}

extern void make_udlist();

extern void equation_expr();

extern void equation_var();
extern void make_actvarlist();

extern void add_set_local(point_list de , point_list add);//将de和add求并，结果在de中
extern void add_set(point_list de , point_list a1 , point_list a2);//de=a1并a2
extern void mul_set_local(point_list de , point_list add);//将de和add求交，结果在de中
extern void mul_set(point_list de , point_list a1 , point_list a2);//de=a1交a2
extern void sub_add_set(point_list de , point_list a , point_list b , point_list c);//集合de=(a-b)并c

extern void free_point_list(point_list head);
