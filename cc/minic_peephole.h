#ifndef __MINIC_PEEPHOLE_H__
#define __MINIC_PEEPHOLE_H__
#include "minic_machinecode.h"
#include <stdio.h>

extern struct mach_code_table * code_table_list;

extern void peephole(int func_index, FILE * out_buf);

#endif


