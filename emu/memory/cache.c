#include <stdio.h>
#include <stdlib.h>
#include <time.h>
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
			Cache[set][line].dirty = 0;
		}
	}
}

//clear cache?
//clear cacheinfo?

inline void set_cache_swap_strategy(enum CacheSwapStrategy strategy)//be careful, this will init the cache
{
	init_cache();
	write_strategy = strategy;
	switch(stragegy)
	{
		case RAND:
			srand((unsigned)time(NULL));
			break;
		default:
			break;
	}
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

static void rand_cache_overflow(int tag, int set, int block)//Rand arith
{
	int line;
	for(line = 0; line < LINENUM; line++)//find empty
	{
		if(Cache[set][line].valid == 0)
		{
			Cache[set][line].valid = 1;
			return;
		}
	}
	
	int rand_line = rand() % LINENUM;

	/*
	//Write_back
	if(Cache[set][selected_line].dirty == 1)
		write_back();//in our emu, we need not that
	*/

	Cache[set][selected_line].valid = 1;
	Cache[set][selected_line].tag = tag;

	if(write_strategy == Write_back)//Set when strategy Write_back
		Cache[set][selected_line].dirty = 0;
}

static void fifo_cache_overflow(int tag, int set, int block)//Fifo arith
{
	int line;
	for(line = 0; line < LINENUM; line++)//find empty
	{
		if(Cache[set][line].valid == 0)
		{
			Cache[set][line].valid = 1;
			Cache[set][line].counter = 0;
			int update_line;//update other counter
			for(update_line = 0; update_line < LINENUM; update_line ++)
			{
				if(Cache[set][update_line].valid == 1 && update_line != line)
					Cache[set][update_line].counter ++;
			}
			return;
		}
	}

	int max, selected_line;
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

	/*
	//Write_back
	if(Cache[set][selected_line].dirty == 1)
		write_back();//in our emu, we need not that
	*/

	Cache[set][selected_line].valid = 1;
	Cache[set][selected_line].counter = 0;
	Cache[set][selected_line].tag = tag;

	if(write_strategy == Write_back)//Set when strategy Write_back
		Cache[set][selected_line].dirty = 0;

	for(line = 0; line < LINENUM; line ++)//update other line 
	{
		if(line != selected_line)
			Cache[set][line].counter ++;
	}
}

static void lru_cache_overflow(int tag, int set, int tag)
{

}
