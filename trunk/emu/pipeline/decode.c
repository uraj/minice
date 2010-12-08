#include <pipeline/decode.h>
#include <pipeline/control.h>
#include <stdlib.h>

#define GET_FIELD(code, hi, lo)                         \
    (((code) >> (lo)) & (0x7fffffff >> (31 - (hi)+ (lo) - 1)))

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
            return Branch_Cond;
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

/* detect data hazard, return 1: hazard exists, stall the pipeline; return 0: secure */
static int read_register(const StoreArch * storage, const PipeState * pipe_state, uint8_t reg_index, uint32_t * dest)
{
    if(reg_index == pipe_state->id_in.ex_fwd.freg)
        *dest = pipe_state->id_in.ex_fwd.fdata;
    else if(reg_index == pipe_state->id_in.mem_fwd.freg)
        *dest = pipe_state->id_in.mem_fwd.fdata;
    else if((pipe_state->mem_in.wb_dest_sel == 1 || pipe_state->mem_in.wb_dest_sel == 2) &&
            (reg_index == pipe_state->mem_in.wb_val_mem_dest))
            return 1;
    else
        *dest = storage->reg[reg_index];
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

static uint32_t gen_operand2(StoreArch * storage, const PipeState * pipe_state, const InstrFields * ifields, InstrType itype, uint32_t * data)
{
    uint32_t temp;
    ShiftType stype;
    int setMSR = 0;
    uint32_t rdata1, rdata2;
    
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
                if(read_register(storage, pipe_state, ifields->rm, &rdata1))
                    return 1;
                temp = (storage->CMSR).C;
                (storage->CMSR).C = rdata1 & 1U; /* rdata1 = reg[rm] */
                *data =  (rdata1 >> 1) | (temp << 31);
            }
            else
            {
                if(read_register(storage, pipe_state, ifields->rm, &rdata1))
                    return 1;
                *data = operand2_shift(
                    rdata1,     /* rdata1 = reg[rm] */
                    ifields->shift_imm,
                    stype,
                    setMSR,
                    &(storage->CMSR));
            }
            break;
        case D_RegShift:
            if(read_register(storage, pipe_state, ifields->rm, &rdata1) ||
               read_register(storage, pipe_state, ifields->rs, &rdata2))
                return 1;
            *data = operand2_shift(
                rdata1,          /* rdata1 = rm */
                (uint8_t)rdata2, /* rdata2 = rs */
                stype,
                setMSR,
                &(storage->CMSR));
            break;
        case D_Immidiate:
            *data =  operand2_shift(
                ifields->rotate_imm9,
                ifields->rotate,
                stype,
                setMSR,
                &(storage->CMSR));
            break;
        case Multiply:
        case LS_Hw_RegOff:
            if(read_register(storage, pipe_state, ifields->rm, &rdata1))
                return 1;           
            *data = rdata1;
            break;
            
        case LS_ImmOff:
            *data = (uint32_t)(ifields->high_imm14);
            break;
        case LS_Hw_ImmOff:
            *data =
                (((uint32_t)(ifields->high_off)) << 5) |
                (((uint32_t)(ifields->low_off)));
            break;
        default:
            ;
    }
    return 0;
}

/* retrun cycle count (including flush or stalling penalty) */
/* if retrun -1, pipeline stalls */
int IDStage(StoreArch * storage, PipeState * pipe_state)
{
    if(pipe_state->id_in.bubble == 1)
    {
        pipe_state->ex_in.bubble = 1;
        return 1;
    }
    InstrFields ifields = get_fields(pipe_state->id_in.instruction);
    InstrType itype = get_instr_type(pipe_state->id_in.instruction);
    int data_hazard;
    uint32_t data;

    gen_control_signals(&ifields, itype, &(pipe_state->ex_in));
    
    /* deal with branch first */
    if(itype == Branch_Ex)
    {
        pipe_state->ex_in.bubble = 1; /* gen bubbles */
        if(ifields.flags.L == 1) /* Branch and link */
            storage->reg[RA] = storage->reg[PC];
        storage->reg[PC] = storage->reg[ifields.rm];
        storage->reg[PC] &= 0xfffffffcU; /* PC word align */
        return 1;
    }
    else if(itype == Branch_Cond)
    {
        pipe_state->ex_in.bubble = 0; /* gen bubbles */
        if(ifields.flags.L == 1) /* Branch and link */
            pipe_state->ex_in.condop = CondBranchLink;
        else
            pipe_state->ex_in.condop = CondBranch;
        pipe_state->ex_in.condcode_branch = ifields.cond;
        pipe_state->ex_in.branch_imm_offset = ifields.signed_off_imm24;
        return 1;
    }
    
    pipe_state->ex_in.bubble = 0;
    pipe_state->ex_in.S = ifields.flags.S;
    pipe_state->ex_in.condop = NoBranch;
    
    /* then deal with multiply */
    if(itype == Multiply)
    {
        data_hazard = read_register(storage, pipe_state, ifields.rn, &data);
        if(data_hazard)
        {
            pipe_state->ex_in.bubble = 1;
            return -1;
        }
        pipe_state->ex_in.val_rn = data;
        data_hazard = read_register(storage, pipe_state, ifields.rm, &data);
        if(data_hazard)
        {
            pipe_state->ex_in.bubble = 1;
            return -1;
        }
        pipe_state->ex_in.val_rm = data;
        
        if(ifields.flags.A == 1)
        {
            pipe_state->ex_in.mulop = MulAdd;
            data_hazard = read_register(storage, pipe_state, ifields.rs, &data);
            if(data_hazard)
            {
                pipe_state->ex_in.bubble = 1;
                return -1;
            }
            pipe_state->ex_in.val_rs = data;
        }
        else
            pipe_state->ex_in.mulop = Mul;
        
        pipe_state->ex_in.aluopcode = ALU_NOP;
        return 1;
    }
    else
        pipe_state->ex_in.mulop = MulNop;

    if(ifields.opcode == 0x0dU || ifields.opcode == 0x0fU ) /* MVN or MOV, operand1 is of no use. Thus ex_in.operand1 stores ifields.rn for executing conditional MVN or MOV */
        pipe_state->ex_in.operand1 = ifields.rn;
    else /* gen operand1. operand1 is always reg[rn] */
    {
        data_hazard = read_register(storage, pipe_state, ifields.rn, &data);
        if(data_hazard)
        {
            pipe_state->ex_in.bubble = 1;
            return -1;
        }
        else
            pipe_state->ex_in.operand1 = data;
    }
    
    data_hazard = gen_operand2(storage, pipe_state, &ifields, itype, &data);
    if(data_hazard)
    {
        pipe_state->ex_in.bubble = 1;
        return -1;
    }
    else
        pipe_state->ex_in.operand2 = data;
    
    /* ALU's execuation in next EX stage */
    if((itype == LS_RegOff) ||
       (itype == LS_ImmOff) ||
       (itype == LS_Hw_RegOff) ||
       (itype == LS_Hw_ImmOff))
    {
        pipe_state->ex_in.S = 0;
        if((ifields.flags).U == 1)
            pipe_state->ex_in.aluopcode =  0x4U;
        else
            pipe_state->ex_in.aluopcode =  0x2U;
        if(ifields.flags.L == 0) /* store instruction */
        {
            data_hazard = read_register(storage, pipe_state, ifields.rd, &data);
            if(data_hazard)
            {
                pipe_state->ex_in.bubble = 1;
                return -1;
            }
            else
                pipe_state->ex_in.val_rd = data;
        }
    }
    else
        pipe_state->ex_in.aluopcode = ifields.opcode;

    return 1;
}
