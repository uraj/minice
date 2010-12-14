#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <memory/cache.h>

#define BLOCKLEN 8
#define SETLEN 8
#define TABLEN 16

//instrution cache and data cache??

static enum CacheSwapStrategy swap_strategy;
static enum CacheWriteStrategy write_strategy;
static struct Cacheinfo cache_info;

static void rand_cache_overflow(unsigned int tag, unsigned int set, unsigned int block)//Rand arith
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
	if(Cache[set][rand_line].dirty == 1)
    write_back();//in our emu, we need not that
	*/

	Cache[set][rand_line].valid = 1;
	Cache[set][rand_line].tag = tag;

	if(write_strategy == Write_back)//Set when strategy Write_back
		Cache[set][rand_line].dirty = 0;
}

static void fifo_cache_overflow(unsigned int tag, unsigned int set, unsigned int block)//Fifo arith
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

	for(line = 0; line < LINENUM; line ++)//update other line 
	{
		if(line != selected_line)
			Cache[set][line].counter ++;
	}

	Cache[set][selected_line].valid = 1;
	Cache[set][selected_line].counter = 0;
	Cache[set][selected_line].tag = tag;

	if(write_strategy == Write_back)//Set when strategy Write_back
		Cache[set][selected_line].dirty = 0;
}

static void lru_cache_overflow(unsigned int tag, unsigned int set, unsigned int block)
{
	int line;
	for(line = 0; line < LINENUM; line++)//find empty
	{
		if(Cache[set][line].valid == 0)
		{
			Cache[set][line].valid = 1;
			Cache[set][line].counter = 0;
			int update_line;//update other counter as fifo
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
	for(line = 0; line < LINENUM; line ++)//set the accessed line with min number 0
	{
		if(Cache[set][line].counter < Cache[set][selected_line].counter)
			Cache[set][line].counter ++;
	}

	Cache[set][selected_line].valid = 1;
	Cache[set][selected_line].counter = 0;
	Cache[set][selected_line].tag = tag;

	if(write_strategy == Write_back)//Set when strategy Write_back
		Cache[set][selected_line].dirty = 0;
}

static inline void cache_overflow(unsigned int tag, unsigned int set, unsigned int block)
{
	switch(swap_strategy)
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

void init_cache(enum CacheSwapStrategy swap, enum CacheWriteStrategy write)
{
	swap_strategy = swap;
	write_strategy = write;
	cache_info.miss = 0;
	cache_info.hit = 0;
	cache_info.save_write_time = 0;
	int set, line;
	for(set = 0; set < SETNUM; set ++)
	{
		for(line = 0; line < LINENUM; line ++)
		{
			Cache[set][line].valid = 0;
			Cache[set][line].counter = 0;
			Cache[set][line].dirty = 0;
		}
	}
	if(swap_strategy == RAND)
		srand((unsigned)time(NULL));
}

//clear cache?
//clear cacheinfo?

inline struct Cacheinfo * get_cache_info()
{
	return &cache_info;
}

int cache_write(uint32_t addr, uint32_t data)
{
	unsigned int tag = addr >> (BLOCKLEN + SETLEN);
	unsigned int set = (addr << TABLEN) >> (TABLEN + BLOCKLEN);
	unsigned int block = (addr << (TABLEN + SETLEN)) >> (TABLEN + SETLEN);
	if(write_strategy == Write_through)//Write_through
	{
		vmem_write(addr, data);
		return -1;
	}
	int line;
	for(line = 0; line < LINENUM; line ++)
	{
		if(Cache[set][line].valid == 1 && Cache[set][line].tag == tag)
		{
			cache_info.hit ++;
			cache_info.save_write_time ++;
			vmem_write(addr, data);//in fact, this should be written to cache, in emu we only care the statistic data
			return 0;
		}
	}
	
	cache_info.miss ++;
	cache_overflow(tag, set, block);
	vmem_write(addr, data);//in fact this should be written to cache
	return 1;
}

int cache_read(uint32_t addr, uint32_t * dest)
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
			return 0;
		}
	}
	cache_info.miss ++;
	cache_overflow(tag, set, block);
	vmem_read(addr, dest);
	return 1;
}
