#ifndef __MINISIM_ELFMANIP_H__
#define __MINISIM_ELFMANIP_H__

#include <elf.h>
#include <stdio.h>
#include <memory/vmem.h>

extern int elf_check(FILE * file);
extern Elf32_Ehdr get_elf_hdr(FILE * file);
extern void get_elf_phdr(Elf32_Phdr phdr[], Elf32_Ehdr hdr, FILE * file);
extern void load_elf_segments(FILE * file, Elf32_Ehdr ehdr);
extern uint32_t get_func_entry(FILE * file, Elf32_Ehdr ehdr, const char * funcname);

#endif
