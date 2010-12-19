#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <console/console.h>
#include <console/bptree.h>
#include <memory/vmem.h>

#define BP_MAX 10

static inline void flush_stdin()
{
    while(getchar() != '\n')
        ;
    return;
}

void ppipe()
{
    printf("Stage  WB: <0x%08x> 0x%08x\n", gp_pipe->wb_in.sinfo.pc, gp_pipe->wb_in.sinfo.icode);
    printf("Stage MEM: <0x%08x> 0x%08x\n", gp_pipe->mem_in.sinfo.pc, gp_pipe->mem_in.sinfo.icode);
    printf("Stage  EX: <0x%08x> 0x%08x\n", gp_pipe->ex_in.sinfo.pc, gp_pipe->ex_in.sinfo.icode);
    printf("Stage  ID: <0x%08x> 0x%08x\n", gp_pipe->id_in.sinfo.pc, gp_pipe->id_in.sinfo.icode);
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

void pfwd()
{
    printf("EX forwarding 0: dest=%hu, value=0x%08x", (uint16_t)(gp_pipe->id_in.ex_fwd[0].freg), gp_pipe->id_in.ex_fwd[0].fdata);
    printf("EX forwarding 1: dest=%hu, value=0x%08x", (uint16_t)(gp_pipe->id_in.ex_fwd[1].freg), gp_pipe->id_in.ex_fwd[1].fdata);
    printf("EX forwarding 1: dest=%hu, value=0x%08x", (uint16_t)(gp_pipe->id_in.mem_fwd.freg), gp_pipe->id_in.mem_fwd.fdata);
    return;
}

static inline void  print_mem_word(uint32_t vaddr)
{
    uint32_t data_32;
    if(mem_read_direct_w(vaddr, &data_32))
        printf("Cannot access memory in 0x%08x.\n", vaddr);
    else
        printf("0x%08x: 0x%08x\n", vaddr, data_32);
    return;
}

static inline void print_mem_halfword(uint32_t vaddr)
{
    uint16_t data_16;
    if(mem_read_direct_h(vaddr, &data_16))
        printf("Cannot access memory in 0x%08x.\n", vaddr);
    else
        printf("0x%08x: 0x%08x\n", vaddr, data_16);
    return;
}

static inline void print_mem_byte(uint32_t vaddr)
{
    uint8_t data_8;
    if(mem_read_direct_b(vaddr, &data_8))
        printf("Cannot access memory in 0x%08x.\n", vaddr);
    else
        printf("0x%08x: 0x%08x\n", vaddr, data_8);
    return;
}

void pstack(uint32_t addr)
{
    print_mem_word(addr);
    return;
}

void pstack_range(uint32_t addr_b, uint32_t addr_e)
{
    while(addr_b < addr_e)
    {
        print_mem_word(addr_b);
        addr_b -= 4;
    }
    return;
}


extern jmp_buf beginning;

void console()
{
    /* input related */
    static size_t bufsize = 32;
    static char uinput[32];
    /* breakpoint */
    static BpTree * bpoints = NULL;
    
    int bp_count = 0;
    
    /* trap and continue */
    static int trap = 1;
    
    char * temp = uinput;
    int argc;
    unsigned int arg1, arg2;

    if(trap == 1)
    {
        trap = 0;
        goto L_enterconsole;
    }
    /* search break points */
    if(bp_search(bpoints, gp_reg->reg[31]))
        goto L_enterconsole;
    
    return;
    
  L_enterconsole:
    
    printf("pc = 0x%08x\n", gp_reg->reg[31]);
    
  L_getinput:
    printf("(console) ");
    getline(&temp, &bufsize, stdin);
    
    switch(uinput[0])
    {
        case 'b':               /* set break points */                
            if(bp_count < BP_MAX)
            {
                if(sscanf(uinput + 1, "%x", &arg1) == 0)
                    arg1 = gp_reg->reg[31];
                if(bp_add(&bpoints, arg1))
                    printf("break point added: 0x%08x\n", arg1);
                else
                    printf("a break point has already been set at 0x%08x\n", arg1);
            }
            else
                puts("break point slot full.");
            goto L_getinput;
        case 'd':
            if(sscanf(uinput + 1, "%x", &arg1) == 0)
                puts("bad command.");
            else if(bp_del(&bpoints, arg1))
                printf("break ponit deleted: 0x%08x\n", arg1);
            else
                printf("no such a point: 0x%08x\n", arg1);
            goto L_getinput;
        case 'q':
            puts("sim quit");
            exit(0);
        case 'c':
            return;
        case 'n':
            trap = 1;
            return;
        case 'p':
            switch(uinput[1])
            {
                case 'r':       /* print regfile */
                    pregs();
                    goto L_getinput;
                case 'p':       /* print pipeline state */
                    ppipe();
                    goto L_getinput;
                case 'f':
                    pfwd();
                    goto L_getinput;
                case 's':       /* print stage */
                    argc = sscanf(uinput + 2, "%x %x", &arg1, &arg2);
                    if(argc == 0)
                        puts("print stack: no address given.\n");
                    if(argc == 1)
                        pstack(arg1 & 0xfffffffc);
                    else if(argc == 2)
                        pstack_range(arg1 & 0xfffffffc, arg2 & 0xfffffffc);
                    else
                        puts("bad command.");
                    goto L_getinput;
                default:
                    puts("bad command.");
                    goto L_getinput;
            }
        case 'r':
            puts("restart simulation?(y/N)");
            switch(getchar())
            {
                case 'y':
                case 'Y':
                    trap = 1;   /* static */
                    flush_stdin();
                    longjmp(beginning, 1); /* restart */
                    break;
                default:
                    flush_stdin();
                    goto L_getinput;
            }
        default:
            puts("bad command.");
            goto L_getinput;
    }
    return;
}
