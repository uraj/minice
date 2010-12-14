#ifndef __MINIEMU_DEBUG_H__
#define __MINIEMU_DEBUG_H__

#include <pipeline.h>

extern const RegFile * gp_reg;
extern const PipeState * pg_pipe;

extern void print_regfile();
extern void print_stage_info(const StageInfo * sinfo);
extern void print_mem_word(uint32_t vaddr);
extern void print_mem_halfword(uint32_t vaddr);
extern void print_mem_byte(uint32_t vaddr);

#endif
