#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include <loader/elfmanip.h>
#include <memory/memory.h>
#include <pipeline.h>
#include <debug.h>

L2PT L1PageTable[L1PTSIZE];
const RegFile * gp_reg = NULL;
const PipeState * gp_pipe = NULL;

void emulate_init(RegFile * storage, PipeState * pipe_state) /* important */
{
    memset(storage->reg, 0, 32 * sizeof(uint32_t));
    storage->reg[SP] = 0xf0000000;
    storage->reg[FP] = 0xf0000000;
    storage->reg[LR] = 0;
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

void emulate(uint32_t emulation_entry, uint32_t special_entry)
{
    int cycle_count = 0;
    int instructon_count = 0;
    
    RegFile storage;
    PipeState pipe_state;
    gp_reg = &storage;
    gp_pipe = &pipe_state;
    
    /* initialization */
    emulate_init(&storage, &pipe_state);
    storage.reg[PC] = emulation_entry;
    
    while(1)
    {
        ++cycle_count;
        
#ifdef DEBUG
        printf("Stage: WB\t");
        print_stage_info(&(pipe_state.wb_in.sinfo));
#endif
        
        WBStage(&storage, &pipe_state);
        
#ifdef DEBUG
        printf("Stage: MEM\t");
        print_stage_info(&(pipe_state.mem_in.sinfo));
#endif
        
        MEMStage(&storage, &pipe_state);
        pipe_state.wb_in.sinfo = pipe_state.mem_in.sinfo;

#ifdef DEBUG
        printf("Stage: EX\t");
        print_stage_info(&(pipe_state.ex_in.sinfo));
#endif
        
        EXStage(&storage, &pipe_state);
        pipe_state.mem_in.sinfo = pipe_state.ex_in.sinfo;
        
#ifdef DEBUG
        printf("Stage: ID\t");
        print_stage_info(&(pipe_state.id_in.sinfo));
#endif
        
        if(IDStage(&storage, &pipe_state) == -1)
        {
            pipe_state.ex_in.bubble = 1;
            continue;           /* stalling */
        }
        pipe_state.ex_in.sinfo = pipe_state.id_in.sinfo;
        if(IFStage(&storage, &pipe_state, special_entry) == -1)
            break;
        ++instructon_count;
        pipe_state.id_in.sinfo.icode = pipe_state.id_in.instruction;
        pipe_state.id_in.sinfo.pc = storage.reg[PC] - 4;
        
    }
    return;
}

int main(int argc, char * argv[])
{
    if(argc < 2)
    {
        fprintf(stderr, "miniemu: no input file.\n");
        return 1;
    }
    
    FILE * elf = fopen(argv[1], "rb");
    
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
    uint32_t emulation_entry = get_func_entry(elf, ehdr, "main");
    emulate(emulation_entry, special_entry);
    mem_free();
    fclose(elf);
    return 0;
}
