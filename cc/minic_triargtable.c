#include "minic_triargtable.h"
#include "minic_typedef.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void new_table_list()
{
    table_list =
        malloc(sizeof(struct triargtable *) * INITIALFUNCSIZE);
    table_list_bound = INITIALFUNCSIZE;
    table_list_index = 0;
}

void insert_table(struct triargtable * newtable)
{
    if(table_list_index >= table_list_bound)
    {
        struct triargtable ** tmp_table_list =
            malloc(sizeof(struct triargtable *) * table_list_bound * 2);
        memcpy(tmp_table_list, table_list,
               sizeof(struct triargtable *) * table_list_bound); 
        table_list_bound *= 2;
        free(table_list);
        table_list = tmp_table_list;
    }
    table_list[table_list_index++] = newtable;
}

void free_table_list()//free the global table list
{
    if(table_list != NULL)
    {
        int i = 0;
        while(i < table_list_bound)
        {
            if(table_list[i] != NULL)
                free_table(table_list[i++]);
        }
        free(table_list);
        table_list_bound = 0;
    }
}

//function for index_to_list
struct triargexpr_list ** new_index_to_list()//Malloc and redirect, be free in free_table
{
	int size = gtriargexpr_table_index;
	struct triargexpr_list ** list = calloc(size, sizeof(struct triargexpr_list *));
	struct triargexpr_list * tmpnode = ghead;
	while(tmpnode != NULL)
	{
		//print_triargexpr(*tmpnode -> entity);
		list[tmpnode -> entity -> index] = tmpnode;
		tmpnode = tmpnode -> next;
	}
	return list;
}

struct triargtable * new_table(char *name)//malloc a newtable
{
    struct triargtable * newtable = malloc(sizeof(struct triargtable));
    newtable -> funcname = strdup(name);
    newtable -> head = ghead;
    newtable -> tail = gtail;
    newtable -> table = gtriargexpr_table;
	newtable -> index_to_list = new_index_to_list();
	newtable -> expr_num = gtriargexpr_table_index;
	newtable -> var_id_num = g_var_id_num;
	newtable -> table_bound = gtriargexpr_table_bound;
	return newtable;
}

void free_table(struct triargtable * oldtable)
{
    if(oldtable -> funcname != NULL)
        free(oldtable -> funcname);
    if(oldtable -> head != NULL)
    {
        while(oldtable -> head != oldtable -> tail)
        {
            ;//free_list(oldtable -> head) need a function
            oldtable -> head = oldtable -> head -> next;
        }
        if(oldtable -> head != NULL)
            ;//free_list(oldtable -> head);
    }
    if(oldtable -> table != NULL)
    {
        free(oldtable -> table);//should be like this
    }
	if(oldtable -> index_to_list != NULL)
	{
		free(oldtable -> index_to_list);
	}
}

void new_global_table()//init a new global table
{
	gtriargexpr_table = malloc(sizeof(struct triargexpr) * INITIALCODESIZE);
	ghead = NULL;
	gtail = NULL;
	gtriargexpr_table_bound = INITIALCODESIZE;
	gtriargexpr_table_index = 0;
	g_var_id_num = g_global_id_num;//init visible var with global id num
	/* MARK TAOTAOTHERIPPER */
}

int insert_triargexpr(struct triargexpr expr)//should be in triargexpr.c
{
    if(gtriargexpr_table_index >= gtriargexpr_table_bound)
    {
        struct triargexpr * tmp_expr_table =
            malloc(sizeof(struct triargexpr) * gtriargexpr_table_bound * 2);
        memcpy(tmp_expr_table, gtriargexpr_table,
               sizeof(struct triargexpr) * gtriargexpr_table_bound);
        gtriargexpr_table_bound *= 2;
        free(gtriargexpr_table);
        gtriargexpr_table = tmp_expr_table;
    }
    expr.index = gtriargexpr_table_index;//just in case
	gtriargexpr_table[gtriargexpr_table_index++] = expr;
    return expr.index;
}

void free_global_table()//specially for the extra table
{
	free(gtriargexpr_table);
	ghead = NULL;
	gtail = NULL;
	gtriargexpr_table_bound = 0;
	g_table_list_size = table_list_index;//record the table list size
}

struct taexpr_list_header * new_taexprlist(int begin, int end)
{
	struct taexpr_list_header * new_list = malloc(sizeof(struct taexpr_list_header));
	int i = begin;
	struct triargexpr_list *lastnode, *newnode;
	newnode = (new_list -> head = malloc(sizeof(struct triargexpr_list)));
	newnode -> entity = &gtriargexpr_table[i];
	newnode -> prev = NULL;
	newnode -> next = NULL;
	newnode -> pointer_entity = NULL;
	i++;
	while(i <= end)
	{
		lastnode = newnode;
		newnode = malloc(sizeof(struct triargexpr_list));
		newnode -> entity = &gtriargexpr_table[i++];
		newnode -> prev = lastnode;
		lastnode -> next = newnode;
		newnode -> next = NULL;
//		newnode -> active_var = NULL;
	}
	new_list -> tail = newnode;
	new_list -> nextlist = NULL;
	return new_list;
}

void free_taexprlist(struct taexpr_list_header * old_list_header)
{
	if(old_list_header != NULL)
	    free(old_list_header);
}

struct taexpr_list_header * expr_list_gen(struct subexpr_info * expr)
{
    struct taexpr_list_header * newexpr = new_taexprlist(expr -> begin, expr -> end);
    if(expr -> arithtype == 0)
    {
        newexpr -> nextlist = patch_list_merge(expr -> truelist, expr -> falselist);
    }
    return newexpr;
}

struct taexpr_list_header * value_list_append(struct subexpr_info * expr)
{
    struct taexpr_list_header * newexpr = new_taexprlist(expr -> begin, expr -> end);
    if(expr -> arithtype == 0)
    {
        int beg_index = subexpr_arithval_gen(expr);
        int end_index = beg_index + 3;
        struct taexpr_list_header * addexpr = new_taexprlist(beg_index, end_index);
        newexpr -> tail -> next = addexpr -> head;
        addexpr -> head -> prev = newexpr -> tail;
        newexpr -> tail = addexpr -> tail;
        free_taexprlist(addexpr);
    }
    return newexpr;
} 
//need to deal with NULL
struct taexpr_list_header * if_else_list_merge(struct subexpr_info * condition, struct taexpr_list_header * truestate, struct taexpr_list_header * falsestate)
{
    struct taexpr_list_header * condition_list = new_taexprlist(condition -> begin, condition -> end);
    struct triargexpr_list * tmp_list = condition_list -> tail;
        
    struct triargexpr endtrue_goto;
    //endtrue_goto.index = gtriargexpr_table_index + 1;// may need not, need uniform to do in insert
    endtrue_goto.op = UncondJump;
    endtrue_goto.width = -1;
    endtrue_goto.arg1.type = ExprArg;
    endtrue_goto.arg1.expr = -1;
    int goto_index = insert_triargexpr(endtrue_goto);
    
    struct triargexpr_list * goto_list_node = malloc(sizeof(struct triargexpr_list));
    goto_list_node -> entity = &gtriargexpr_table[goto_index];//if truestate is null, the goto can be delete auto
    
    condition_list -> nextlist = patch_list_append(condition_list -> nextlist, goto_index);
    
    if(condition -> arithtype == 0)
    {
        if(truestate == NULL)
        {
            condition_list -> nextlist = patch_list_merge(condition_list -> nextlist, condition -> truelist);
            tmp_list -> next = goto_list_node;
            goto_list_node -> prev = tmp_list;
        }
        else
        {
            condition_list -> nextlist = patch_list_merge(condition_list -> nextlist, truestate -> nextlist);
            patch_list_backpatch(condition -> truelist, truestate -> head -> entity -> index);
            tmp_list -> next = truestate -> head;
            truestate -> head -> prev = tmp_list;
            tmp_list = truestate -> tail;
            tmp_list -> next = goto_list_node;
            goto_list_node -> prev = tmp_list;
            free_taexprlist(truestate);
        }
        
        if(falsestate == NULL)
        {
            condition_list -> nextlist = patch_list_merge(condition_list -> nextlist, condition -> falselist);
            condition_list -> tail = goto_list_node;
        }
        else
        {         
            condition_list -> nextlist = patch_list_merge(condition_list -> nextlist, falsestate -> nextlist);
            patch_list_backpatch(condition -> falselist, falsestate -> head -> entity -> index);
            goto_list_node -> next = falsestate -> head;
            falsestate -> head -> prev = goto_list_node;
            condition_list -> tail = falsestate -> tail;
            free_taexprlist(falsestate);
        }
    }/*taexpr_list_header * condition_list = new_taexprlist(condition);
    triargexpr endtrue_goto;*/
    else
    {
        struct triargexpr false_goto;
        false_goto.op = FalseJump;
        false_goto.width = -1;
        false_goto.arg1 = condition -> subexpr_arg;
        false_goto.arg2.type = ExprArg;
        if(falsestate == NULL)
            false_goto.arg2.expr = -1;
        else
            false_goto.arg2.expr = falsestate -> head -> entity -> index;
        int false_goto_index = insert_triargexpr(false_goto);
        
        struct triargexpr_list * false_goto_list_node = malloc(sizeof(struct triargexpr_list));
        false_goto_list_node -> entity = &gtriargexpr_table[false_goto_index];
        
        if(falsestate == NULL)
            condition_list -> nextlist = patch_list_append(condition_list -> nextlist, false_goto_index);
        else
            condition_list -> nextlist = patch_list_merge(condition_list -> nextlist, falsestate -> nextlist);
            
        if(truestate != NULL || falsestate != NULL)//has optimized some
        {
            tmp_list -> next = false_goto_list_node;
            false_goto_list_node -> prev = tmp_list;
            tmp_list = false_goto_list_node;//Jump to the next expr, can be optimized
            if(truestate != NULL)
            {
                condition_list -> nextlist = patch_list_merge(condition_list -> nextlist, truestate -> nextlist);
                tmp_list -> next = truestate -> head;
                truestate -> head -> prev = tmp_list;
                tmp_list = truestate -> tail;
                free_taexprlist(truestate);
            }
            if(falsestate != NULL)
            {
                tmp_list -> next = goto_list_node;
                goto_list_node -> prev = tmp_list;
                tmp_list = goto_list_node;//if falsestate == NULL the goto will jump to the next expr, can be optimized
                /*has done some optimizing*/
            
                tmp_list -> next = falsestate -> head;
                falsestate -> head -> prev = goto_list_node;
                tmp_list = falsestate -> tail;
                free_taexprlist(falsestate);
            }
        }
        condition_list -> tail = tmp_list;
    }
    //free(condition);
    return condition_list;
}

struct taexpr_list_header * if_list_merge(struct subexpr_info * condition, struct taexpr_list_header * truestate)
{
    struct taexpr_list_header * condition_list = new_taexprlist(condition -> begin, condition -> end);
    struct triargexpr_list * tmp_list = condition_list -> tail;
    
    if(condition -> arithtype == 0)
    {
        if(truestate == NULL)
            condition_list -> nextlist = patch_list_merge(condition_list -> nextlist, condition -> truelist);
        else
        {
            patch_list_backpatch(condition -> truelist, truestate -> head -> entity -> index);//it seems that use index is better than pointer
            condition_list -> nextlist = patch_list_merge(condition_list -> nextlist, truestate -> nextlist);
        }
        
        condition_list -> nextlist = patch_list_merge(condition_list -> nextlist, condition -> falselist);
        
        if(truestate != NULL)
        {
            tmp_list -> next = truestate -> head;
            truestate -> head -> prev = tmp_list;
            tmp_list = truestate -> tail;
            free_taexprlist(truestate);
        }
        //free(condition);???
    }/*taexpr_list_header * condition_list = new_taexprlist(condition);
    triargexpr endtrue_goto;*/
    else
    {
        if(truestate == NULL)
            tmp_list = condition_list -> tail;//has optmize some, in face some can be optimized above, but I've left them to code optimizing
        else
        {
            struct triargexpr false_goto;
            false_goto.op = FalseJump;
            false_goto.width = -1;
            false_goto.arg1 = condition -> subexpr_arg;
            false_goto.arg2.type = ExprArg;
            false_goto.arg2.expr = -1;
            //false_goto.index = gtriargexpr_table_index + 1;// may need not, need uniform to do in insert
            int false_goto_index = insert_triargexpr(false_goto);
            
            condition_list -> nextlist = patch_list_append(truestate -> nextlist, false_goto_index);//should not free truestate -> nextlist
        
            struct triargexpr_list * false_goto_list_node = malloc(sizeof(struct triargexpr_list));
            false_goto_list_node -> entity = &gtriargexpr_table[false_goto_index];
        
            tmp_list -> next = false_goto_list_node;
            false_goto_list_node -> prev = tmp_list;
        
            false_goto_list_node -> next = truestate -> head;
            truestate -> head -> prev = false_goto_list_node;
            tmp_list = truestate -> tail;
            
            free_taexprlist(truestate);
            //printf("htere\n");//****************************
        }
        
    }
    
    condition_list -> tail = tmp_list;
    return condition_list;
}

struct taexpr_list_header * while_list_merge(struct subexpr_info * condition, struct taexpr_list_header * loopbody)
{
    struct taexpr_list_header * condition_list = new_taexprlist(condition -> begin, condition -> end);
    struct triargexpr_list * tmp_list = condition_list -> tail;
    
    if(loopbody != NULL)
        patch_list_backpatch(loopbody -> nextlist, condition -> begin);
    
    struct triargexpr reloop_goto;
    //endtrue_goto.index = gtriargexpr_table_index + 1;// may need not, need uniform to do in insert
    reloop_goto.op = UncondJump;
    reloop_goto.width = -1;
    reloop_goto.arg1.type = ExprArg;
    reloop_goto.arg1.expr = condition -> begin;//jump back
    int reloop_index = insert_triargexpr(reloop_goto);
        
    struct triargexpr_list * reloop_list_node = malloc(sizeof(struct triargexpr_list));
    reloop_list_node -> entity = &gtriargexpr_table[reloop_index];
    if(condition -> arithtype == 0)
    {
        if(loopbody == NULL)
            patch_list_backpatch(condition -> truelist, condition -> begin);
        else
            patch_list_backpatch(condition -> truelist, loopbody -> head -> entity -> index);
        
        condition_list -> nextlist = condition -> falselist;//shouldn't delete, should be deleted in backpatch
        
        if(loopbody == NULL)
            tmp_list -> next = reloop_list_node;
        else
        {
            tmp_list -> next = loopbody -> head;
            loopbody -> head -> prev = tmp_list;
            tmp_list = loopbody -> tail;
            tmp_list -> next = reloop_list_node;
            free_taexprlist(loopbody);
        }
    }
    else
    {
        struct triargexpr false_goto;
        false_goto.op = FalseJump;
        false_goto.width = -1;
        false_goto.arg1 = condition -> subexpr_arg;
        //printf("%d %d\n",false_goto.arg1.type, condition -> subexpr_arg.type);
        //printf("lala");
        false_goto.arg2.type = ExprArg;
        false_goto.arg2.expr = -1;
        //false_goto.index = gtriargexpr_table_index + 1;// may need not, need uniform to do in insert
        int false_goto_index = insert_triargexpr(false_goto);
        
        struct triargexpr_list * false_goto_list_node = malloc(sizeof(struct triargexpr_list));
        false_goto_list_node -> entity = &gtriargexpr_table[false_goto_index];
        
        condition_list -> nextlist = patch_list_append(condition_list -> nextlist, false_goto_index);
        
        tmp_list -> next = false_goto_list_node;
        false_goto_list_node -> prev = tmp_list;
        tmp_list = false_goto_list_node;
        
        if(loopbody == NULL)
            tmp_list -> next = reloop_list_node;
        else
        {
            tmp_list -> next = loopbody -> head;
            loopbody -> head -> prev = tmp_list;
            tmp_list = loopbody -> tail;
            tmp_list -> next = reloop_list_node;
            free_taexprlist(loopbody);
        }
    }
    reloop_list_node -> prev = tmp_list;
    condition_list -> tail = reloop_list_node;
    return condition_list;
}
        
struct taexpr_list_header * for_list_merge(struct subexpr_info * init_expr,struct subexpr_info * cond_expr,
                                           struct subexpr_info * change_expr, struct taexpr_list_header * loopbody)
{
//order:init->cond->loopbody->change->goto_cond
     struct taexpr_list_header * temp_header = new_taexprlist(init_expr->begin , init_expr->end);
	 struct taexpr_list_header * cond_header = new_taexprlist(cond_expr->begin , cond_expr->end); 
	 struct taexpr_list_header * change_header = new_taexprlist(change_expr->begin , change_expr->end);
     struct triargexpr_list * goto_expr_list = (struct triargexpr_list *)malloc(sizeof(struct triargexpr_list));
     int cond_expr_index = cond_expr->begin;

     struct triargexpr goto_expr;//the last UncondJump tri_expr
     goto_expr.op = UncondJump;
     goto_expr.width = -1;
     goto_expr.arg1.type = ExprArg;
     goto_expr.arg1.expr = cond_expr_index;
     int goto_index = insert_triargexpr(goto_expr);//insert the UncondJump tri_expr to the table 
     goto_expr_list->entity = gtriargexpr_table + goto_index;
     
     if(init_expr->arithtype == 0)//如果初始化表达式或者改变表达式是逻辑表达式，要把他们的truelist和falselist合并成nextlist，并回填
     {
          temp_header->nextlist = patch_list_merge(init_expr->truelist , init_expr->falselist);
          patch_list_backpatch(temp_header->nextlist , cond_expr_index);//初始化表达式的下一步是条件判断
     }
     if(change_expr->arithtype == 0)
     {
          change_header->nextlist = patch_list_merge(change_expr->truelist , change_expr->falselist);
          patch_list_backpatch(change_header->nextlist , cond_expr_index);//变化表达式下一步也是条件判断，这样省略了UncondJump
     }

     //this part is to merge the for part and the last UncondJump tri_expr
     temp_header->tail->next = cond_header->head;
     cond_header->head->prev = temp_header->tail;
     temp_header->tail = cond_header->tail;//merge initial part and condition part

     int cond_true_to_index;//计算条件正确时的跳转位置，如果循环体为空，则跳转到变化表达式
     if(loopbody == NULL)
          cond_true_to_index = change_expr->begin;
     else
          cond_true_to_index = loopbody->head->entity->index;
     if(cond_expr->arithtype == 0)//condition is a bool expression
     {
          temp_header->nextlist = cond_expr->falselist;//the direct table's nextlist is the condition part's falselist
          patch_list_backpatch(cond_expr->truelist , cond_true_to_index);//if the condition is true,goto the change part
     }
     else if(cond_expr->arithtype == 1)//condition is an arithmetic expression
     {
          struct triargexpr arith_goto;//如果条件是算术表达式，需要加一条语句：FalseJump arg1 nextlist
          arith_goto.op = FalseJump;
          arith_goto.width = -1;
          arith_goto.arg1 = cond_expr->subexpr_arg;//arg1就是条件表达式变成链的最后一条语句
          arith_goto.arg2.type = ExprArg;
          arith_goto.arg2.expr = -1;
          int arith_goto_index = insert_triargexpr(arith_goto);
          temp_header->nextlist = (struct patch_list *)malloc(sizeof(struct patch_list));
          temp_header->nextlist->index = arith_goto_index;//nextlist也就是该for语句的nextlist
          temp_header->nextlist->next = NULL;
          
          //将FalseJump arg1 nextlist封装成triargexpr_list，穿成链
          struct triargexpr_list * arith_goto_list = (struct triargexpr_list *)malloc(sizeof(struct triargexpr_list));
          arith_goto_list->entity = gtriargexpr_table + arith_goto_index;
          temp_header->tail->next = arith_goto_list;//
          arith_goto_list->prev = temp_header->tail;
          temp_header->tail = arith_goto_list;
          arith_goto_list->next = NULL;
     }
     
     if(loopbody == NULL)
          goto after_loop;
          
     patch_list_backpatch(loopbody -> nextlist, change_expr->begin); 
	 
     temp_header->tail->next = loopbody->head;
     loopbody->head->prev = temp_header->tail;
     temp_header->tail = loopbody->tail;//if it has the loop body,append it to the direct table 
	   
after_loop:
	 temp_header->tail->next = change_header->head;
     change_header->head->prev = temp_header->tail;
     temp_header->tail = change_header->tail;//append the change part

     temp_header->tail->next = goto_expr_list;//最后加上UncondJump con_expr_index
     goto_expr_list->prev = temp_header->tail;
     goto_expr_list->next = NULL;
     temp_header->tail = goto_expr_list;
     
     free_taexprlist(cond_header);
     free_taexprlist(change_header);
     free_taexprlist(loopbody);

     return temp_header;
}

    
struct taexpr_list_header * stmt_list_merge(struct taexpr_list_header * pre, struct taexpr_list_header * sub)
{
     if(pre == NULL)
		 return sub;
     if(sub == NULL)
		 return pre;
     patch_list_backpatch( pre->nextlist , sub->head->entity->index );
     pre->tail->next = sub->head;
     sub->head->prev = pre->tail;
     pre->tail = sub->tail;
     //free_patch_list( pre->nextlist );
     pre->nextlist = sub->nextlist;
     free_taexprlist(sub);
     return pre;
}

struct taexpr_list_header * return_list_append(struct subexpr_info * value)
{
     struct triargexpr ret_expr;
     ret_expr.op = Return;
     ret_expr.arg1.type = ExprArg;
     ret_expr.arg1.expr = -1;

     struct taexpr_list_header *expr_header;
     if(value != NULL)
         expr_header = value_list_append(value);
     
     struct triargexpr_list *ret_list = (struct triargexpr_list *)calloc(1, sizeof(struct triargexpr_list));
     int ret_index = insert_triargexpr(ret_expr);
     ret_list->entity = gtriargexpr_table + ret_index;
     ret_list->prev = ret_list->next = NULL;
     
     struct taexpr_list_header *ret_header = (struct taexpr_list_header *)calloc(1, sizeof(struct taexpr_list_header));
     ret_header->head = ret_header->tail = ret_list;
     ret_header->nextlist = NULL;
     
     if(value == NULL)
          return ret_header;

     if(value->subexpr_arg.type == ImmArg)//if the expression returned is an immidiate number,just return it
     {
          gtriargexpr_table[ret_index].arg1.type = ImmArg;
          gtriargexpr_table[ret_index].arg1.expr = value->subexpr_arg.imme;
          return ret_header;
     }

     if(value->subexpr_arg.type == IdArg)
     {
         gtriargexpr_table[ret_index].arg1.type = IdArg;
         gtriargexpr_table[ret_index].arg1.idname = value->subexpr_arg.idname;
         return ret_header;
     }
     
     gtriargexpr_table[ret_index].arg1.expr = value->end;
     expr_header->tail->next = ret_list;
     ret_list->prev = expr_header->tail;
     expr_header->tail = ret_list;
     return expr_header;
}

void print_table(struct triargtable *table)
{
    printf("%s\n", table -> funcname);
    struct triargexpr_list * tmp;
    tmp = table -> head;
	while(tmp != NULL)
    {
        print_triargexpr(*(tmp -> entity));
        tmp = tmp -> next;
    }
	printf("%s end\n\n",table -> funcname);
}

extern void print_list_header(struct taexpr_list_header * list)
{
    struct triargexpr_list *tmp = list -> head;
    while(tmp != NULL)
    {
        print_triargexpr(*(tmp -> entity));
        tmp = tmp -> next;
    }
}
