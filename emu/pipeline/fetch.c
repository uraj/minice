#include <pipeline/fetch.h>
#include <memory/vmem.h>

void fetch(StoreArch * storage, PipeState * pipe_state)
{
    uint32_t instr;
    mem_read(storage->reg[PC], &instr);
    pipe_state->id_in.instruction = instr;
    storage->reg[PC] += 4;
    return;
}

