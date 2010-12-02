#ifndef __MINIEMU_CACHE_H__
#define __MINIEMU_CACHE_H__

#include "vmem.h"

/* capacity = 32KB blocksize = 64B linenum = 8 */
/* if we want to build up multilevel caches, we can just use this */

#define LINENUM 0x8		/*8*/
#define SETNUM 0x40		/*64*/
#define BLOCKSIZE 0x40	/*64*/

enum { RAND, FIFO, LRU } CacheSwapStrategy;
enum { Write_back, Write_through } CacheWriteStrategy;

//Write-through + No-write-allocate
//Write-back + Write-allocate

struct Cacheline
{
	char valid;//1 bit
	//When design LRU, may need more flags
	char counter;//3 bit	
	unsigned int tag;
};

struct Cacheinfo
{
	int miss;
	int hit;
	//may need more
};

extern struct Cacheline Cache[SETNUM][LINENUM];//global or not?
/*cacheinfo global or not?*/

extern void init_cache();
extern void set_cache_swap_strategy(enum CacheSwapStrategy strategy);
extern void set_cache_write_strategy(enum CacheWriteStrategy strategy);

extern struct Cacheinfo * get_cache_info();

extern void cache_write(uint32_t addr, uint32_t data);
extern void cache_read(uint32_t addr, uint32_t * dest);

#endif


