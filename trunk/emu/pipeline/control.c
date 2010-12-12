#include <pipeline/control.h>

void gen_control_signals(const InstrFields * ifields, InstrType itype, EX_input * ex_in)
{
    switch(itype)
    {
        case D_ImmShift:
        case D_RegShift:
        case D_Immidiate:
            ex_in->bubble = 0;
            ex_in->mem_addr_sel = 2; /* no mem acsess */
            if((ifields->opcode <= 0xbU) && (ifields->opcode >= 8))
                ex_in->wb_dest_sel = 3; /* no write back */
            else
            {
                ex_in->wb_dest_sel = 0; /* write back val_ex */
                ex_in->wb_val_ex_dest = ifields->rd;
            }
            break;
            
        case Multiply:
            ex_in->bubble = 0;
            ex_in->mem_addr_sel = 2; /* no mem acsess */
            ex_in->wb_dest_sel = 0;  /* write back val_ex */
            ex_in->wb_val_ex_dest = ifields->rd;
            break;
            
        case Usr_Trap:
        case Branch_Ex:
        case Branch_Cond:
            ex_in->bubble = 1;
            break;
            
        case LS_RegOff:
        case LS_ImmOff:
            ex_in->bubble = 0;
            ex_in->wb_val_mem_dest = ifields->rd;
            ex_in->mem_load = ifields->flags.L;
            
            if(ifields->flags.B == 1)
            {
                ex_in->mem_data_size = 1;
                ex_in->mem_sign_ext = 0; /* zero extension */
            }
            else
                ex_in->mem_data_size = 4;

            if(ifields->flags.P == 1)
            {
                ex_in->mem_addr_sel = 0; /* val_ex as mem address */
                if(ifields->flags.W == 1)
                {
                    if(ex_in->mem_load == 1) /* load */
                        ex_in->wb_dest_sel = 2; /* both of val_ex and val_mem have to be written back */
                    else                        /* store */
                        ex_in->wb_dest_sel = 0;
                    ex_in->wb_val_ex_dest = ifields->rn;
                }
                else
                {
                    if(ex_in->mem_load == 1) /* load */
                        ex_in->wb_dest_sel = 1; /* only val_mem needs to be written back */
                    else                        /* store */
                        ex_in->wb_dest_sel = 3;
                }
            }
            else                /* P == 0 */
            {
                ex_in->mem_addr_sel = 1; /* val_base as mem address */
                if(ex_in->mem_load == 1) /* load */
                    ex_in->wb_dest_sel = 2;
                else            /* store */
                    ex_in->wb_dest_sel = 0;
                ex_in->wb_val_ex_dest = ifields->rn;
            }
            break;
                    
        case LS_Hw_RegOff:
        case LS_Hw_ImmOff:
            ex_in->bubble = 0;
            ex_in->wb_val_mem_dest = ifields->rd;
            ex_in->mem_sign_ext = ifields->flags.LSsign;
            ex_in->mem_load = ifields->flags.L;
            
            if(ifields->flags.LShalf == 1)
                ex_in->mem_data_size = 2;
            else
                ex_in->mem_data_size = 1;
            
            if(ifields->flags.P == 1)
            {
                ex_in->mem_addr_sel = 0; /* val_ex as mem address */
                if(ifields->flags.W == 1) /* base addr reg written back */
                {
                    if(ex_in->mem_load == 1) /* load */
                        ex_in->wb_dest_sel = 2; /* both of val_ex and val_mem have to be written back */
                    else
                        ex_in->wb_dest_sel = 0;
                    ex_in->wb_val_ex_dest = ifields->rn;
                }
                else
                {
                    if(ex_in->mem_load == 1) /* load */
                        ex_in->wb_dest_sel = 1; /* only val_mem needs to be written back */
                    else                        /* store */
                        ex_in->wb_dest_sel = 3;
                }
            }
            else
            {
                ex_in->mem_addr_sel = 1; /* val_base as mem address */
                if(ex_in->mem_load)      /* load */
                    ex_in->wb_dest_sel = 2;
                else            /* store */
                    ex_in->wb_dest_sel = 0;
                ex_in->wb_val_ex_dest = ifields->rn;
            }
            break;
    }
    return;
}
