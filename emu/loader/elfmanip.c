#include <loader/elfmanip.h>
#include <memory/vmem.h>
#include <stdint.h>
#include <elf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int elf_check(FILE * file)
{
     long int sindicator = ftell(file);
    rewind(file);
    unsigned char magic[16];
    size_t get = fread(magic, sizeof(unsigned char), 16, file);
    fseek(file, sindicator, SEEK_SET);
    if(get != 16)
        return 1;
    if((*(uint32_t*)magic != 0x464c457fU) ||
       (magic[4] != 1) ||
       (magic[5] != 1))
        return 1;
    return 0;
}

Elf32_Ehdr get_elf_hdr(FILE * file)
{
    rewind(file);
    Elf32_Ehdr ret;
    fread(&ret, sizeof(Elf32_Ehdr), 1, file);
    return ret;
}

void get_elf_phdr(Elf32_Phdr phdr[], Elf32_Ehdr ehdr, FILE * file)
{
    fseek(file, ehdr.e_phoff, SEEK_SET);
    fread(phdr, sizeof(Elf32_Phdr), ehdr.e_phnum, file);
    return;
}

void load_elf_segments(FILE * file, Elf32_Ehdr ehdr)
{
    Elf32_Phdr * phdrtable = NULL;
    int i, size;
    void * buffer;
    PageAttr flag;
    phdrtable = malloc(sizeof(Elf32_Phdr) * ehdr.e_phnum *2);
    get_elf_phdr(phdrtable, ehdr, file);
    for(i = 0; i < ehdr.e_phnum; ++i)
    {
        if(phdrtable[i].p_type == PT_LOAD)
        {
            fseek(file, phdrtable[i].p_offset, SEEK_SET);
            size = phdrtable[i].p_filesz;
            buffer = malloc(size);
            fread(buffer, 1, size, file);
            switch(phdrtable[i].p_flags)
            {
                case PF_R:
                    flag = RPage;
                    break;
                case PF_X:
                case PF_R + PF_X:
                    flag = XPage;
                    break;
                case PF_W:
                case PF_R + PF_W:
                case PF_X + PF_W:
                case PF_R + PF_W + PF_X:
                    flag = RWPage;
                    break;
                default:
                    exit(1);
            }
            mem_load(phdrtable[i].p_vaddr, size, buffer, flag);
            free(buffer);
        }
    }
    free(phdrtable);
    return;
}

uint32_t get_func_entry(FILE * file, Elf32_Ehdr ehdr, const char * funcname)
{
    int i, found;
    Elf32_Shdr strshdr, symshdr;
    char * strbuff = NULL;
    uint32_t ret = 0;
    
    /* get section header of .shstrtab */
    fseek(file, ehdr.e_shoff + ehdr.e_shstrndx * sizeof(Elf32_Shdr), SEEK_SET);
    fread(&strshdr, 1, sizeof(Elf32_Shdr), file);
    
    /* get section .shstrtab itself */
    strbuff = malloc(sizeof(char) * strshdr.sh_size);
    fseek(file, strshdr.sh_offset, SEEK_SET);
    fread(strbuff, strshdr.sh_size, 1, file);
    
    /* scan all the sections to find ".symtab" and ".strtab" */
    fseek(file, ehdr.e_shoff, SEEK_SET);
    for(i = 0, found = 0; i < ehdr.e_shnum && found < 2; ++i)
    {
        Elf32_Shdr temp;
        fread(&temp, 1, sizeof(Elf32_Shdr), file);
        if((temp.sh_type == SHT_SYMTAB) &&
           (strcmp(strbuff + temp.sh_name, ".symtab") == 0))
            symshdr = temp, ++found;
        if((temp.sh_type == SHT_STRTAB) &&
           (strcmp(strbuff + temp.sh_name, ".strtab") == 0))
            strshdr = temp, ++found;
    }
    free(strbuff);

    /* scan all the .symtab section to get funcname */
    strbuff = malloc(strshdr.sh_size);
    fseek(file, strshdr.sh_offset, SEEK_SET);
    fread(strbuff, strshdr.sh_size, sizeof(char), file);
    fseek(file, symshdr.sh_offset, SEEK_SET);
    for(i = 0; i < symshdr.sh_size / symshdr.sh_entsize; ++i)
    {
        Elf32_Sym temp;
        fread(&temp, 1, sizeof(Elf32_Sym), file);
        if(strcmp(funcname, strbuff + temp.st_name) == 0)
        {
            ret = temp.st_value;
            break;
        }
    }
    free(strbuff);
    return ret;
}
