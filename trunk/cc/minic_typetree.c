#include "minic_typetree.h"
#include "minic_typedef.h"
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct typetree * typet_new_type(enum data_type newtype)
{
	struct typetree * newtree = malloc(sizeof(struct typetree));
	newtree -> type = newtype;
    newtree -> size = 0;
    newtree -> return_type = NULL;
    newtree -> parm_list = NULL;
    newtree -> base_type = NULL;
    newtree -> next_parm = NULL;
	return newtree;
}

struct typetree * typet_typetree_dup(const struct typetree * src)
{
    struct typetree * obj;
    if(src == NULL)
        obj = NULL;
    else
    {
        obj = malloc(sizeof(struct typetree));
        obj -> type = src -> type;
        switch(src -> type)
        {
            case Array:
                obj -> size = src -> size;
                obj -> base_type = typet_typetree_dup(src -> base_type);
                break;
            case Pointer:
                obj -> base_type = typet_typetree_dup(src -> base_type);
                break;
            case Function:
                obj -> return_type = typet_typetree_dup(src -> return_type);
                obj -> parm_list = typet_typetree_dup(src -> parm_list);
                break;
            default:
                obj -> next_parm = typet_typetree_dup(src -> next_parm);
        }        
    }
    return obj;
}

int typet_type_equal(struct typetree * t1, struct typetree * t2)
{
    if( t1 == NULL || t2 == NULL )
    {
         if( t1 == t2 )
              return 1;
         return 0;
    }
    if( t1->type != t2->type )
		return 0;
	int child_equal = 1 , sibling_equal = 1;
    if( t1->type == Function )
    {
         sibling_equal = typet_type_equal( t1->return_type , t2->return_type );
         child_equal = typet_type_equal( t1->parm_list , t2->parm_list );
    }
	else
    {
		child_equal = typet_type_equal( t1->base_type , t2->base_type );
        if( t1->type != Array )
             sibling_equal = typet_type_equal( t1->next_parm , t2->next_parm );
    }
	return ( child_equal & sibling_equal );
}

void typet_free_typetree(struct typetree * oldtype)
{
    if(!oldtype)
        return;
    else if(oldtype -> return_type != NULL)
        typet_free_typetree(oldtype -> return_type);
    else if(oldtype -> next_parm != NULL)
        typet_free_typetree(oldtype -> next_parm);
    if(oldtype -> base_type != NULL && oldtype -> base_type != 1)//**************************
        typet_free_typetree(oldtype -> base_type);
    else if(oldtype -> parm_list != NULL && oldtype -> parm_list != 1)//***********************
        typet_free_typetree(oldtype -> parm_list);
	free(oldtype);
    return;
}
