#include <pipeline/fetch.h>
#include <memory/memory.h>

int IFStage(StoreArch * storage, PipeState * pipe_state)
{
    uint32_t instr;
    mem_read(storage->reg[PC], 4, 0, &instr);
    pipe_state->id_in.instruction = instr;
    storage->reg[PC] += 4;
    return 0;
}
