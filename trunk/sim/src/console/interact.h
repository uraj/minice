#ifndef __MINISIM_INTERACT_H__
#define __MINISIM_INTERACT_H__

#include <pipeline.h>

extern const RegFile * gp_reg;
extern const PipeState * gp_pipe;

extern void pregs();
extern void ppipe();
extern void pstack(uint32_t addr);
extern void pstack_range(uint32_t addr_b, uint32_t addr_e);
extern void pfwd();

extern void console();

#endif
