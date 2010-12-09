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
    int bubble;
    instr instruction;
    FwdData ex_fwd, mem_fwd;
} ID_input;

#define ALU_NOP 0xffU

typedef enum { Mul, MulAdd, MulNop } MulOp;

typedef enum
{
    CondBranch,
    CondBranchLink,
    NoBranch
} CondBranchOp;

typedef struct
{
    short bubble;
    short S;
    CondBranchOp condop;
    union
    {
        uint8_t aluopcode;
        uint8_t condcode_branch;
    };
    uint32_t branch_imm_offset;
    MulOp mulop;
    union
    {
        uint32_t operand1;
        uint32_t val_base;
        uint32_t val_rn;
        uint32_t condcode_assign;
    };
    union
    {
        uint32_t operand2;
        uint32_t val_rm;
    };
    union
    {
        uint32_t val_rd;        /* for store */
        uint32_t val_rs;        /* for multiply */
    };
    
    /* the following will be pushed to MEM Stage */
    uint8_t mem_load;
    uint8_t mem_addr_sel;
    uint8_t mem_data_size;
    uint8_t mem_sign_ext;
    
    uint8_t wb_dest_sel;
    uint8_t wb_val_ex_dest;
    uint8_t wb_val_mem_dest;
} EX_input;

typedef struct
{
    short bubble;
    
    /* load = 0: store */
    /* load = 1: load */
    uint8_t load;
    
    /* addr_sel = 0: addr = val_ex */
    /* addr_sel = 1: addr = val_base */
    /* addr_sel = 2: addr undefined(no mem acsess) */
    uint8_t addr_sel;
    
    /* data_size = 4: word */
    /* data_size = 2: half word */
    /* data_size = 1: byte */
    uint8_t data_size;
    uint8_t sign_ext;
    uint32_t val_ex;            /* from EX stage */
    uint32_t val_base;          /* from val_rn in EX stage */
    uint32_t val_store;         /* from val_rd in EX stage */
    
    /* the following will be pushed to WB stage */
    int wb_dest_sel;
    uint8_t wb_val_ex_dest;
    uint8_t wb_val_mem_dest;
    
} MEM_input;

typedef enum
{
    WB_ValEx = 0,
    WB_valMem = 1,
    WB_Both = 2,
    WB_Neither = 3,
} WB_SEL;

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

#define FP 27
#define IP 28
#define SP 29
#define LR 30
#define PC 31

#endif
