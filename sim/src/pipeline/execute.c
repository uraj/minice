#include <pipeline/execute.h>
#include <pipeline/control.h>
#include <stdlib.h>

static uint32_t ALUcal(uint8_t aluopcode, uint32_t operand1, uint32_t operand2, PSW * cmsr)
{
    uint32_t ret;
    switch(aluopcode)
    {
        case 0x0U:              /* and */
            ret = operand1 & operand2;                
            break;
        case 0x1U:              /* xor */
            ret = operand1 ^ operand2;
            break;
        case 0x2U:              /* sub */
            ret = operand1 - operand2;
            break;
        case 0x3U:              /* rsb */
            ret = operand2 - operand1;
            break;
        case 0x4U:              /* add */
            ret = operand1 + operand2;
            break;
        case 0x5U:              /* adc */
            ret = operand1 + operand2 + cmsr->C;
            break;
        case 0x6U:              /* sbc */
            ret = operand1 - operand2 + cmsr->C - 1;
            break;
        case 0x7U:              /* rsc */
            ret = operand2 - operand1 + cmsr->C - 1;
            break;
        case 0xcU:              /* or */
            ret = operand1 | operand2;
            break;
        case 0xdU:
            ret = operand2;
            break;
        case 0xeU:
            ret = operand1 & ~operand2;
            break;
        case 0xfU:
            ret = ~operand2;
            break;
        default:
            exit(2);
    }
    return ret;
}

static uint32_t ALUcal_setCMSR(uint8_t aluopcode, uint32_t operand1, uint32_t operand2, PSW * cmsr)
{
    uint32_t ret;
    uint32_t C31, C32;
    switch(aluopcode)
    {
        case 0x0U:              /* and */
        case 0x8U:              /* cand */
            ret = operand1 & operand2;
            break;
        case 0x1U:              /* xor */
        case 0x9U:              /* cxor */
            ret = operand1 ^ operand2;
            break;
        case 0x2U:              /* sub */
        case 0xaU:              /* csub */
            ret = operand1 - operand2;
            C31 = ((operand1 & 0x7fffffffU) - (operand2 & 0x7fffffffU)) >> 31;
            C32 = (((uint64_t)operand1 - operand2) >> 32) & 1;
            cmsr->C = !C32;
            cmsr->V = C32 ^ C31;
            break;
        case 0x3U:              /* rsb */
            ret = operand2 - operand1;
            C31 = ((operand2 & 0x7fffffffU) - (operand1 & 0x7fffffffU)) >> 31;
            C32 = (((uint64_t)operand2 - operand1) >> 32) & 1;
            cmsr->C = !C32;
            cmsr->V = C32 ^ C31;
            break;
        case 0x4U:              /* add */
        case 0xbU:              /* cadd */
            ret = operand1 + operand2;
            C31 = ((operand1 & 0x7fffffffU) + (operand2 & 0x7fffffffU)) >> 31;
            C32 = ((uint64_t)operand1 + operand2) >> 32;
            cmsr->C = C32;
            cmsr->V = C32 ^ C31;
            break;
        case 0x5U:              /* adc */
            ret = operand1 + operand2 + cmsr->C;
            C31 = ((operand1 & 0x7fffffffU) + (operand2 & 0x7fffffffU) + cmsr->C) >> 31;
            C32 = ((uint64_t)operand1 + operand2 + cmsr->C) >> 32;
            cmsr->C = C32;
            cmsr->V = C32 ^ C31;
            break;
        case 0x6U:              /* sbc */
            ret = operand1 - operand2 + cmsr->C - 1;
            C31 = ((operand1 & 0x7fffffffU) - (operand2 & 0x7fffffffU) + cmsr->C - 1) >> 31;
            C32 = (((uint64_t)operand1 - operand2) >> 32) & 1;
            cmsr->C = !C32;
            cmsr->V = C32 ^ C31;
            break;
        case 0x7U:              /* rsc */
            ret = operand2 - operand1 + cmsr->C - 1;
            C31 = ((operand2 & 0x7fffffffU) - (operand1 & 0x7fffffffU) + cmsr->C - 1) >> 31;
            C32 = (((uint64_t)operand2 - operand1) >> 32) & 1;
            cmsr->C = !C32;
            cmsr->V = C32 ^ C31;
            break;
        case 0xcU:              /* or */
            ret = operand1 | operand2;
            break;
        case 0xdU:              /* mov */
            ret = operand2;
            return ret;
        case 0xeU:              /* clr */
            ret = operand1 & ~operand2;
            break;
        case 0xfU:              /* mvn */
            ret = ~operand2;
            return ret;
        default:
            exit(2);
    }
    cmsr->N = ret >> 31;
    cmsr->Z = !ret;
    return ret;
}

static uint32_t Mulcal(RegFile * storage, const EX_input * ex_in)
{
    uint32_t ret;
    switch(ex_in->mulop)
    {
        case Mul:
            ret = ex_in->val_rn * ex_in->val_rm;
            break;
        case MulAdd:
            ret = ex_in->val_rn * ex_in->val_rm + ex_in->val_rs;
            break;
        default:
            exit(2);
    }
    if(ex_in->S)
    {
        storage->CMSR.N = ret >> 31;
        storage->CMSR.Z = !ret;
        storage->CMSR.C = 2;    /* C set to be undefined */
    }
    return ret;
}

int EXStage(RegFile * storage, PipeState * pipe_state)
{

    if(pipe_state->ex_in.bubble)
    {
        pipe_state->mem_in.bubble = 1;
        
        /* data forwarding */
        pipe_state->ex_fwd[1] = pipe_state->ex_fwd[0];
        pipe_state->ex_fwd[0].freg = 0xffU;
        return 0;
    }
    pipe_state->mem_in.bubble = 0;
    FwdData ex_fwd;
    if(pipe_state->ex_in.wb_dest_sel == 0 || pipe_state->ex_in.wb_dest_sel == 2)
        ex_fwd.freg = pipe_state->ex_in.wb_val_ex_dest;
    else
        ex_fwd.freg = 0xff;
    
    /* push control signals */
    pipe_state->mem_in.load = pipe_state->ex_in.mem_load;
    pipe_state->mem_in.addr_sel = pipe_state->ex_in.mem_addr_sel;
    pipe_state->mem_in.data_size = pipe_state->ex_in.mem_data_size;
    pipe_state->mem_in.sign_ext = pipe_state->ex_in.mem_sign_ext;

    pipe_state->mem_in.val_base = pipe_state->ex_in.val_base;
    pipe_state->mem_in.val_store = pipe_state->ex_in.val_rd;
    
    pipe_state->mem_in.wb_dest_sel = pipe_state->ex_in.wb_dest_sel;
    pipe_state->mem_in.wb_val_ex_dest = pipe_state->ex_in.wb_val_ex_dest;
    pipe_state->mem_in.wb_val_mem_dest = pipe_state->ex_in.wb_val_mem_dest;

    if(pipe_state->ex_in.mulop != MulNop)
    {    
        ex_fwd.fdata = Mulcal(storage, &(pipe_state->ex_in));
        pipe_state->mem_in.val_ex = ex_fwd.fdata;
        pipe_state->wb_in.val_ex = ex_fwd.fdata;
        
        /* data forwading */
        pipe_state->ex_fwd[1] = pipe_state->ex_fwd[0];
        pipe_state->ex_fwd[0] = ex_fwd;
            
        return 1;
    }
    else if(pipe_state->ex_in.aluopcode != ALU_NOP)
    {
        if(pipe_state->ex_in.S)
            ex_fwd.fdata = ALUcal_setCMSR(
                pipe_state->ex_in.aluopcode,
                pipe_state->ex_in.operand1,
                pipe_state->ex_in.operand2,
                &(storage->CMSR));
        else
            ex_fwd.fdata = ALUcal(
                pipe_state->ex_in.aluopcode,
                pipe_state->ex_in.operand1,
                pipe_state->ex_in.operand2,
                &(storage->CMSR));
        if((pipe_state->ex_in.aluopcode == 0xdU || pipe_state->ex_in.aluopcode == 0xfU) &&
           ((pipe_state->ex_in.operand1 & 0x10U) != 0) &&
           test_cond(&(storage->CMSR), (uint8_t)(pipe_state->ex_in.condcode_assign)) == 0) /* condtional MVN or MOV and test_cond fails*/
        {
            if(pipe_state->mem_in.addr_sel == 0)
                pipe_state->mem_in.addr_sel = 2;
            if(pipe_state->mem_in.wb_dest_sel == 0)
                pipe_state->mem_in.wb_dest_sel = 3;
            else if(pipe_state->mem_in.wb_dest_sel == 2)
                pipe_state->mem_in.wb_dest_sel = 1;
        }
        else
        {   
            pipe_state->mem_in.val_ex = ex_fwd.fdata;
            
            /* data forwarding */
            pipe_state->ex_fwd[1] = pipe_state->ex_fwd[0];
            pipe_state->ex_fwd[0] = ex_fwd;
        }
        return 0;
    }
    return 0;
}
