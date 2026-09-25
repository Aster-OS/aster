#include "userspace/elf.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "fs/initrd.h"
#include "fs/ustar.h"
#include "klog/klog.h"
#include "lib/align.h"
#include "lib/compiler.h"
#include "lib/math.h"
#include "lib/memutil.h"
#include "memory/pmm/pmm.h"
#include "memory/vmm/vmm.h"
#include "sched/proc.h"
#include "sched/sched.h"

typedef uint64_t Elf64_Addr;
typedef uint64_t Elf64_Off;
typedef uint16_t Elf64_Half;
typedef uint32_t Elf64_Word;
typedef int32_t Elf64_Sword;
typedef uint64_t Elf64_Xword;
typedef int64_t Elf64_Sxword;

#define EI_MAG0       0
#define EI_MAG1       1
#define EI_MAG2       2
#define EI_MAG3       3
#define EI_CLASS      4
#define EI_DATA       5
#define EI_VERSION    6
#define EI_OSABI      7
#define EI_ABIVERSION 8
#define EI_NIDENT     16

#define ELFMAG0 0x7f
#define ELFMAG1 'E'
#define ELFMAG2 'L'
#define ELFMAG3 'F'

#define ELFCLASS64 2

#define ELFDATA2LSB 1

#define ELFOSABI_NONE 0

#define EM_X86_64 62

#define ET_NONE 0
#define ET_REL  1
#define ET_EXEC 2
#define ET_DYN  3
#define ET_CORE 4

#define PT_NULL    0
#define PT_LOAD    1
#define PT_DYNAMIC 2
#define PT_INTERP  3
#define PT_NOTE    4
#define PT_SHLIB   5
#define PT_PHDR    6
#define PT_TLS     7

#define PF_X 0x1
#define PF_W 0x2
#define PF_R 0x4

typedef struct {
    unsigned char e_ident[EI_NIDENT];
    Elf64_Half e_type;
    Elf64_Half e_machine;
    Elf64_Word e_version;
    Elf64_Addr e_entry;
    Elf64_Off e_phoff;
    Elf64_Off e_shoff;
    Elf64_Word e_flags;
    Elf64_Half e_ehsize;
    Elf64_Half e_phentsize;
    Elf64_Half e_phnum;
    Elf64_Half e_shentsize;
    Elf64_Half e_shnum;
    Elf64_Half e_shstrndx;
} ASTER_PACKED Elf64_Ehdr;

typedef struct {
    Elf64_Word p_type;
    Elf64_Word p_flags;
    Elf64_Off p_offset;
    Elf64_Addr p_vaddr;
    Elf64_Addr p_paddr;
    Elf64_Xword p_filesz;
    Elf64_Xword p_memsz;
    Elf64_Xword p_align;
} ASTER_PACKED Elf64_Phdr;

typedef struct {
    Elf64_Word sh_name;
    Elf64_Word sh_type;
    Elf64_Xword sh_flags;
    Elf64_Addr sh_addr;
    Elf64_Off sh_offset;
    Elf64_Xword sh_size;
    Elf64_Word sh_link;
    Elf64_Word sh_info;
    Elf64_Xword sh_addralign;
    Elf64_Xword sh_entsize;
} ASTER_PACKED Elf64_Shdr;

static void ASTER_USED print_ehdr(Elf64_Ehdr *ehdr) {
    klog_info("EHDR entry %x, flags %x, ehsize %x, shstrndx %x, phoff %x, "
              "phentsize %x, phnum %x, shoff %x, shentsize %x, shnum %x",
              ehdr->e_entry, ehdr->e_flags, ehdr->e_ehsize, ehdr->e_shstrndx,
              ehdr->e_phoff, ehdr->e_phentsize, ehdr->e_phnum, ehdr->e_shoff,
              ehdr->e_shentsize, ehdr->e_shnum);
}

static void ASTER_USED print_phdr(Elf64_Phdr *phdr) {
    klog_info("PHDR type %x, flags %x, offset %x, vaddr %x, filesz "
              "%x, memsz %x, align %x, base %x, size %x",
              phdr->p_type, phdr->p_flags, phdr->p_offset, phdr->p_vaddr,
              phdr->p_filesz, phdr->p_memsz, phdr->p_align);
}

bool elf_load(char const *path) {
    struct ustar_file_t file;
    initrd_open(path, &file);

    if (sizeof(Elf64_Ehdr) > file.size) {
        return false;
    }

    uint8_t *file_ptr = file.address;

    Elf64_Ehdr *ehdr = (Elf64_Ehdr *) file_ptr;

    if (file.size < ehdr->e_phoff + ehdr->e_phnum * ehdr->e_phentsize ||
        file.size < ehdr->e_shoff + ehdr->e_shnum * ehdr->e_shentsize) {
        return false;
    }

    if (ehdr->e_ident[EI_MAG0] != ELFMAG0 ||
        ehdr->e_ident[EI_MAG1] != ELFMAG1 ||
        ehdr->e_ident[EI_MAG2] != ELFMAG2 ||
        ehdr->e_ident[EI_MAG3] != ELFMAG3 ||
        ehdr->e_ident[EI_CLASS] != ELFCLASS64 ||
        ehdr->e_ident[EI_DATA] != ELFDATA2LSB ||
        ehdr->e_ident[EI_VERSION] != 1 ||
        ehdr->e_ident[EI_OSABI] != ELFOSABI_NONE || ehdr->e_type != ET_EXEC ||
        ehdr->e_machine != EM_X86_64 || ehdr->e_entry == 0 ||
        ehdr->e_phoff == 0) {
        return false;
    }

    struct pagemap_t *pagemap = vmm_new_user_pagemap();

    Elf64_Phdr *phdr = (Elf64_Phdr *) (file_ptr + ehdr->e_phoff);
    for (Elf64_Half i = 0; i < ehdr->e_phnum; i++, phdr++) {
        if (phdr->p_type != PT_LOAD) {
            continue;
        }

        vmm_flags_t flags = VMM_USER;
        if (phdr->p_flags & PF_W) {
            flags |= VMM_WRITE;
        }
        if (phdr->p_flags & PF_X) {
            flags |= VMM_EXEC;
        }

        uintptr_t mem_begin = phdr->p_vaddr;
        uintptr_t mem_end = phdr->p_vaddr + phdr->p_memsz;
        uintptr_t file_begin = phdr->p_vaddr;
        uintptr_t file_end = phdr->p_vaddr + phdr->p_filesz;

        if (phdr->p_vaddr == 0) {
            return false;
        }

        if (phdr->p_filesz > phdr->p_memsz) {
            return false;
        }

        if (mem_end < mem_begin) {
            return false;
        }

        if (mem_begin == 0 || mem_end > VMM_LOWER_HALF_MAX) {
            return false;
        }

        size_t sum;
        if (__builtin_add_overflow(phdr->p_offset, phdr->p_filesz, &sum) ||
            sum > file.size) {
            return false;
        }

        uintptr_t vaddr_begin = align_down(mem_begin, PAGE_SIZE);
        uintptr_t vaddr_end = align_up(mem_end, PAGE_SIZE);

        for (uintptr_t vaddr = vaddr_begin; vaddr < vaddr_end;
             vaddr += PAGE_SIZE) {
            phys_t paddr = pmm_alloc(false);
            vmm_map_page(pagemap, vaddr, paddr, flags);

            uint8_t *hhdm_ptr = (uint8_t *) (paddr + vmm_hhdm());

            if (vaddr == vaddr_begin) {
                uintptr_t padding = mem_begin - vaddr_begin;
                kmemset(hhdm_ptr, 0, padding);
            }

            if (vaddr == vaddr_end - PAGE_SIZE) {
                uintptr_t padding = vaddr_end - mem_end;
                kmemset(hhdm_ptr + PAGE_SIZE - padding, 0, padding);
            }

            uintptr_t copy_begin = max(vaddr, file_begin);
            uintptr_t copy_end = min(vaddr + PAGE_SIZE, file_end);
            if (copy_begin < copy_end) {
                uint8_t *dest = hhdm_ptr + copy_begin - vaddr;
                uint8_t *src =
                    file_ptr + phdr->p_offset + copy_begin - file_begin;
                size_t len = copy_end - copy_begin;
                kmemcpy(dest, src, len);
            }

            uintptr_t zero_begin = max(vaddr, file_end);
            uintptr_t zero_end = min(vaddr + PAGE_SIZE, mem_end);
            if (zero_begin < zero_end) {
                uint8_t *dest = hhdm_ptr + zero_begin - vaddr;
                size_t len = zero_end - zero_begin;
                kmemset(dest, 0, len);
            }
        }
    }

    struct proc_t *proc = sched_new_proc("user", pagemap);
    sched_new_thread(proc, (void *(*) (void *) ) ehdr->e_entry, NULL);

    return true;
}
