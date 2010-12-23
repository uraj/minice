#ifndef __MINISIM_MEMORY_H__
#define __MINISIM_MEMORY_H__

#include <stdint.h>
#include <stddef.h>
#include "cache.h"
#include "vmem.h"

extern int * gp_cachemiss;
extern int * gp_cachewb;

extern int mem_read_instruction(uint32_t addr, uint32_t * instr);

extern int mem_read(uint32_t addr, size_t size, int sign_ext, uint32_t * data);

extern int mem_write(uint32_t addr, size_t size, uint32_t data);

#endif
