#include <loader/elfmanip.h>
#include <memory/memory.h>
#include <pipeline/pipeline.h>
#include <stdio.h>

L2PT L1PageTable[L1PTSIZE];

void emulate(void * emulation_entry)
{
    StoreArch storage;
    PipeState pipe_state;
    
    pipe_state.id_in.bubble = 1;
    pipe_state.ex_in.bubble = 1;
    pipe_state.mem_in.bubble = 1;
    pipe_state.wb_in.bubble = 1;

    /* initialization */
    storage.reg[PC] = (uint32_t)emulation_entry;
    storage.reg[RA] = 0;
    
    if(IFStage(&storage, &pipe_state) == -1)
        return;
    
    while(1)
    {
        WBStage(&storage, &pipe_state);
        MEMStage(&storage, &pipe_state);
        EXStage(&storage, &pipe_state);
        if(IDStage(&storage, &pipe_state) == -1)
            continue;           /* stalling */
        if(IFStage(&storage, &pipe_state) == -1)
            break;
    }
    return;
}

void emulate_nopipe(void * emulation_entry)
{
    StoreArch storage;
    PipeState pipe_state;
    
    pipe_state.id_in.bubble = 1;
    pipe_state.ex_in.bubble = 1;
    pipe_state.mem_in.bubble = 1;
    pipe_state.wb_in.bubble = 1;

    /* initialization */
    storage.reg[PC] = (uint32_t)emulation_entry;
    storage.reg[RA] = 0;
    
    while(1)
    {
        if(IFStage(&storage, &pipe_state) == -1)
            break;
        IDStage(&storage, &pipe_state);
        EXStage(&storage, &pipe_state);
        MEMStage(&storage, &pipe_state);
        WBStage(&storage, &pipe_state);
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
    void * emulation_entry = get_func_entry(elf, ehdr, "main");
    
    emulate_nopipe(emulation_entry);
    mem_free();
    return 0;
}
