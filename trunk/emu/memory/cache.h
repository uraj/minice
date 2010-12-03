#ifndef __MINIEMU_CACHE_H__
#define __MINIEMU_CACHE_H__

#include "vmem.h"

/* capacity = 32KB blocksize = 64B linenum = 8 */
/* if we want to build up multilevel caches, we can just use this */

#define LINENUM 0x8		/*8*/
#define SETNUM 0x40		/*64*/
#define BLOCKSIZE 0x40	/*64*/

enum CacheSwapStrategy { RAND, FIFO, LRU };
enum CacheWriteStrategy { Write_back, Write_through };

//Write-through + No-write-allocate
//Write-back + Write-allocate

struct Cacheline
{
	char dirty;//1 bit for Write-back + Write_allocate
	char valid;//1 bit
	char counter;//3 bit, used by fifo and lru	
	unsigned int tag;
};

struct Cacheinfo
{
	int miss;
	int hit;
	int save_write_time;//statistic for Write_back strategy
	//may need more
};

extern struct Cacheline Cache[SETNUM][LINENUM];//global or not?
/*cacheinfo global or not?*/

extern void init_cache();

extern struct Cacheinfo * get_cache_info();

extern void cache_write(uint32_t addr, uint32_t data);
extern void cache_read(uint32_t addr, uint32_t * dest);

#endif


