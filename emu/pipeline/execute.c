#include <pipeline/execute.h>
#include <stdlib.h>

static uint32_t ALUcal(uint8_t aluopcode, uint32_t operand1, uint32_t operand2, PSW * cmsr)
{
    uint32_t ret;
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
            break;
        case 0x3U:              /* rsb */
            ret = operand2 - operand1;
            break;
        case 0x4U:              /* add */
        case 0xbU:              /* cadd */
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

int EXStage(StoreArch * storage, PipeState * pipe_state)
{
    if(pipe_state->ex_in.bubble)
    {
        pipe_state->mem_in.bubble = 1;
        return 1;
    }
    if(pipe_state->ex_in.mulop != MulNop)
    {
        Mul_cal();
        return 2;
    }
    if(pipe_state->ex_in.aluopcode != ALU_NOP)
    {
        if(pipe_state->ex_in.S)
            ALUcal_setCMSR(
                pipe_state->ex_in.aluopcode,
                pipe_state->ex_in.operand1,
                pipe_state->ex_in.operand2,
                &(storage->CMSR));
        else
            ALUcal(
                pipe_state->ex_in.aluopcode,
                pipe_state->ex_in.operand1,
                pipe_state->ex_in.operand2,
                &(storage->CMSR));
        return 1;
    }
    return 1;
}
