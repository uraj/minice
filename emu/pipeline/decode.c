#include <pipeline/decode.h>
#include <stdlib.h>

#ifdef DEBUG
static uint32_t GET_FIELD(const instr code,const int hi,const int lo)
{
    uint32_t ret;
    int i;
    ret = code >> lo;
    ret &= 0x8ffffff >> 31 - (hi - lo + 1);
    return ret;
}
#else
#define GET_FIELD(code, hi, lo)                         \
    (((code) >> (lo)) & (0x8ffffff >> 31 -((hi)-(lo) + 1)))
#endif

static InstrFields get_fields(instr instruction)
{
    InstrFields ret;
    ret.type = GET_FIELD(instruction, 31, 29);
    ret.rn = GET_FIELD(instruction, 23, 19);
    ret.rd = GET_FIELD(instruction, 18, 14);
    ret.rs = GET_FIELD(instruction, 13, 9);
    ret.rm = GET_FIELD(instruction, 4, 0);
    
    ret.flags.A = ret.flags.W =
        GET_FIELD(instruction, 25, 25);
    ret.flags.S = ret.flags.L =
        GET_FIELD(instruction, 24, 24);
    ret.flags.P = GET_FIELD(instruction, 28, 28);
    ret.flags.U = GET_FIELD(instruction, 27, 27);
    ret.flags.B = GET_FIELD(instruction, 26, 26);
    ret.flags.LSsign = GET_FIELD(instruction, 7, 7);
    ret.flags.LShalf = GET_FIELD(instruction, 6, 6);

    ret.opcode = GET_FIELD(instruction, 28, 25);
    ret.shift_type = GET_FIELD(instruction, 7, 6);
    ret.rotate_imm9 = GET_FIELD(instruction, 8, 0);
    ret.high_imm14 = GET_FIELD(instruction, 13, 0);
    ret.signed_off_imm24 = GET_FIELD(instruction, 23, 0);
    return ret;
}

static InstrType get_instr_type(instr code)
{
    switch(GET_FIELD(code, 31, 29))
    {
        case 0U:
            goto L1;
        case 1U:
            return D_Immidiate;
        case 2U:
            goto L2;
        case 3U:
            return LS_ImmOff;
        case 5U:
            return Branch_Link;
        default:
            exit(1);
    }
  L1:
    switch(GET_FIELD(code, 8, 8))
    {
        case 0U:
            if(GET_FIELD(code, 5, 5) == 0U)
                return D_ImmShift;
            else
                return D_RegShift;
        case 1U:
            if(GET_FIELD(code, 28, 26) == 0U)
                return Multiply;
            else
                return Branch_Ex;
        default:
            exit(1);
    }
  L2:
    switch(GET_FIELD(code, 8, 8))
    {
        case 0U:
            return LS_RegOff;
        case 1U:
            if(GET_FIELD(code, 26, 26) == 0)
                return LS_Hw_RegOff;
            else
                return LS_Hw_ImmOff;
        default:
            exit(1);
    }
}

static uint32_t gen_operand1(const InstrFields * ifield, InstrType itype, StoreArch * storage)
{
    
    switch(itype)
    {
        case D_ImmShift:
        case D_RegShift:
        case D_Immidiate:
        case Multiply:
            return storage->reg[ifield->rn];
        default:
            exit(1);
    }
    return 0;
}

static uint32_t operand2_shift(uint32_t base, uint8_t bias, ShiftType stype, int setMSR, PSW * MSR)
{
    uint32_t ret;
    switch(stype)
    {
        case LShiftLeft:
            ret = base << bias;
            if(setMSR)
            {
                if(bias >= 32)
                    MSR->C = 0;
                else
                    MSR->C = 1U & (base >> (32 - bias));
            }
            break;
        case LShiftRight:
            ret = base >> bias;
            if(setMSR)
                MSR->C = 1U & (base >> (bias - 1));
            break;
        case AShiftRight:
            if((int)base >= 0)
            {
                if(bias == 0)
                    bias = 32;
                ret = base >> bias;
                if(setMSR)
                    MSR->C = 1U & (base >> (bias - 1));    
            }
            else
            {
                if(bias == 0)
                    bias = 32;
                if(setMSR)
                {
                    ret = base >> (bias - 1);
                    ret |= (0xffffffffU >> (33 - bias)) << (33 - bias);
                    MSR->C = 1U & ret;
                    ret = (ret >> 1) | ((ret >> 31) << 31);
                }
                else
                {
                    ret = base >> bias;
                    ret |= (0xffffffffU >> (32 - bias)) << (32 - bias);
                }
            }
            break;
        case RotateRight:
            bias %= 32;
            ret = base >> bias;
            ret |= base << (32 - bias);
            if(setMSR)
                MSR->C = 1U & (base >> (bias - 1));
            break;
    }
    return ret;
}

static uint32_t gen_operand2(const InstrFields * ifields, InstrType itype, StoreArch * storage)
{
    uint32_t temp;
    ShiftType stype;
    int setMSR = 0;
    
    /* when ALU operation is logical, MSR is set whet generating operand2 */
    if(((ifields->opcode < 0x1U) || (ifields->opcode > 0x7U)) &&
       ((ifields->flags).S == 1))
        setMSR = 1;
    
    switch(ifields->shift_type)
    {
        case 0:
            stype = LShiftLeft;
            break;
        case 1:
            stype = LShiftRight;
            break;
        case 2:
            stype = AShiftRight;
            break;
        case 3:
            stype = RotateRight;
            break;
        default:
            exit(1);
    }
    switch(itype)
    {
        case D_ImmShift:
        case LS_RegOff:
            if((stype == RotateRight) && (ifields->shift_imm == 0)) /* RRX */
            {
                temp = (storage->CMSR).C;
                (storage->CMSR).C = storage->reg[ifields->rm] & 1U;
                return (storage->reg[ifields->rm] >> 1) | (temp << 31);
            }
            return operand2_shift(
                storage->reg[ifields->rm],
                ifields->shift_imm,
                stype,
                setMSR,
                &(storage->CMSR));
        case D_RegShift:
            return operand2_shift(
                storage->reg[ifields->rm],
                (uint8_t)storage->reg[ifields->rs],
                stype,
                setMSR,
                &(storage->CMSR));
        case D_Immidiate:
            return operand2_shift(
                ifields->rotate_imm9,
                ifields->rotate,
                stype,
                setMSR,
                &(storage->CMSR));
        case Multiply:
        case LS_Hw_RegOff:
            return storage->reg[ifields->rm];
        case LS_ImmOff:
            return (uint32_t)(ifields->high_imm14);
        case LS_Hw_ImmOff:
            return
                (((uint32_t)(ifields->high_off)) << 5) |
                (((uint32_t)(ifields->low_off)));
    }
    return 0;
}

static ALUop gen_ALUop(const InstrFields * ifields, InstrType itype)
{
    uint8_t tmp_opcode = ifields->opcode;
    if((itype == LS_RegOff) ||
       (itype == LS_ImmOff) ||
       (itype == LS_Hw_RegOff) ||
       (itype == LS_Hw_ImmOff))
    {
        if((ifields->flags).U == 1)
            tmp_opcode = 0xbU;
        else
            tmp_opcode = 0xaU;
    }
    switch(tmp_opcode)
    {
        case 0x0U:
            return ALU_And;
        case 0x1U:
            return ALU_Xor;
        case 0x2U:
            return ALU_Sub;
        case 0x3U:
            return ALU_Rsb;
        case 0x4U:
            return ALU_Add;
        case 0x5U:
            return ALU_Adc;
        case 0x6U:
            return ALU_Sbc;
        case 0x7U:
            return ALU_Rsc;
        case 0x8U:
            return ALU_Cand;
        case 0x9U:
            return ALU_Cxor;
        case 0xaU:
            return ALU_Csub;
        case 0xbU:
            return ALU_Cadd;
        case 0xcU:
            return ALU_Or;
        case 0xdU:
            return ALU_Mov;
        case 0xeU:
            return ALU_Clb;
        case 0xfU:
            return ALU_Mvn;
    }
    return ALU_Nop;
}

void D_phrase(StoreArch * storage, PipeState * pipe_state)
{
    InstrFields ifields = get_fields(pipe_state->id_in.instruction);
    InstrType itype = get_instr_type(pipe_state->id_in.instruction);
    pipe_state->ex_in.ifields = ifields;
    pipe_state->ex_in.operand1 = gen_operand1(&ifields, itype, storage);
    pipe_state->ex_in.operand2 = gen_operand2(&ifields, itype, storage);
    pipe_state->ex_in.S = ifields.flags.S;
    pipe_state->ex_in.aluop = gen_ALUop(&ifields, itype);
    if(itype == Multiply)
    {
        if(ifields.flags.A == 1)
            pipe_state->ex_in.mulop = MulAdd;
        else
            pipe_state->ex_in.mulop = Mul;
    }
    else
        pipe_state->ex_in.mulop = MulNop;
    return;
}
