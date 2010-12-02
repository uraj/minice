#ifndef __MINIEMU_CONTROL_H__
#define __MINIEMU_CONTROL_H__

#include <pipeline/globdefs.h>

/* generate constrol signals of the pipeline at ID stage */
extern void gen_control_signals(const InstrFields * ifields, InstrType itype, EX_input * ex_in);

extern int data_hazard_detect(uint8_t read_src, const PipeState * pipe_state);

#endif
