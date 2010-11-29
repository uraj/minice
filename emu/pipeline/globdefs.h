#ifndef __MINIEMU_GLOBDEFS_H__
#define __MINIEMU_GLOBDEFS_H__

#include <stdint.h>

typedef uint32_t instr;

typedef struct
{
    uint8_t A, S, L, P, U, B, W, H, LSsign, LShalf;
} IstrFlags;

typedef struct
{
    uint8_t type;
    uint8_t rn, rd;
    union                       /* reuse */
    {
        uint8_t rs;
        uint8_t shift_imm;
        uint8_t rotate;
        uint8_t high_off;
    };
    union                       /* reuse */
    {
        uint8_t rm;
        uint8_t low_off;
    };
    IstrFlags flags;
    union                       /* reuse */
    {    
        uint8_t opcode;
        uint8_t cond;
    };
    uint8_t shift_type;
    uint16_t rotate_imm9;       /* unsigned */
    uint16_t high_imm14;
    uint32_t signed_off_imm24;
} InstrFields;

typedef enum
{
    D_ImmShift,
    D_RegShift,
    D_Immidiate,
    Multiply,
    Branch_Ex,
    Branch_Cond,
    LS_RegOff,
    LS_ImmOff,
    LS_Hw_RegOff,
    LS_Hw_ImmOff,
} InstrType;

typedef struct
{
    uint8_t fwdreg;
    uint32_t fwddata;
} FwdData;

typedef struct
{
    instr instruction;
    FwdData ex_fwd, mem_fwd;
} ID_input;

#define ALU_NOP 0xffU

typedef enum
{
    Mul,
    MulAdd,
    MulNop
} MULop;

typedef struct
{
    int bubble;
    uint8_t aluopcode;
    MULop mulop;
    int S;
    uint32_t operand1, operand2;
    uint8_t rn, rm, rs, rd;
} EX_input;

typedef struct
{
    int bubble;
} MEM_input;

typedef struct
{
    int bubble;
    int WBrn;
    uint32_t val_rn;
    uint8_t rn;
    int WBrm;
    uint32_t val_rm;
    uint8_t rm;
} WB_input;

typedef struct
{
    ID_input id_in;
    EX_input ex_in;
    MEM_input mem_in;
    WB_input wb_in;
} PipeState;

typedef struct
{
    uint8_t N,Z,C,V;
} PSW;

typedef struct
{
    uint32_t reg[32];
    PSW CMSR;
} StoreArch;

#define RA 30
#define PC 31

#endif
