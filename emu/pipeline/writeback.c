#include <pipeline/writeback.h>

int WBStage(StoreArch * storage, PipeState * pipe_state)
{
    if(pipe_state->wb_in.bubble)
        return 1;
    switch(pipe_state->wb_in.dest_sel)
    {
        case 0:
            storage->reg[pipe_state->wb_in.val_ex_dest]
                = pipe_state->wb_in.val_ex;
            break;
        case 1:
            storage->reg[pipe_state->wb_in.val_mem_dest]
                = pipe_state->wb_in.val_mem;
            break;
        case 2:
            storage->reg[pipe_state->wb_in.val_ex_dest]
                = pipe_state->wb_in.val_ex;
            storage->reg[pipe_state->wb_in.val_mem_dest]
                = pipe_state->wb_in.val_mem;
            break;
        case 3:
            break;
        default:
            exit(4);
    }
    return 1;
}

