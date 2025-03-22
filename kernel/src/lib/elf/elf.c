#include <stddef.h>

#include "klog/klog.h"
#include "lib/elf/elf.h"
#include "lib/strutil.h"

#define EI_NIDENT 16

typedef struct __attribute__((packed)) {
    unsigned char e_ident[EI_NIDENT];
    Elf64_Half e_type;
    Elf64_Half e_machine;
    Elf64_Word e_version;
    Elf64_Addr e_entry;
    Elf64_Off  e_phoff;
    Elf64_Off  e_shoff;
    Elf64_Word e_flags;
    Elf64_Half e_ehsize;
    Elf64_Half e_phentsize;
    Elf64_Half e_phnum;
    Elf64_Half e_shentsize;
    Elf64_Half e_shnum;
    Elf64_Half e_shstrndx;
} Elf64_Ehdr;

void elf_find_section(void *file, char *search_name, Elf64_Shdr **shdr, Elf64_Half *shndx) {
    Elf64_Ehdr *elf_hdr = (Elf64_Ehdr *) file;
    Elf64_Shdr *shstrtab_hdr = (Elf64_Shdr *)
        ((uintptr_t) file + elf_hdr->e_shoff + elf_hdr->e_shentsize * elf_hdr->e_shstrndx);

    Elf64_Shdr *section_hdr = (Elf64_Shdr *) ((uintptr_t) file + elf_hdr->e_shoff);

    for (Elf64_Half i = 0; i < elf_hdr->e_shnum; i++) {
        char *section_name = (char *) ((uintptr_t) file + shstrtab_hdr->sh_offset + section_hdr->sh_name);
        if (strcmp(section_name, search_name) == 0) {
            if (shdr) *shdr = section_hdr;
            if (shndx) *shndx = i;
            return;
        }

        section_hdr++;
    }

    if (shdr) *shdr = NULL;
    if (shndx) *shndx = 0;
}
