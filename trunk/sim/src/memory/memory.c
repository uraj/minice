#include <memory/memory.h>
#include <stdio.h>

int mem_read_instruction(uint32_t addr, uint32_t * instr)
{
    int cachemiss = 0;
    if(mem_read_direct_w(addr, instr))
        return -1;
#ifdef DEBUG
    printf("Fetch instruction 0x%08x, pc = 0x%08x\n", *instr, addr);
#endif
    return cachemiss;
}

int mem_read(uint32_t addr, size_t size, int sign_ext, uint32_t * data)
{
    int cachemiss = 0;          /* cache related */
    if((size == 4) && (addr % 4 == 0))
    {    
        if(mem_read_direct_w(addr, data))
            return -1;
#ifdef DEBUG
        printf("Read word 0x%08x, addr = 0x%08x\n", *data, addr);
#endif
    }
    else if((size == 2) && (addr % 2 == 0))
    {
        uint16_t data_16;
        if(mem_read_direct_h(addr, &data_16))
            return -1;
        if(sign_ext && ((int16_t)data_16 < 0))
            *data = (uint32_t)data_16 | 0xffff0000;
        else
            *data = (uint32_t)data_16;
#ifdef DEBUG
        printf("Read halfword 0x%04x, addr = 0x%08x\n", *data, addr);
#endif
    }
    else if(size == 1)
    {
        uint8_t data_8;
        if(mem_read_direct_b(addr, &data_8))
            return -1;
        if(sign_ext && ((int8_t)data_8 < 0))
            *data = (uint32_t)data_8 | 0xffffff00;
        else
            *data = (uint32_t)data_8;
#ifdef DEBUG
        printf("Read byte 0x%02x, addr = 0x%08x\n", *data, addr);
#endif
    }
    else
        return -1;
    
    return cachemiss;
}

int mem_write(uint32_t addr, size_t size, uint32_t data)
{
    int writeback = 0;          /* cache related */
    if((size == 4) && (addr % 4 == 0))
    {
        if(mem_write_direct_w(addr, data))
            return -1;
#ifdef DEBUG
        printf("Write word 0x%08x, addr = 0x%08x\n", data, addr);
#endif
    }
    else if((size == 2) && (addr % 2 == 0))
    {
        if(mem_write_direct_h(addr, (uint16_t)data))
            return -1;
#ifdef DEBUG
        printf("Write halfword 0x%04x, addr = 0x%08x\n", data, addr);
#endif
    }
    else if(size == 1)
    {
        if(mem_write_direct_b(addr, (uint16_t)data))
            return -1;
#ifdef DEBUG
        printf("Write byte 0x%02x, addr = 0x%08x\n", data, addr);
#endif
    }   
    else
        return -1;
    
    return writeback;
}
