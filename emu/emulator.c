#include <loader/elfmanip.h>
#include <memory/memory.h>
#include <pipeline.h>
#include <stdio.h>
#define DEBUG
L2PT L1PageTable[L1PTSIZE];

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
    storage.reg[PC] = (uint32_t)emulation_entry;
    storage.reg[LR] = 0;
    
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

void emulate_single_instruction(void * emulation_entry)
{
    StoreArch storage;
    PipeState pipe_state;
    
    pipe_state.id_in.bubble = 0;
    pipe_state.ex_in.bubble = 0;
    pipe_state.mem_in.bubble = 0;
    pipe_state.wb_in.bubble = 0;

    /* initialization */
    storage.reg[PC] = (uint32_t)emulation_entry;
    storage.reg[LR] = 0;
    
    if(IFStage(&storage, &pipe_state, 0) == -1)
        return;
    if(pipe_state.id_in.bubble == 0)
        IDStage(&storage, &pipe_state);
    if(pipe_state.ex_in.bubble == 0)
        EXStage(&storage, &pipe_state);
    if(pipe_state.mem_in.bubble == 0)
        MEMStage(&storage, &pipe_state);
    if(pipe_state.wb_in.bubble == 0)
        WBStage(&storage, &pipe_state);
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
#ifndef DEBUG
    uint32_t emulation_entry = get_func_entry(elf, ehdr, "main");
    uint32_t special_entry = get_func_entry(elf, ehdr, "__minic_print");
    emulate(emulation_entry, special_entry);
#else
    void * emulation_entry;
    scanf("%p", &emulation_entry);
    emulate_single_instruction(emulation_entry);
#endif
    mem_free();
    return 0;
}
