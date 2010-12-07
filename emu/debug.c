#include <debug.h>
#include <stdio.h>
#include <memory/vmem.h>

void print_regfile(const StoreArch * storage)
{
    printf("r0 :0x%08x\tr1 :0x%08x\tr2 :0x%08x\tr3 :0x%08x\n", storage->reg[0], storage->reg[1], storage->reg[2], storage->reg[3]);
    printf("r4 :0x%08x\tr5 :0x%08x\tr6 :0x%08x\tr7 :0x%08x\n", storage->reg[4], storage->reg[5], storage->reg[6], storage->reg[7]);
    printf("r8 :0x%08x\tr9 :0x%08x\tr10:0x%08x\tr11:0x%08x\n", storage->reg[8], storage->reg[9], storage->reg[10], storage->reg[11]);
    printf("r12:0x%08x\tr13:0x%08x\tr14:0x%08x\tr15:0x%08x\n", storage->reg[12], storage->reg[13], storage->reg[14], storage->reg[15]);
    printf("r16:0x%08x\tr17:0x%08x\tr18:0x%08x\tr19:0x%08x\n", storage->reg[16], storage->reg[17], storage->reg[18], storage->reg[19]);
    printf("r20:0x%08x\tr21:0x%08x\tr22:0x%08x\tr24:0x%08x\n", storage->reg[20], storage->reg[21], storage->reg[22], storage->reg[23]);
    printf("r24:0x%08x\tr25:0x%08x\tr26:0x%08x\tfp :0x%08x\n", storage->reg[24], storage->reg[25], storage->reg[26], storage->reg[27]);
    printf("ip :0x%08x\tsp :0x%08x\tlr :0x%08x\tpc :0x%08x\n\n", storage->reg[28], storage->reg[29], storage->reg[30], storage->reg[31]);
    printf("CMSR.N:%ud\tCMSR.Z:%ud\tCMSR.C:%ud\tCMSR.V:%ud\n", storage->CMSR.N, storage->CMSR.Z, storage->CMSR.C, storage->CMSR.V);
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
