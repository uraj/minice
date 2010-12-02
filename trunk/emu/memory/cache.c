#include "vmem.h"
#include "cache.h"

#define BLOCKLEN 8
#define SETLEN 8
#define TABLEN 16

//instrution cache and data cache??

static enum CacheSwapStrategy swap_strategy;
static enum WriteStrategy write_strategy;
static struct Cacheinfo cache_info;

void init_cache()
{
	swap_strategy = FIFO;
	write_strategy = Write_back;
	cache_info.cache_miss = 0;
	cache_info.cache_hit = 0;
	for(int set = 0; set < SETNUM; set ++)
	{
		for(int line = 0; line < LINENUM; line ++)
		{
			Cache[set][line].valid = 0;
			Cache[set][line].counter = 0;
		}
	}
}

//clear cache?
//clear cacheinfo?

inline void set_cache_swap_strategy(enum CacheSwapStrategy strategy)
{
	write_strategy = strategy;
}

inline void set_cache_write_strategy(enum CacheWriteStrategy strategy)
{
	swap_strategy = strategy;
}

inline struct Cacheinfo * get_cache_info()
{
	return &cache_info;
}

void cache_write(uint32_t addr, uint32_t data)
{
	unsigned int tag = addr >> (BLOCKLEN + SETLEN);
	unsigned int set = (addr << TABLEN) >> (TABLEN + BLOCKLEN);
	unsigned int block = (addr << (TABLEN + SETLEN)) >> (TABLEN + SETLEN);}

void cache_read(uint32_t addr, uint32_t * dest)
{
	unsigned int tag = addr >> (BLOCKLEN + SETLEN);
	unsigned int set = (addr << TABLEN) >> (TABLEN + BLOCKLEN);
	unsigned int block = (addr << (TABLEN + SETLEN)) >> (TABLEN + SETLEN);
	int line;
	for(line = 0; line < LINENUM; line++)
	{
		if(Cache[set][line].valid == 1 && Cache[set][line].tag == tag)
		{
			cache_info.hit ++;
			vmem_read(addr, dest);
			return;
		}
	}
	cache_info.miss ++;
	cache_overflow(int tag, int set, int block);
	vmem_read(addr, dest);
	return;
}

static inline void cache_overflow(int tag, int set, int block)
{
	switch(strategy)
	{
		case RAND:
			rand_cache_overflow(tag, set, block);
			return;
		case FIFO:
			fifo_cache_overflow(tag, set, block);
			return;
		case LRU:
			lru_cache_overflow(tag, set, block);
			return;
		default:
			break;
	}
}

static void rand_cache_overflow

static void fifo_cache_overflow(int tag, int set, int block)//Fifo arith
{
	int line, found_flag, max, selected_line;
	found_flag = 0;
	for(line = 0; line < LINENUM; line++)//find empty
	{
		if(found_flag == 0)
		{
			if(Cache[set][line].valid == 0)
			{
				Cache[set][line].valid = 1;
				Cache[set][line].counter = 0;
				found_flag = 1;
			}
			else Cache[set][line].counter ++;
		}
		else if(Cache[set][line].valid == 1)
			Cache[set][line].counter ++;
	}
	if(found_flag == 1)
		return;
	max = 0;
	selected_line = 0;

	for(line = 0; line < LINENUM; line ++)//find max
	{
		if(Cache[set][line].counter > max)
		{
			max = Cache[set][line].counter;
			selected_line = line;
		}
	}

	Cache[set][selected_line].valid = 1;
	Cache[set][selected_line].counter = 0;
	Cache[set][selected_line].tag = tag;

	for(line = 0; line < LINENUM; line ++)//update other line 
	{
		if(line != selected_line)
			Cache[set][line].counter ++;
	}
} 
