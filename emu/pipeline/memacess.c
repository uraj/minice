#include <pipeline/memacess.h>
#include <memory/memory.h>
#include <stdlib.h>
#include <stdio.h>

int MEMStage(StoreArch * storage, PipeState * pipe_state)
{
    if(pipe_state->mem_in.bubble)
    {
        pipe_state->wb_in.bubble = 1;
        return 1;
    }

    pipe_state->wb_in.dest_sel = pipe_state->mem_in.wb_dest_sel;
    pipe_state->wb_in.val_ex = pipe_state->mem_in.val_ex;
    pipe_state->wb_in.val_ex_dest = pipe_state->mem_in.wb_val_ex_dest;
    pipe_state->wb_in.val_mem_dest = pipe_state->mem_in.wb_val_mem_dest;
    if(pipe_state->mem_in.addr_sel == 2)
        return 1;
    else
    {
        if(pipe_state->mem_in.load)
        {
            uint32_t data;
            int readinfo;
            FwdData mem_fwd;
            if(pipe_state->mem_in.addr_sel == 0)
                readinfo = mem_read(
                    pipe_state->mem_in.val_ex,
                    pipe_state->mem_in.data_size,
                    pipe_state->mem_in.sign_ext,
                    &data);
            else if(pipe_state->mem_in.addr_sel == 1)
                readinfo = mem_read(
                    pipe_state->mem_in.val_base,
                    pipe_state->mem_in.data_size,
                    pipe_state->mem_in.sign_ext,
                    &data);
            if(readinfo != 0)
            {
                fprintf(stderr, "Invalid memory read.\n");
                exit(3);
            }
            pipe_state->wb_in.val_mem = data;
            
            /* data forwarding */
            mem_fwd.fdata = data;
            mem_fwd.freg = pipe_state->mem_in.val_mem_dest;
            pipe_state->id_in.mem_fwd = mem_fwd;
        }
        else
        {
            int write_info;
            if(pipe_state->mem_in.addr_sel == 0)
                write_info = mem_write(
                    pipe_state->mem_in.val_ex,
                    pipe_state->mem_in.data_size,
                    pipe_state->mem_in.val_store);
            else if(pipe_state->mem_in.addr_sel = 1)
                write_info = mem_write(
                    pipe_state->mem_in.val_base,
                    pipe_state->mem_in.data_size,
                    pipe_state->mem_in.val_store);
            if(write_info != 0)
            {
                fprintf(stderr, "Invalid memory write.\n");
                exit(3);    
            }
        }
    }
    return 1;
}