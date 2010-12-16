#ifndef __MINIEMU_DEBUG_H__
#define __MINIEMU_DEBUG_H__

#include <pipeline.h>

extern const RegFile * gp_reg;
extern const PipeState * pg_pipe;

extern void pregs();
extern void pstack(uint32_t addr);
extern void pstack_range(uint32_t addr_b, uint32_t addr_e);
extern void print_stage_info(const StageInfo * sinfo);

#endif
