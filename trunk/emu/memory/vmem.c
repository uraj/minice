#include <memory/vmem.h>
#include <stdlib.h>
#include <string.h>

int vmem_read(uint32_t addr, uint32_t * dest)
{
    if(L1PageTable[addr >> 22] == NULL)
        return 1;
    PTelem * ptelem = &L1PageTable[addr >> 22][(addr >> 12) & 0x3ff];
    if(ptelem->flag == NAPage)
        return 1;
    if(ptelem->flag == XPage)
        return 2;
    *dest = ((uint32_t *)(ptelem->pageref))[addr & 0xfff];
    return 0;
}

int vmem_write(uint32_t addr, uint32_t data)
{
    int L1PTindex = addr >> 22;
    PTelem * ptelem;
    if(L1PageTable[L1PTindex] == NULL)
    {
        L1PageTable[L1PTindex] = calloc(sizeof(PTelem), L1PTSIZE);
        ptelem = &L1PageTable[L1PTindex][(addr >> 12) & 0x3ff];
        ptelem->flag = RWPage;
        ptelem->pageref = malloc(VMEMPAGE_SIZE);
        ((uint32_t *)(ptelem->pageref))[addr & 0xfff] = data;
        return 0;
    }
    else
    {
        ptelem = &L1PageTable[L1PTindex][(addr >> 12) & 0x3ff];
        if(ptelem->flag == NAPage)
        {
            ptelem->flag = RWPage;
            ptelem->pageref = malloc(VMEMPAGE_SIZE);
            ((uint32_t *)(ptelem->pageref))[addr & 0xfff] = data;
            return 0;
        }
        else if(ptelem->flag == RWPage)
        {
            ((uint32_t *)(ptelem->pageref))[addr & 0xfff] = data;
            return 0;
        }
        else
            return 1;
    }
    return 0;
}

void vmem_load(uint32_t addr, size_t size, const uint32_t * source, PageAttr flag)
{
    int L1PTindex, L2PTindex;
    PTelem * ptelem;
    int wsize;
    while(size > 0)
    {
        /* cal write size */
        L1PTindex = addr >> 22;
        L2PTindex = (addr >> 12) & 0x3ff;
        wsize = VMEMPAGE_SIZE - (addr % VMEMPAGE_SIZE);
        wsize = size < wsize ? size : wsize;
        /* allocate new page */
        L1PageTable[L1PTindex] = calloc(sizeof(PTelem), L2PTSIZE);
        ptelem = L1PageTable[L1PTindex] + L2PTindex;
        ptelem->flag = flag;
        ptelem->pageref = malloc(VMEMPAGE_SIZE);
        /* load data */
        memcpy(ptelem->pageref, source, wsize);
        addr += wsize;
        source += wsize;
        size -= wsize;
    }
    return;
}

void vmem_free()
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
