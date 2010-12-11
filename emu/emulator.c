#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include <loader/elfmanip.h>
#include <memory/memory.h>
#include <pipeline.h>
#include <debug.h>

L2PT L1PageTable[L1PTSIZE];

void emulate_init(StoreArch * storage)
{
    memset(storage->reg, 0, 32 * sizeof(uint32_t));
    storage->reg[SP] = 0xf0000000;
    storage->reg[FP] = 0xf0000000;
    storage->reg[LR] = 0;
    storage->CMSR.N = 0;
    storage->CMSR.Z = 0;
    storage->CMSR.C = 0;
    storage->CMSR.V = 0;
    
    return;
}

void emulate(uint32_t emulation_entry, uint32_t special_entry)
{
    int cycle_count = 0;
    int instructon_count = 0;
    
    StoreArch storage;
    PipeState pipe_state;
    
    pipe_state.id_in.bubble = 1;
    pipe_state.ex_in.bubble = 1;
    pipe_state.mem_in.bubble = 1;
    pipe_state.wb_in.bubble = 1;

    /* initialization */
    emulate_init(&storage);
    storage.reg[PC] = emulation_entry;
    
    while(1)
    {
        ++cycle_count;
        
        WBStage(&storage, &pipe_state);
        MEMStage(&storage, &pipe_state);
        EXStage(&storage, &pipe_state);
        if(IDStage(&storage, &pipe_state) == -1)
            continue;           /* stalling */
        if(IFStage(&storage, &pipe_state, special_entry) == -1)
            break;

        ++instructon_count;
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
    uint32_t emulation_entry;
#ifdef DEBUG
    scanf("%x", &emulation_entry);
#else
    emulation_entry = get_func_entry(elf, ehdr, "main");
#endif
    
#ifndef NOPIPE
    emulate(emulation_entry, special_entry);
#else
    emulate_nopipe(emulation_entry, special_entry);
#endif
    mem_free();
    fclose(elf);
    return 0;
}
