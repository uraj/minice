#include "minic_typedef.h"
#include "minic_typetree.h"
#include "minic_symtable.h"
#include "minic_symfunc.h"
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern int yylineno;
//maybe somewhere else
void decl_retype_insert(struct symbol_stack * type_stack, struct symbol_table * curr_table,
                        enum data_type curr_type, enum data_type curr_modi)
{
	struct value_type * tmptype;
    while(type_stack -> top != NULL)
    {
    	tmptype = syms_pop(type_stack);
    	if(curr_type == Void)/* special for void */
    	{
    		if(tmptype -> cur_typenode -> type == Simpletype)/* not pointer */
    		{
    			if(tmptype -> value -> type -> type == Array)
    			{
                    fprintf(stderr, "Line %d: the Array \'%s\' is declared voids.\n", yylineno, tmptype -> value -> name);/*error! void vars decled*/
                    tmptype -> cur_typenode -> type = Typeerror;
                    goto decl_retype_insert_end;
    			}
    			else if(tmptype -> value -> type -> type == Simpletype)
                {
    				fprintf(stderr, "Line %d: the variable \'%s\' is declared void.\n", yylineno, tmptype -> value -> name);/*error! void var decled*/
    				tmptype -> cur_typenode -> type = Typeerror;
    				goto decl_retype_insert_end;
    			}
    		}
    	}
        tmptype -> cur_typenode -> type = curr_type;
        
decl_retype_insert_end:
		tmptype -> value -> modi = curr_modi;/* up or down , either is OK */
		if(!symt_insert(curr_table, tmptype -> value))
		    fprintf(stderr, "Line %d: The token \'%s\' is redefined.\n", yylineno, tmptype -> value -> name);//error! redefine! maybe need more variable...
    	delete_valuetype(tmptype);
    }
}

void decl_add_pointer(struct value_type * newtype)
{
    struct typetree * oldtype = newtype -> cur_typenode;//cur_typenode can only be a simple type
    struct typetree * tmptype = typet_new_type(Pointer);//change simpletype to pointer to mark the pointer change
    oldtype -> base_type = tmptype;
    oldtype -> type = Pointer;
    newtype -> cur_typenode = tmptype;
}

void decl_push_array(char * name, int size, struct symbol_stack * type_stack)
{
	struct value_type * newtype;
   	newtype = new_valuetype(name);
    newtype -> value -> type = typet_new_type(Array);
    newtype -> value -> type -> size = size;
    newtype -> value -> type -> base_type = typet_new_type(Simpletype);
    newtype -> cur_typenode = newtype -> value -> type -> base_type;
    syms_push(type_stack, newtype);
}

struct value_type * decl_push_simple(char * name, struct symbol_stack * type_stack)
{
    struct value_type * newtype;
    newtype = new_valuetype(name);
    newtype -> value -> type = typet_new_type(Simpletype);
    newtype -> cur_typenode = newtype -> value -> type;
    syms_push(type_stack, newtype);
    return newtype;
}

void dd_push_parm(char * name, struct symbol_stack * parm_stack, enum data_type new_parm_type, enum data_type pointer_type)
{
	struct value_type * newtype;
    newtype = new_valuetype(name);
    if(new_parm_type == Void && pointer_type != Pointer)/*special for void, parm must be a simple type*/
    {
        fprintf(stderr, "Line %d: The parameter \'%s\' is declared void.", yylineno, newtype -> value -> name);/*void parm*/
        newtype -> value -> type = typet_new_type(Typeerror);
        newtype -> cur_typenode = typet_new_type(Typeerror);
    }
    else
    {
        if(pointer_type != Pointer)
        {
            newtype -> value -> type = typet_new_type(new_parm_type);
            newtype -> cur_typenode = typet_new_type(new_parm_type);
        }
        else
        {
            newtype -> value -> type = typet_new_type(Pointer);
            newtype -> value -> type -> base_type = typet_new_type(new_parm_type);
            newtype -> cur_typenode = typet_new_type(Pointer);
            newtype -> cur_typenode -> base_type = typet_new_type(new_parm_type);
        }
    }
    syms_push(parm_stack, newtype);
}

struct value_type * decl_push_func(char * name, struct symbol_stack * type_stack, struct symbol_stack * parm_stack)
{
    struct value_type * newtype;
    struct value_type * parmvalue;
    struct typetree * parmtype;
    struct typetree * tmptype;
    newtype = new_valuetype(name);
    newtype -> value -> type = typet_new_type(Function);
    while(parm_stack -> top != NULL)
    {
        parmvalue = syms_pop(parm_stack);
        parmtype = parmvalue -> cur_typenode;
        delete_valueinfo(parmvalue -> value);
        delete_valuetype(parmvalue);
        if(newtype -> value -> type -> parm_list == NULL)
       	{
            newtype -> value -> type -> parm_list = parmtype;
            continue;
        }
        tmptype = newtype -> value -> type -> parm_list;
        newtype -> value -> type -> parm_list = parmtype;
        newtype -> value -> type -> parm_list -> next_parm = tmptype;
    }
    newtype -> value -> type -> return_type = typet_new_type(Simpletype);
    newtype -> cur_typenode = newtype -> value -> type -> return_type;
    syms_push(type_stack, newtype);
    return newtype;
}

void def_insert_func(char * name, struct symbol_stack * parm_stack, struct symbol_table * simb_table, 
                     struct symbol_table * curr_table, enum data_type type_name, enum data_type pointer_type)
{
    struct value_info * newinfo;
    struct value_type * parmvalue;
    struct typetree * parmtype;
    struct typetree * tmptype;
    newinfo = new_valueinfo(name);
    newinfo -> type = typet_new_type(Function);
    start_arglist();
    while(parm_stack -> top != NULL)
    {
        parmvalue = syms_pop(parm_stack);
        parmtype = parmvalue -> cur_typenode;
        if(!symt_insert(curr_table, parmvalue -> value))
            fprintf(stderr, "Line %d: The parameter \'%s\' is redefined.\n", yylineno, parmvalue -> value -> name);
            //error! redefine! maybe need more variable...
        if(newinfo -> type -> parm_list == NULL)
        {
            newinfo -> type -> parm_list = parmtype;
            continue;
        }
        tmptype = newinfo -> type -> parm_list;
        newinfo -> type -> parm_list = parmtype;
        newinfo -> type -> parm_list -> next_parm = tmptype;
    }
    end_arglist();
    
    if(pointer_type == Pointer)
    {
        newinfo -> type -> return_type = typet_new_type(Pointer);
        newinfo -> type -> return_type -> base_type = typet_new_type(type_name);
    }
    else
        newinfo -> type -> return_type = typet_new_type(type_name);
    newinfo -> func_symt = curr_table;
    curr_table -> func = newinfo;
    if(!symt_insert(simb_table, newinfo))
        fprintf(stderr, "Line %d: The function \'%s\' is redefined.\n", yylineno, name);//error! redefine! maybe need more variable...
}
    
//may be somewhere else
struct typetree * cal_lookup_var(char * name, struct symbol_table * simb_table, struct symbol_table * curr_table)
{
    struct value_info * varinfo = symbol_search(simb_table, curr_table, name);
    if(varinfo == NULL)
    {
        fprintf(stderr, "Line %d: The token \'%s\' is used but undefined.\n", yylineno, name);//error! undefine!
        varinfo = new_valueinfo(name);
        varinfo -> type = typet_new_type(Typeerror);
        symt_insert(curr_table, varinfo);
    }
    return varinfo -> type;
}
