#ifndef __MINISIM_CACHE_H__
#define __MINISIM_CACHE_H__

#include "vmem.h"

/* capacity = 32KB blocksize = 64B linenum = 8 */
/* if we want to build up multilevel caches, we can just use this */

#define LINENUM 0x8		/* 8  */
#define SETNUM 0x40		/* 64 */
#define BLOCKSIZE 0x40	/* 64 */
#define ICACHE 0
#define DCACHE 1

enum CacheSwapStrategy { RAND, FIFO, LRU };
enum CacheWriteStrategy { Write_back, Write_through };

extern enum CacheSwapStrategy swap_strategy;
extern enum CacheWriteStrategy write_strategy;

/* Write-through + No-write-allocate */
/* Write-back + Write-allocate */

typedef struct
{
	uint8_t dirty;//1 bit for Write-back + Write_allocate
	uint8_t valid;//1 bit
	uint8_t counter;//3 bit, used by fifo and lru	
	uint32_t tag;
} Cacheline;

extern Cacheline Cache[2][SETNUM][LINENUM];

extern void init_cache(enum CacheSwapStrategy swap, enum CacheWriteStrategy write);

extern int cache_write(int id_sel, uint32_t addr);/* miss: 1		hit: 0		write_through: -1 */
extern int cache_read(int id_sel, uint32_t addr);/* miss: 1	hit: 0	*/

#endif
