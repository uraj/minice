#ifndef __MINIEMU_VMEM_H__
#define __MINIEMU_VMEM_H__

#include <stdint.h>
#include <stddef.h>

#define VMEMSPACE_SIZE 0x100000
#define VMEMPAGE_SIZE 0x1000
#define L1PTSIZE 0x1000
#define L2PTSIZE 0x1000

typedef void * PageAddr;

typedef enum {NAPage = 0, RPage, XPage, RWPage} PageAttr;
    
typedef struct
{
    PageAttr flag;
    PageAddr pageref;
} PTelem;

typedef PTelem * L2PT;

extern L2PT L1PageTable[L1PTSIZE];

extern void mem_load(uint32_t addr, size_t size, const uint32_t * source, PageAttr flag);

/* 0: success
 * 1: mem readonly
 */
extern int mem_write_direct_b(uint32_t addr, uint8_t data);
extern int mem_write_direct_h(uint32_t addr, uint16_t data);
extern int mem_write_direct_w(uint32_t addr, uint32_t data);

/* 0: success
 * 1: mem not allocated
 * 2: mem unreadable
 */
extern int mem_read_direct_b(uint32_t addr, uint8_t * dest);
extern int mem_read_direct_h(uint32_t addr, uint16_t * dest);
extern int mem_read_direct_w(uint32_t addr, uint32_t * dest);

extern void mem_free();

#endif
