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
    uint8_t freg;
    uint32_t fdata;
} FwdData;

typedef struct
{
    instr instruction;
    FwdData ex_fwd, mem_fwd;
} ID_input;

#define ALU_NOP 0xffU

typedef enum { Mul, MulAdd, MulNop } MULop;

typedef struct
{
    short bubble;
    short S;
    uint8_t aluopcode;
    MULop mulop;
    union
    {
        uint32_t operand1;
        uint32_t val_base;
        uint32_t val_rn;
    };
    union
    {
        uint32_t operand2;
        uint32_t val_rm;
    };
    uint32_t val_rs;

    /* the following will be pushed to MEM Stage */
    short mem_addr_sel;
    short mem_data_size;
    short mem_sign_ext;
    
    short wb_dest_sel;
    uint8_t wb_val_ex_dest;
    uint8_t wb_val_mem_dest;
} EX_input;

typedef struct
{
    short bubble;
    /* addr_sel = 0: addr = val_ex */
    /* addr_sel = 1: addr = val_base */
    /* addr_sel = 2: addr undefined(no mem acsess) */
    short addr_sel;
    /* data_size = 4: word */
    /* data_size = 2: half word */
    /* data_size = 1: byte */
    short data_size;
    short sign_ext;
    uint32_t val_ex;            /* from EX stage */
    uint32_t val_base;          /* from val_rn in EX stage */
    
    /* the following will be pushed to WB stage */
    int wb_dest_sel;
    uint8_t wb_val_ex_dest;
    uint8_t wb_val_mem_dest;
    
} MEM_input;

typedef struct
{
    short bubble;

    /* dest_sel = 0: wb val_ex */
    /* dest_sel = 1: wb val_mem */
    /* dest_sel = 2: both */
    /* dest_sel = 3: neither */
    short dest_sel;
    
    uint8_t val_ex_dest;
    uint8_t val_mem_dest;
    
    uint32_t val_ex;            /* from EX stage */
    uint32_t val_mem;           /* from MEM stage */

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
