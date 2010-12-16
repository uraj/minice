#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <getopt.h>
#include <unistd.h>
#include <setjmp.h>

#include <loader/elfmanip.h>
#include <memory/memory.h>
#include <pipeline.h>
#include <debug.h>
#include <console/console.h>

/* used by memory.h */
L2PT L1PageTable[L1PTSIZE];

/* used by debug.h */
const RegFile * gp_reg = NULL;
const PipeState * gp_pipe = NULL;

/* used by console() in console.c */
jmp_buf beginning;

typedef struct
{
    int instr_count;
    int cycle_count;
    int bubble_count;
    int mem_read;
    int cache_miss;
    int mem_write;
    int cache_wb;
} StatInfo;

void simulate_init(RegFile * storage, PipeState * pipe_state) /* important */
{
    memset(storage->reg, 0, 32 * sizeof(uint32_t));
    storage->reg[SP] = 0xf0000000;
    storage->reg[FP] = 0xf0000000;
    storage->CMSR.N = 0;
    storage->CMSR.Z = 0;
    storage->CMSR.C = 0;
    storage->CMSR.V = 0;

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
    pipe_state->id_in.ex_fwd[0].freg = 0xff;
    pipe_state->id_in.ex_fwd[1].freg = 0xff;
    pipe_state->id_in.mem_fwd.freg = 0xff;
    return;
}

StatInfo simulate(uint32_t simulation_entry, uint32_t special_entry)
{
    setjmp(beginning);
    
    StatInfo stat_info;
    stat_info.cycle_count = 0;
    stat_info.instr_count = 0;
    stat_info.bubble_count = 0;
    
    RegFile storage;
    PipeState pipe_state;
    gp_reg = &storage;
    gp_pipe = &pipe_state;
    
    /* initialization */
    simulate_init(&storage, &pipe_state);
    storage.reg[PC] = simulation_entry;
    
    while(1)
    {
        ++stat_info.cycle_count;
        WBStage(&storage, &pipe_state);
        stat_info.bubble_count += pipe_state.wb_in.bubble;  
        MEMStage(&storage, &pipe_state);
        EXStage(&storage, &pipe_state);
        if(IDStage(&storage, &pipe_state) == -1)
        {
            pipe_state.ex_in.bubble = 1;
            continue;           /* stalling */
        }
        if(IFStage(&storage, &pipe_state, special_entry) == -1)
            break;
        ++stat_info.instr_count;
    }
    return stat_info;
}

StatInfo simulate_db(uint32_t simulation_entry, uint32_t special_entry)
{
    setjmp(beginning);
    
    StatInfo stat_info;
    RegFile storage;
    PipeState pipe_state;
    gp_reg = &storage;
    gp_pipe = &pipe_state;
    
    /* initialization */
    stat_info.cycle_count = 0;
    stat_info.instr_count = 0;
    stat_info.bubble_count = 0;

    simulate_init(&storage, &pipe_state);
    storage.reg[PC] = simulation_entry;
    
    while(1)
    {
        //       console();
        
        ++stat_info.cycle_count;

        WBStage(&storage, &pipe_state);
        stat_info.bubble_count += pipe_state.wb_in.bubble;
        
        printf("Stage MEM: <0x%08x> 0x%08x\n", pipe_state.mem_in.sinfo.pc, pipe_state.mem_in.sinfo.icode);
        
        MEMStage(&storage, &pipe_state);
        pipe_state.wb_in.sinfo = pipe_state.mem_in.sinfo;

        EXStage(&storage, &pipe_state);
        pipe_state.mem_in.sinfo = pipe_state.ex_in.sinfo;        

        if(IDStage(&storage, &pipe_state) == -1)
        {
            pipe_state.ex_in.bubble = 1;
            continue;           /* stalling */
        }
        pipe_state.ex_in.sinfo = pipe_state.id_in.sinfo;
        if(IFStage(&storage, &pipe_state, special_entry) == -1)
            break;
        ++stat_info.instr_count;
        pipe_state.id_in.sinfo.icode = pipe_state.id_in.instruction;
        pipe_state.id_in.sinfo.pc = storage.reg[PC] - 4;
    }
    return stat_info;
}

void print_stat_info(const StatInfo * stat_info)
{
    fprintf(stderr, "Simulation Profile:\n\tclock-cycle count: %d\n", stat_info->cycle_count);
    fprintf(stderr, "\tinstrcutin count: %d\n", stat_info->instr_count);
    fprintf(stderr, "\tbubble count: %d\n", stat_info->bubble_count);
    return;
}

inline void usage()
{
    fprintf(stderr, "Usage: minisim [-d] [-t] -f FILE\n");
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
    
    Elf32_Ehdr ehdr = get_elf_hdr(elf);
    load_elf_segments(elf, ehdr);
    uint32_t special_entry = get_func_entry(elf, ehdr, "__minic_print");
    uint32_t simulation_entry = get_func_entry(elf, ehdr, "main");
    fclose(elf);
    
    StatInfo stat_info;
    
    if(db_option)
        stat_info = simulate_db(simulation_entry, special_entry);
    else
        stat_info = simulate(simulation_entry, special_entry);
    
    if(stat_option)
        print_stat_info(&stat_info);
    
    mem_free();
    
    return 0;
}
