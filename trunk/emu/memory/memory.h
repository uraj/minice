#ifndef __MINIEMU_MEMORY_H__
#define __MINIEMU_MEMORY_H__

#include <memory/vmem.h>
#include <stdint.h>
#include <stddef.h>

extern int mem_read(uint32_t addr, size_t size, int sign_ext, uint32_t * data);

extern int mem_write(uint32_t addr, size_t size, uint32_t data);

#endif
