#include "minic_udanalyse.h"
#include "minic_varmapping.h"
#include <stdio.h>
#include <stdlib.h>

struct def_point_list
{
	int id_index;
	struct var_list * define_point;
	struct def_point_list * next;
};


static int cur_var_id_num;
static struct def_point_list ** ud_in;
static struct def_point_list ** ud_out;
static struct def_point_list ** ud_gen;
static int * local_var_id_flag;

/*static inline void new_def_point_list()
{*/

static inline int set_cur_function(int function_index)
{
	cur_var_id_num = table_list[function_index] -> var_id_num;
}

static struct def_point_list * new_def_point_list()
{
	struct def_point_list * head = NULL;
	struct def_point_list * lastnode = NULL;
	struct def_point_list * tmpnode = NULL;
	int index;
	for(index = 0; index < cur_var_id_num; index++)
	{
		if(local_var_id_flag[index] != -1)
		{
			local_var_id_flag[index] = -1;
			tmpnode = malloc(sizeof(struct def_point_list));
			tmpnode -> id_index = index;
			tmpnode -> define_point = malloc(sizeof(struct var_list));
			var_list_append(tmpnode -> define_point, local_var_id_flag[index]);
			tmpnode -> next = NULL;
			if(head == NULL)
				head = tmpnode;
			else
				lastnode -> next = tmpnode;
			lastnode = tmpnode;
		}
	}
	return head;//if no def, then return NULL
}

static void free_def_point_list(struct def_point_list * oldlist)
{
	struct def_point_list * tempnode;
	while(oldlist != NULL)
	{
		tempnode = oldlist;
		oldlist = oldlist -> next;
		var_list_free(tempnode -> define_point);//the define_point itself has been free, too
		free(tempnode);
	}
}

static void new_temp_list()
{
	ud_in = malloc(sizeof(struct def_point_list **) * g_block_num);
	ud_out = malloc(sizeof(struct def_point_list **) * g_block_num);
	ud_gen = malloc(sizeof(struct def_point_list **) * g_block_num);
	local_var_id_flag = malloc(sizeof(int) * cur_var_id_num);	
	int index;
	for(index = 0; index < cur_var_id_num; index ++)
		local_var_id_flag[index] = -1;
}

static void free_temp_list()
{
	int index;
	for(index = 0; index < g_block_num; index ++)
	{
		free_def_point_list(ud_in[index]);
		free_def_point_list(ud_out[index]);
		free_def_point_list(ud_gen[index]);
	}
	free(local_var_id_flag);
	free(ud_in);
	free(ud_out);
	free(ud_gen);
}

static inline struct def_point_list * def_point_list_node_copy(struct def_point_list * list_node)//next is NULL
{
	struct def_point_list * new_node = malloc(sizeof(struct def_point_list));
	new_node -> id_index = list_node -> id_index;
	new_node -> define_point = malloc(sizeof(struct var_list));
	var_list_copy(list_node -> define_point, new_node -> define_point);
	new_node -> next = NULL;
	return new_node;
}

static struct def_point_list * def_point_list_copy(struct def_point_list * list_head)
{
	struct def_point_list * head = NULL;
	struct def_point_list * temp_node = list_head;
	struct def_point_list * last_node = NULL;
	while(temp_node != NULL)
	{
		struct def_point_list * new_node = def_point_list_node_copy(temp_node);
		if(head == NULL)
			head = new_node;
		else last_node -> next = new_node;
		last_node = new_node;
		temp_node = temp_node -> next;
	}
	return head;//if temp_node == NULL return NULL
}

static struct def_point_list * def_point_list_merge(struct def_point_list * adder, struct def_point_list * dest)
{
	if(adder == NULL)
		return dest;
	else if(dest == NULL)
		return adder;

	struct def_point_list * head = NULL;
	struct def_point_list * adder_cur_node = adder;
	struct def_point_list * dest_cur_node = dest;
	struct def_point_list * dest_last_node = NULL;
	struct def_point_list * dest_new_node = NULL;
	
	while(1)
	{
		if(adder_cur_node == NULL)//head can't be NULL here
			return head;
		else if(dest_cur_node == NULL)//head can't be NULL here, either 
		{
			if(adder_cur_node != NULL)
				dest_new_node = def_point_list_copy(adder_cur_node);
			dest_last_node -> next = dest_new_node;	
			return head;
		}
		else if(adder_cur_node -> id_index < dest_cur_node -> id_index)
		{
			dest_new_node = def_point_list_node_copy(adder_cur_node);
			if(head == NULL)
				head = dest_new_node;
			else dest_last_node -> next = dest_new_node;
			
			dest_new_node -> next = dest_cur_node;
			dest_last_node = dest_new_node;

			adder_cur_node = adder_cur_node -> next;
		}
		else if(adder_cur_node -> id_index > dest_cur_node -> id_index)
		{
			if(head == NULL)
				head = dest_cur_node;
			dest_last_node = dest_cur_node;
			dest_cur_node = dest_cur_node -> next;
		}
		else
		{
			var_list_merge(adder_cur_node -> define_point, dest_cur_node -> define_point);//the two define_point cann't be empty, Mark
			if(head == NULL)
				head = dest_cur_node;
			dest_last_node = dest_cur_node;
			dest_cur_node = dest_cur_node -> next;
			adder_cur_node = adder_cur_node -> next;
		}
	}
}

static struct def_point_list * def_point_list_sub_merge(struct def_point_list *cur_in, struct def_point_list * cur_gen)
{
	if(cur_gen == NULL)
		return def_point_list_copy(cur_in);//if cur_in is NULL, will return NULL
	else if(cur_in == NULL)
		return def_point_list_copy(cur_gen);
	
	struct def_point_list * head = NULL;
	struct def_point_list * in_cur_node = cur_in;
	struct def_point_list * gen_cur_node = cur_gen;
	struct def_point_list * out_last_node = NULL;
	struct def_point_list * out_new_node = NULL;
	
	while(1)
	{
		if(gen_cur_node == NULL)//head can't be NULL here
		{
			if(in_cur_node != NULL)
			{
				out_new_node = def_point_list_copy(in_cur_node);
				out_last_node -> next = out_new_node;
			}
			return head;
		}
		else if(in_cur_node == NULL)
		{
			if(gen_cur_node != NULL)
			{
				out_new_node = def_point_list_copy(gen_cur_node);
				out_last_node -> next = out_new_node;
			}
			return head;
		}
		else if(in_cur_node -> id_index < gen_cur_node -> id_index)
		{
			out_new_node = def_point_list_node_copy(in_cur_node);
			if(head == NULL)
				head = out_new_node;
			else out_last_node -> next = out_new_node;
			out_last_node = out_new_node;
			in_cur_node = in_cur_node -> next;
		}
		else
		{
			out_new_node = def_point_list_node_copy(gen_cur_node);
			if(head == NULL)
				head = out_new_node;
			else out_last_node -> next = out_new_node;
			out_last_node = out_new_node;
			gen_cur_node = gen_cur_node -> next;
			
			if(in_cur_node -> id_index == gen_cur_node -> id_index)
				in_cur_node = in_cur_node -> next;
		}	
	}	
}

static int def_point_list_is_equal(struct def_point_list * first_list, struct def_point_list * second_list)
{
	if(first_list == NULL && second_list == NULL)
		return 1;
	struct def_point_list * first_tmp = first_list, * second_tmp = second_list;
	while(first_tmp != NULL && second_tmp != NULL)
	{
		if(first_tmp -> id_index != second_tmp -> id_index)
			return 0;
		else if(var_list_isequal(first_tmp -> define_point, second_tmp -> define_point) != 1)
			return 0;
		first_tmp = first_tmp -> next;
		second_tmp = second_tmp -> next;
	}
	if(first_tmp == NULL && second_tmp == NULL)
		return 1;
	else return 0;
}

static struct def_point_list * generate_gen_for_each(struct basic_block * block)
{
	struct triargexpr_list * temp_node = block -> head;
	int index;
	while(temp_node != NULL)
	{
		struct triargexpr * expr = temp_node -> entity;
		struct var_info * id_info;
		if(expr -> op == Assign && expr -> arg1.type == IdArg)//Nullop?
		{
			index = get_index_of_id(expr -> arg1.idname);
			local_var_id_flag[index] = expr -> index;
		}		
		temp_node = temp_node -> next;	
	}
	return new_def_point_list();
}

static void generate_gen_for_all()
{
	int index;
	for(index = 0; index < g_block_num; index ++)
		ud_gen[index] = generate_gen_for_each(DFS_array[index]);	
}

static void generate_in_out_for_all()
{
	int index;
	for(index = 0; index < g_block_num; index ++)
		ud_out[index] = def_point_list_copy(generate_gen_for_each(DFS_array[index]));
	int change = 1;
	while(change)
	{
		change = 0;
		for(index = 0; index < g_block_num; index++)
		{
			struct basic_block_list * list_node = DFS_array[index] -> prev;
			while(list_node != NULL)
			{
				ud_in[index] = def_point_list_merge(ud_out[list_node -> entity-> index], ud_in[index]);
				list_node = list_node -> next;
			}

			struct def_point_list * newout;
			newout = def_point_list_sub_merge(ud_in[index], ud_gen[index]);
			if(def_point_list_is_equal(newout, ud_out[index]) != 1)
			{
				change = 1;
				def_point_list_free(ud_out[index]);
				ud_out[index] = newout;//see what the function is next
			}
		}
	}
}

static void generate_ud_for_each_block()
{

}

static void generate_ud_for_all_block()
{
	int index;
	for(index = 0; index < g_block_num; index ++)
	{
		ud_gen[index] = generate_gen_for_each_block(index);
	}
}

void ud_analyse(int function_index)
{
	set_cur_function(function_index);//set the function	
	new_temp_list();
	generate_gen_for_all();
	generate_in_out_for_all();
	generate_ud_for_all_block();
	free_temp_list();
}

