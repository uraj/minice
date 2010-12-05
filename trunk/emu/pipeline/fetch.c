#include <pipeline/fetch.h>
#include <memory/memory.h>

int IFStage(StoreArch * storage, PipeState * pipe_state)
{
    uint32_t instr;
    int readinfo;
    readinfo = mem_read_instruction(storage->reg[PC], &instr);
    if(readinfo == -1)
    {
        fprintf(stderr, "Invalid instruction fetch.\n");
        exit(1);
    }
    pipe_state->id_in.instruction = instr;
    storage->reg[PC] += 4;
    return 0;
}
