#ifndef __MINIC_VARMAPPING_H__
#define __MINIC_VARMAPPING_H__

#include "minic_symtable.h"
#include "minic_triargexpr.h"
#include "minic_flowanalyse.h"

/* this is for mapping the operands of triargexpr to a integer, which */
/* will be useful during data flow analyze and register allocation */
/* [char * idname, int exprindex] <=> var_info_index <=> index */
/* At last, realize that we can find index with var info and var info with index */
struct var_info//the struct to contain var's flags
{
	/* some flags */
	int is_define;
	int is_use;
	
	int reg_addr;
	int mem_addr;/* can be union with is_define and is use */

	struct var_list * ref_point;//only for array

	int tag_num;// the expr jump to which tag /* can be union */

	int index;
};

extern struct symbol_table *simb_table; 
extern struct triargtable **table_list;
extern int g_table_list_size;

extern int get_index_of_temp(int expr);
extern int get_index_of_id(char * idname);

extern struct var_info * get_info_of_temp(int expr);
extern struct var_info * get_info_of_id(char * idname);

extern struct var_info * get_info_from_index(int index);
extern int get_ref_var_num();/*return total number of vars which have been refered*/

extern int is_global(int index);

extern int new_var_map(int func_index);
extern void free_var_map();

extern int insert_tempvar(int exprindex);

#endif
