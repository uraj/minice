#ifndef __MINISIM_CONTROL_H__
#define __MINISIM_CONTROL_H__

#include "globdefs.h"

/* generate constrol signals of the pipeline at ID stage */
extern void gen_control_signals(const InstrFields * ifields, InstrType itype, EX_input * ex_in);

extern int test_cond(const PSW * cmsr, uint8_t condcode);

#endif
