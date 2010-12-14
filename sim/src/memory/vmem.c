#include <memory/vmem.h>
#include <stdlib.h>
#include <string.h>

int mem_read_direct_b(uint32_t addr, uint8_t * dest)
{
    if(L1PageTable[addr >> 25] == NULL)
        return 1;
    PTelem * ptelem = &L1PageTable[addr >> 25][(addr >> 15) & 0x3ff];
    if(ptelem->flag == NAPage)
        return 1;
    if(ptelem->flag == XPage)
        return 2;
    *dest = *(uint8_t *)(ptelem->pageref + (addr & 0x7fff));
    return 0;
}

int mem_read_direct_h(uint32_t addr, uint16_t * dest)
{
    /* alignment */
    addr &= 0xfffffffe;
    
    if(L1PageTable[addr >> 25] == NULL)
        return 1;
    PTelem * ptelem = &L1PageTable[addr >> 25][(addr >> 15) & 0x3ff];
    if(ptelem->flag == NAPage)
        return 1;
    if(ptelem->flag == XPage)
        return 2;
    *dest = *(uint16_t *)(ptelem->pageref + (addr & 0x7fff));
    return 0;
}

int mem_read_direct_w(uint32_t addr, uint32_t * dest)
{
    /* alignment */
    addr &= 0xfffffffc;
    
    if(L1PageTable[addr >> 25] == NULL)
        return 1;
    PTelem * ptelem = &(L1PageTable[addr >> 25][(addr >> 15) & 0x3ff]);
    if(ptelem->flag == NAPage)
        return 1;
    *dest = *(uint32_t *)(ptelem->pageref + (addr & 0x7fff));
    return 0;
}

int mem_write_direct_b(uint32_t addr, uint8_t data_8)
{
    int L1PTindex = addr >> 25;
    PTelem * ptelem;
    if(L1PageTable[L1PTindex] == NULL)
    {
        L1PageTable[L1PTindex] = calloc(sizeof(PTelem), L2PTSIZE);
        ptelem = &L1PageTable[L1PTindex][(addr >> 15) & 0x3ff];
        ptelem->flag = RWPage;
        ptelem->pageref = malloc(VMEMPAGE_SIZE);
        *(uint8_t *)(ptelem->pageref + (addr & 0x7fff)) = data_8;
        return 0;
    }
    else
    {
        ptelem = &L1PageTable[L1PTindex][(addr >> 15) & 0x3ff];
        if(ptelem->flag == NAPage)
        {
            ptelem->flag = RWPage;
            ptelem->pageref = malloc(VMEMPAGE_SIZE);
            *(uint8_t *)(ptelem->pageref + (addr & 0x7fff)) = data_8;
            return 0;
        }
        else if(ptelem->flag == RWPage)
        {
            *(uint8_t *)(ptelem->pageref + (addr & 0x7fff)) = data_8;
            return 0;
        }
        else
            return 1;
    }
    return 0;
}

int mem_write_direct_h(uint32_t addr, uint16_t data_16)
{
    /* alignment */
    addr &= 0xfffffffe;
    
    int L1PTindex = addr >> 25;
    PTelem * ptelem;
    if(L1PageTable[L1PTindex] == NULL)
    {
        L1PageTable[L1PTindex] = calloc(sizeof(PTelem), L2PTSIZE);
        ptelem = &L1PageTable[L1PTindex][(addr >> 15) & 0x3ff];
        ptelem->flag = RWPage;
        ptelem->pageref = malloc(VMEMPAGE_SIZE);
        *(uint16_t *)(ptelem->pageref + (addr & 0x7fff)) = data_16;
        return 0;
    }
    else
    {
        ptelem = &L1PageTable[L1PTindex][(addr >> 15) & 0x3ff];
        if(ptelem->flag == NAPage)
        {
            ptelem->flag = RWPage;
            ptelem->pageref = malloc(VMEMPAGE_SIZE);
            *(uint16_t *)(ptelem->pageref + (addr & 0x7fff)) = data_16;
            return 0;
        }
        else if(ptelem->flag == RWPage)
        {
            *(uint16_t *)(ptelem->pageref + (addr & 0x7fff)) = data_16;
            return 0;
        }
        else
            return 1;
    }
    return 0;
}

int mem_write_direct_w(uint32_t addr, uint32_t data_32)
{
    /* alignment */
    addr &= 0xfffffffc;
    
    int L1PTindex = addr >> 25;
    PTelem * ptelem;
    if(L1PageTable[L1PTindex] == NULL)
    {
        L1PageTable[L1PTindex] = calloc(sizeof(PTelem), L2PTSIZE);
        ptelem = &L1PageTable[L1PTindex][(addr >> 15) & 0x3ff];
        ptelem->flag = RWPage;
        ptelem->pageref = malloc(VMEMPAGE_SIZE);
        *(uint32_t *)(ptelem->pageref + (addr & 0x7fff)) = data_32;
        return 0;
    }
    else
    {
        ptelem = &L1PageTable[L1PTindex][(addr >> 15) & 0x3ff];
        if(ptelem->flag == NAPage)
        {
            ptelem->flag = RWPage;
            ptelem->pageref = malloc(VMEMPAGE_SIZE);
            *(uint32_t *)(ptelem->pageref + (addr & 0x7fff)) = data_32;
            return 0;
        }
        else if(ptelem->flag == RWPage)
        {
            *(uint32_t *)(ptelem->pageref + (addr & 0x7fff)) = data_32;
            return 0;
        }
        else
            return 1;
    }
    return 0;
}

void mem_load(uint32_t addr, size_t size, const void * source, PageAttr flag)
{
    int L1PTindex, L2PTindex;
    int wsize;
    const void * temp = source;
    while(size > 0)
    {
        /* cal write size */
        L1PTindex = addr >> 25;
        L2PTindex = (addr >> 15) & 0x3ff;
        wsize = VMEMPAGE_SIZE - (addr % VMEMPAGE_SIZE);
        wsize = size < wsize ? size : wsize;
        /* allocate new page */
        if(L1PageTable[L1PTindex] == NULL)
            L1PageTable[L1PTindex] = calloc(sizeof(PTelem), L2PTSIZE);
        L1PageTable[L1PTindex][L2PTindex].flag = flag;
        L1PageTable[L1PTindex][L2PTindex].pageref = malloc(VMEMPAGE_SIZE);
        /* load data */
        memcpy(L1PageTable[L1PTindex][L2PTindex].pageref, temp, wsize);
        addr += wsize;
        temp += wsize;
        size -= wsize;
    }
    return;
}

void mem_free()
{
    int i, j;
    for(i = 0; i < L1PTSIZE; ++i)
    {
        if(L1PageTable[i] != NULL)
        {
            for(j = 0; j < L2PTSIZE; ++j)
                if(L1PageTable[i][j].flag != NAPage)
                    free(L1PageTable[i][j].pageref);
            free(L1PageTable[i]);
        }
    }
    return;
}
