#ifndef __MINIC_ASMOUT_H__
#define __MINIC_ASMOUT_H__

#include <stdio.h>
#include "minic_machinecode.h"

extern void mcode_out(const struct mach_code * mcode, FILE * out_buff);

#endif
