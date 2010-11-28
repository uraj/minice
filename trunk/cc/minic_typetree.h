#ifndef __MINIC_TYPETREE_H__
#define __MINIC_TYPETREE_H__

#include "minic_typedef.h"

struct typetree
{
	enum data_type type;
	union
	{
		struct typetree * return_type;
		struct typetree * next_parm;
		int size;
	};
	union
	{
	    struct typetree * base_type;
	    struct typetree * parm_list;
	};
};

extern struct typetree * typet_new_type(enum data_type newtype);
extern int typet_type_equal(struct typetree * t1, struct typetree * t2);
extern void typet_free_typetree(struct typetree * oldtype);
extern struct typetree * typet_typetree_dup(const struct typetree * src);
#endif
