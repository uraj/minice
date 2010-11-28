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
    Branch_Link,
    LS_RegOff,
    LS_ImmOff,
    LS_Hw_RegOff,
    LS_Hw_ImmOff,
} InstrType;

typedef struct
{
    instr instruction;
} ID_input;

typedef enum
{
    ALU_Add,
    ALU_Sub,
    ALU_Mul,
    ALU_And,
    ALU_Xor,
    ALU_Adc,
    ALU_Sbc,
    ALU_Rsb,
    ALU_Rsc,
    ALU_Cand,
    ALU_Cxor,
    ALU_Csub,
    ALU_Cadd,
    ALU_Or,
    ALU_Mov,
    ALU_Clb,
    ALU_Mvn,
    ALU_Nop
} ALUop;

typedef enum
{
    Mul,
    MulAdd,
    MulNop
} MULop;

typedef struct
{
    InstrFields ifields;
    uint32_t operand1, operand2;
    int S;
    ALUop aluop;
    MULop mulop;
    uint8_t rn, rm, rs, rd;
} EX_input;

typedef struct
{
    int token;
} MEM_input;

typedef struct
{
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
