#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "cache.h"

#define BLOCKLEN 6
#define SETLEN 6
#define TAGLEN 20

//instrution cache and data cache??

static void rand_cache_overflow(int id_sel, unsigned int tag, unsigned int set, unsigned int block)//Rand arith
{
	int line;
	for(line = 0; line < LINENUM; line++)//find empty
	{
		if(Cache[id_sel][set][line].valid == 0)
		{
			Cache[id_sel][set][line].valid = 1;
			return;
		}
	}
	
	int rand_line = rand() % LINENUM;

	Cache[id_sel][set][rand_line].valid = 1;
	Cache[id_sel][set][rand_line].tag = tag;

	if(write_strategy == Write_back)//Set when strategy Write_back
		Cache[id_sel][set][rand_line].dirty = 0;
}

static void fifo_cache_overflow(int id_sel, unsigned int tag, unsigned int set, unsigned int block)//Fifo arith
{
	int line;
	for(line = 0; line < LINENUM; line++)//find empty
	{
		if(Cache[id_sel][set][line].valid == 0)
		{
			Cache[id_sel][set][line].valid = 1;
			Cache[id_sel][set][line].counter = 0;
			int update_line;//update other counter
			for(update_line = 0; update_line < LINENUM; update_line ++)
			{
				if(Cache[id_sel][set][update_line].valid == 1 && update_line != line)
					Cache[id_sel][set][update_line].counter ++;
			}
			return;
		}
	}

	int max, selected_line;
	max = 0;
	selected_line = 0;

	for(line = 0; line < LINENUM; line ++)//find max
	{
		if(Cache[id_sel][set][line].counter > max)
		{
			max = Cache[id_sel][set][line].counter;
			selected_line = line;
		}
	}

	for(line = 0; line < LINENUM; line ++)//update other line 
	{
		if(line != selected_line)
			Cache[id_sel][set][line].counter ++;
	}

	Cache[id_sel][set][selected_line].valid = 1;
	Cache[id_sel][set][selected_line].counter = 0;
	Cache[id_sel][set][selected_line].tag = tag;

	if(write_strategy == Write_back)//Set when strategy Write_back
		Cache[id_sel][set][selected_line].dirty = 0;
}

static void lru_cache_overflow(int id_sel, unsigned int tag, unsigned int set, unsigned int block)
{
	int line;
	for(line = 0; line < LINENUM; line++)//find empty
	{
		if(Cache[id_sel][set][line].valid == 0)
		{
			Cache[id_sel][set][line].valid = 1;
			Cache[id_sel][set][line].counter = 0;
			int update_line;//update other counter as fifo
			for(update_line = 0; update_line < LINENUM; update_line++)
			{
				if(Cache[id_sel][set][update_line].valid == 1 && update_line != line)
					Cache[id_sel][set][update_line].counter ++;
			}
			return;
		}
	}

	int max, selected_line;
	max = 0;
	selected_line = 0;

	for(line = 0; line < LINENUM; line ++)//find max
	{
		if(Cache[id_sel][set][line].counter > max)
		{
			max = Cache[id_sel][set][line].counter;
			selected_line = line;
		}
	}

	for(line = 0; line < LINENUM; line ++)//set the accessed line with min number 0
	{
		if(Cache[id_sel][set][line].counter < Cache[id_sel][set][selected_line].counter)
			Cache[id_sel][set][line].counter ++;
	}

	Cache[id_sel][set][selected_line].valid = 1;
	Cache[id_sel][set][selected_line].counter = 0;
	Cache[id_sel][set][selected_line].tag = tag;

	if(write_strategy == Write_back)//Set when strategy Write_back
		Cache[id_sel][set][selected_line].dirty = 0;
}

static inline void cache_overflow(int id_sel, unsigned int tag, unsigned int set, unsigned int block)
{
	switch(swap_strategy)
	{
		case RAND:
			rand_cache_overflow(id_sel, tag, set, block);
			return;
		case FIFO:
			fifo_cache_overflow(id_sel, tag, set, block);
			return;
		case LRU:
			lru_cache_overflow(id_sel, tag, set, block);
			return;
		default:
			break;
	}
}

void init_cache(enum CacheSwapStrategy swap, enum CacheWriteStrategy write)
{
	swap_strategy = swap;
	write_strategy = write;
	int set, line;
    
	for(set = 0; set < SETNUM; set ++)
	{
		for(line = 0; line < LINENUM; line ++)
		{
			Cache[0][set][line].valid = 0;
			Cache[0][set][line].counter = 0;
			Cache[0][set][line].dirty = 0;
		}
	}
    
	for(set = 0; set < SETNUM; set ++)
	{
		for(line = 0; line < LINENUM; line ++)
		{
			Cache[1][set][line].valid = 0;
			Cache[1][set][line].counter = 0;
			Cache[1][set][line].dirty = 0;
		}
	}
    
	if(swap_strategy == RAND)
		srand((unsigned)time(NULL));
}

int cache_write(int id_sel, uint32_t addr)
{
	unsigned int tag = addr >> (BLOCKLEN + SETLEN);
	unsigned int set = (addr << TAGLEN) >> (TAGLEN + BLOCKLEN);
	unsigned int block = (addr << (TAGLEN + SETLEN)) >> (TAGLEN + SETLEN);
    int line;
    
	if(write_strategy == Write_through)//Write_through
		return -1;
	
	for(line = 0; line < LINENUM; line ++)
	{
		if(Cache[id_sel][set][line].valid == 1 && Cache[id_sel][set][line].tag == tag)
			return 0;
	}
	cache_overflow(id_sel, tag, set, block);
	return 1;
}

int cache_read(int id_sel, uint32_t addr)
{
	unsigned int tag = addr >> (BLOCKLEN + SETLEN);
	unsigned int set = (addr << TAGLEN) >> (TAGLEN + BLOCKLEN);
	unsigned int block = (addr << (TAGLEN + SETLEN)) >> (TAGLEN + SETLEN);
	int line;
    
	for(line = 0; line < LINENUM; line++)
	{
		if(Cache[id_sel][set][line].valid == 1 && Cache[id_sel][set][line].tag == tag)
			return 0;
	}
	cache_overflow(id_sel, tag, set, block);
	return 1;
}
