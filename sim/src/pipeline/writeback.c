#include <pipeline/writeback.h>
#include <stdlib.h>

int WBStage(RegFile * storage, PipeState * pipe_state)
{
    if(pipe_state->wb_in.bubble)
        return 0;
    switch(pipe_state->wb_in.dest_sel)
    {
        case WB_ValEx:
            if(pipe_state->wb_in.val_ex_dest != PC) /* PC is not editable in WB stage */
                storage->reg[pipe_state->wb_in.val_ex_dest]
                    = pipe_state->wb_in.val_ex;
            break;
        case WB_ValMem:
            storage->reg[pipe_state->wb_in.val_mem_dest]
                = pipe_state->wb_in.val_mem;
            break;
        case WB_Both:
            if(pipe_state->wb_in.val_ex_dest != PC)
                storage->reg[pipe_state->wb_in.val_ex_dest]
                    = pipe_state->wb_in.val_ex;
            storage->reg[pipe_state->wb_in.val_mem_dest]
                = pipe_state->wb_in.val_mem;
            break;
        case WB_Neither:
            break;
        default:
            exit(5);
    }
    return 0;
}
