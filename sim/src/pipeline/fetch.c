#include <memory/memory.h>
#include <stdio.h>
#include <stdlib.h>

#include "fetch.h"

int IFStage(RegFile * storage, PipeState * pipe_state, uint32_t special_entry)
{
    static int quit = 0;
    uint32_t pc = storage->reg[PC];
    uint32_t instr;
    int readinfo;

     /* as for pc, take the earlier one */
    if(pipe_state->ex_fwd[1].freg == PC)
    {
        pc = pipe_state->ex_fwd[1].fdata;
        /* flush the pipeline */
        pipe_state->wb_in.bubble = 1;
        pipe_state->mem_in.bubble = 1;
        pipe_state->ex_in.bubble = 1;
        
    }
    else if(pipe_state->ex_fwd[0].freg == PC)
    {
        pc = pipe_state->ex_fwd[0].fdata;
        /* flush the pipeline */
        pipe_state->mem_in.bubble = 1;
        pipe_state->ex_in.bubble = 1;
    }
    else
        pc = storage->reg[PC];
    
    if(pc == 0)
    {
        ++quit;
        if(quit == 5)           /* pipeline empty */
            return -1;
        else
        {
            pipe_state->id_in.bubble = 1;
            return 0;
        }
    }
    else if(pc == special_entry)
        instr = 0xffffffffU;
    else
    {
        readinfo = mem_read_instruction(pc, &instr);
        if(readinfo == -1)
        {
            fprintf(stderr, "Invalid instruction fetch.\n");
            exit(1);
        }
    }
    pipe_state->id_in.instruction = instr;
    pipe_state->id_in.bubble = 0;
    
    pipe_state->id_in.sinfo.icode = instr;
    pipe_state->id_in.sinfo.pc = pc;
    
    storage->reg[PC] = pc + 4;
    return 0;
}
