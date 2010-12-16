#ifndef __MINIEMU_DEBUG_H__
#define __MINIEMU_DEBUG_H__

#include <pipeline.h>

extern const RegFile * gp_reg;
extern const PipeState * gp_pipe;

extern void pregs();
extern void print_stage_info();
extern void pstack(uint32_t addr);
extern void pstack_range(uint32_t addr_b, uint32_t addr_e);

#endif
