#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <getopt.h>
#include <unistd.h>
#include <setjmp.h>

#include "loader.h"
#include "pipeline.h"
#include "console.h"
#include "memory/memory.h"

typedef struct
{
    int instr_count;
    int cycle_count;
    int bubble_count;
    int cache_miss;
    int mem_access;
    int cache_wb;
} StatInfo;

/* used by memory.h */
L2PT L1PageTable[L1PTSIZE];

/* used by cache.h */
Cacheline Cache[2][SETNUM][LINENUM];
enum CacheSwapStrategy swap_strategy;
enum CacheWriteStrategy write_strategy;

/* used by debug.h */
const RegFile * gp_reg = NULL;
const PipeState * gp_pipe = NULL;

/* used by memory.h */
int * gp_cachemiss = NULL;
int * gp_cachewb = NULL;
int * gp_memaccess = NULL;

/* special functions */
/* #define PRINT_INT 0 */
/* #define PRINT_CHAR 1 */
/* #define PRINT_STRING 2 */
/* #define PRINTLINE_INT 3 */
/* #define MAIN 4 */

/* used by console() in console.c */
jmp_buf beginning;


 /* important */
void simulate_init(RegFile * storage, PipeState * pipe_state, StatInfo * stat_info)
{
    gp_reg = storage;
    gp_pipe = pipe_state;
    gp_cachemiss = &(stat_info->cache_miss);
    gp_cachewb = &(stat_info->cache_wb);
    gp_memaccess = &(stat_info->mem_access);
    
    /* init cache */
    init_cache(LRU, Write_back);
    
    /* init statistic information */
    memset(stat_info, 0, sizeof(StatInfo));
    
    /* init reg file */
    memset(storage->reg, 0, 32 * sizeof(uint32_t));
    storage->reg[SP] = 0xf0000000;
    storage->reg[FP] = 0xf0000000;
    storage->CMSR.N = 0;
    storage->CMSR.Z = 0;
    storage->CMSR.C = 0;
    storage->CMSR.V = 0;
    
    /* init pipeline */
    pipe_state->id_in.bubble = 1;
    pipe_state->ex_in.bubble = 1;
    pipe_state->mem_in.bubble = 1;
    pipe_state->wb_in.bubble = 1;
    
    pipe_state->id_in.sinfo.icode = 0;
    pipe_state->ex_in.sinfo.icode = 0;
    pipe_state->mem_in.sinfo.icode = 0;
    pipe_state->wb_in.sinfo.icode = 0;
    
    pipe_state->id_in.sinfo.pc = 0;
    pipe_state->ex_in.sinfo.pc = 0;
    pipe_state->mem_in.sinfo.pc = 0;
    pipe_state->wb_in.sinfo.pc = 0;
    
    /* data forwarding related */
    pipe_state->ex_fwd[0].freg = 0xff;
    pipe_state->ex_fwd[1].freg = 0xff;
    pipe_state->mem_fwd.freg = 0xff;
    return;
}

StatInfo simulate(uint32_t * func_entry)
{
    StatInfo stat_info;
    RegFile storage;
    PipeState pipe_state;
    
    /* initialization */
    simulate_init(&storage, &pipe_state, &stat_info);
    storage.reg[PC] = func_entry[MAIN];
    
    while(1)
    {
        ++stat_info.cycle_count;
        WBStage(&storage, &pipe_state);
        stat_info.bubble_count += pipe_state.wb_in.bubble;  
        MEMStage(&storage, &pipe_state);
        EXStage(&storage, &pipe_state);
//        stat_info.cycle_count += EXStage(&storage, &pipe_state);
        if(IDStage(&storage, &pipe_state) == 1)
        {
            pipe_state.ex_in.bubble = 1;
            continue;           /* stalling */
        }
        if(IFStage(&storage, &pipe_state, func_entry) == -1)
            break;
        ++stat_info.instr_count;
    }
    return stat_info;
}

StatInfo simulate_db(uint32_t * func_entry)
{
    setjmp(beginning);
    
    StatInfo stat_info;
    RegFile storage;
    PipeState pipe_state;

    simulate_init(&storage, &pipe_state, &stat_info);
    storage.reg[PC] = func_entry[MAIN];
    
    while(1)
    {
        console();
        
        ++stat_info.cycle_count;

        WBStage(&storage, &pipe_state);
        stat_info.bubble_count += pipe_state.wb_in.bubble;
        
        MEMStage(&storage, &pipe_state);
        pipe_state.wb_in.sinfo = pipe_state.mem_in.sinfo;

        stat_info.cycle_count += EXStage(&storage, &pipe_state);
        pipe_state.mem_in.sinfo = pipe_state.ex_in.sinfo;        

        if(IDStage(&storage, &pipe_state) == 1)
        {
            pipe_state.ex_in.bubble = 1;
            continue;           /* stalling */
        }
        pipe_state.ex_in.sinfo = pipe_state.id_in.sinfo;
        if(IFStage(&storage, &pipe_state, func_entry) == -1)
            break;
        ++stat_info.instr_count;
     
    }
    return stat_info;
}

void print_stat_info(StatInfo * stat_info)
{
    stat_info->cycle_count += 9 * stat_info->cache_miss;
    fprintf(stderr, "Simulation Profile:\n\tclock-cycle count: %d\n", stat_info->cycle_count);
    fprintf(stderr, "\tinstrcutin count: %d\n", stat_info->instr_count);
    fprintf(stderr, "\tbubble count: %d\n", stat_info->bubble_count);
    fprintf(stderr, "\tmemory access: %d\n", stat_info->mem_access);
    fprintf(stderr, "\tcache miss: %d\n", stat_info->cache_miss);
    fprintf(stderr, "\tcache writeback: %d\n", stat_info->cache_wb);
    return;
}

inline void usage()
{
    fprintf(stderr, "Usage: minisim [-s] [-d] [-t] -f FILE\n");
    return;
}

int main(int argc, char * argv[])
{
    char option = 0, * filename = NULL;
    int db_option = 0, stat_option = 0, file_option = 0;
    while((option = getopt(argc, argv, "f:dsh")) != -1)
    {
        switch(option)
        {
            case 'f':
                file_option = 1;
                filename = optarg;
                break;
            case 'd':
                db_option = 1;
                break;
            case 's':
                stat_option = 1;
                break;
            case 'h':
                usage();
                return 0;
            default:
                usage();
                return 1;
        }
    }
    if(file_option == 0)
    {
        usage();
        return 1;
    }
    FILE * elf = fopen(filename, "rb");
    
    if(elf == NULL)
    {
        fprintf(stderr, "miniemu: %s: file open fails.\n", argv[1]);
        return 1;
    }

    if(elf_check(elf) == 1)
    {
        fprintf(stderr, "miniemu: %s: not a valid ELF file.\n", argv[1]);
        return 1;
    }
    
    memset(L1PageTable, 0, sizeof(L1PageTable));    
    Elf32_Ehdr ehdr = get_elf_hdr(elf);
    load_elf_segments(elf, ehdr);
    
    uint32_t func_entry[7];
    func_entry[PRINT_INT] = get_func_entry(elf, ehdr, "print_int");
    func_entry[PRINT_CHAR] = get_func_entry(elf, ehdr, "print_char");
    func_entry[PRINT_STRING] = get_func_entry(elf, ehdr, "print_string");
    func_entry[PRINTLINE_INT] = get_func_entry(elf, ehdr, "printline_int");
    func_entry[PRINTLINE_CHAR] = get_func_entry(elf, ehdr, "printline_char");
    func_entry[MAIN] = get_func_entry(elf, ehdr, "main");
    
    fclose(elf);
    
    StatInfo stat_info;
    
    if(db_option)
        stat_info = simulate_db(func_entry);
    else
        stat_info = simulate(func_entry);
    
    if(stat_option)
        print_stat_info(&stat_info);
    
    mem_free();
    
    return 0;
}
