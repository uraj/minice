#include <pipeline/fetch.h>
#include <memory/memory.h>
#include <stdio.h>
#include <stdlib.h>

int IFStage(RegFile * storage, PipeState * pipe_state, uint32_t special_entry)
{
    static int quit = 0;
    
    uint32_t instr;
    int readinfo;
    if(storage->reg[PC] == 0)
    {
        return -1;
        ++quit;
        if(quit == 5)           /* pipeline empty */
            return -1;
        else
        {
            pipe_state->id_in.bubble = 1;
            return 1;
        }
    }
    else if(storage->reg[PC] == special_entry)
        instr = 0xffffffffU;
    else
    {
        readinfo = mem_read_instruction(storage->reg[PC], &instr);
        if(readinfo == -1)
        {
            fprintf(stderr, "Invalid instruction fetch.\n");
            exit(1);
        }
    }
    pipe_state->id_in.instruction = instr;
    pipe_state->id_in.bubble = 0;
    storage->reg[PC] += 4;
    return 1;
}
