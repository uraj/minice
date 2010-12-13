#include <debug.h>
#include <stdio.h>
#include <memory/vmem.h>

void print_stage_info(const StageInfo * sinfo)
{
    printf("0x%08x 0x%08x\n", sinfo->pc, sinfo->icode);
    return;
}


void pregs()
{
    if(gp_reg == NULL)
    {
        printf("Register file not initialized yet.\n");
        return;
    }
    printf("r0 :0x%08x\tr1 :0x%08x\tr2 :0x%08x\tr3 :0x%08x\n", gp_reg->reg[0], gp_reg->reg[1], gp_reg->reg[2], gp_reg->reg[3]);
    printf("r4 :0x%08x\tr5 :0x%08x\tr6 :0x%08x\tr7 :0x%08x\n", gp_reg->reg[4], gp_reg->reg[5], gp_reg->reg[6], gp_reg->reg[7]);
    printf("r8 :0x%08x\tr9 :0x%08x\tr10:0x%08x\tr11:0x%08x\n", gp_reg->reg[8], gp_reg->reg[9], gp_reg->reg[10], gp_reg->reg[11]);
    printf("r12:0x%08x\tr13:0x%08x\tr14:0x%08x\tr15:0x%08x\n", gp_reg->reg[12], gp_reg->reg[13], gp_reg->reg[14], gp_reg->reg[15]);
    printf("r16:0x%08x\tr17:0x%08x\tr18:0x%08x\tr19:0x%08x\n", gp_reg->reg[16], gp_reg->reg[17], gp_reg->reg[18], gp_reg->reg[19]);
    printf("r20:0x%08x\tr21:0x%08x\tr22:0x%08x\tr24:0x%08x\n", gp_reg->reg[20], gp_reg->reg[21], gp_reg->reg[22], gp_reg->reg[23]);
    printf("r24:0x%08x\tr25:0x%08x\tr26:0x%08x\tfp :0x%08x\n", gp_reg->reg[24], gp_reg->reg[25], gp_reg->reg[26], gp_reg->reg[27]);
    printf("ip :0x%08x\tsp :0x%08x\tlr :0x%08x\tpc :0x%08x\n\n", gp_reg->reg[28], gp_reg->reg[29], gp_reg->reg[30], gp_reg->reg[31]);
    printf("CMSR.N:%u\tCMSR.Z:%u\tCMSR.C:%u\tCMSR.V:%u\n", gp_reg->CMSR.N, gp_reg->CMSR.Z, gp_reg->CMSR.C, gp_reg->CMSR.V);
    return;
}

void print_mem_word(uint32_t vaddr)
{
    uint32_t data_32;
    if(mem_read_direct_w(vaddr, &data_32))
        printf("Cannot access memory in 0x%08x.\n", vaddr);
    else
        printf("0x%08x: 0x%08x\n", vaddr, data_32);
    return;
}

void print_mem_halfword(uint32_t vaddr)
{
    uint16_t data_16;
    if(mem_read_direct_h(vaddr, &data_16))
        printf("Cannot access memory in 0x%08x.\n", vaddr);
    else
        printf("0x%08x: 0x%08x\n", vaddr, data_16);
    return;
}

void print_mem_byte(uint32_t vaddr)
{
    uint8_t data_8;
    if(mem_read_direct_b(vaddr, &data_8))
        printf("Cannot access memory in 0x%08x.\n", vaddr);
    else
        printf("0x%08x: 0x%08x\n", vaddr, data_8);
    return;
}
