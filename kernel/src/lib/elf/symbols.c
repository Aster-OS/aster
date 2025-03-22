#include <stddef.h>

#include "klog/klog.h"
#include "lib/elf/elf.h"
#include "lib/elf/symbols.h"
#include "memory/kmalloc/kmalloc.h"

struct symbol_t {
    void *addr;
    char *name;
};

static Elf64_Shdr *strtab_hdr;
static Elf64_Shdr *symtab_hdr;
static bool symbols_initialized;
static struct symbol_t *funcs;
static size_t funcs_count;

extern unsigned char __TEXT_MAX_ADDR[];

char *symbols_get_func_name(void *addr) {
    if (!symbols_initialized || addr < funcs[0].addr || (uintptr_t) addr >= (uintptr_t) &__TEXT_MAX_ADDR) {
        goto unknown_func;
    }

    size_t i = 0;
    while (i < funcs_count && funcs[i].addr <= addr) {
        i++;
    }

    struct symbol_t prev = funcs[i - 1];
    if (prev.addr <= addr) {
        return prev.name;
    }

unknown_func:
    return "unknown function";
}

void symbols_init(void *file) {
    Elf64_Half text_shndx;
    elf_find_section(file, ".text", NULL, &text_shndx);
    elf_find_section(file, ".strtab", &strtab_hdr, NULL);
    elf_find_section(file, ".symtab", &symtab_hdr, NULL);

    if (text_shndx == 0 || strtab_hdr == NULL || symtab_hdr == NULL) {
        klog_warn("Could not get debug symbols");
        return;
    }

    Elf64_Xword symbol_count = symtab_hdr->sh_size / symtab_hdr->sh_entsize;

    Elf64_Sym *sym = (Elf64_Sym *) ((uintptr_t) file + symtab_hdr->sh_offset);
    for (Elf64_Xword i = 0; i < symbol_count; i++, sym++) {
        if (sym->st_shndx == text_shndx) {
            funcs_count++;
        }
    }

    funcs = (struct symbol_t *) kmalloc(funcs_count * sizeof(struct symbol_t));

    sym = (Elf64_Sym *) ((uintptr_t) file + symtab_hdr->sh_offset);
    size_t funcs_index = 0;
    for (Elf64_Xword i = 0; i < symbol_count; i++, sym++) {
        if (sym->st_shndx != text_shndx) {
            continue;
        }

        struct symbol_t *func = &funcs[funcs_index++];
        func->addr = (void *) sym->st_value;
        func->name = (char *) ((uintptr_t) file + strtab_hdr->sh_offset + sym->st_name);
    }

    for (size_t i = 0; i < funcs_count - 1; i++) {
        for (size_t j = i; j < funcs_count; j++) {
            if (funcs[i].addr > funcs[j].addr) {
                struct symbol_t tmp = funcs[i];
                funcs[i] = funcs[j];
                funcs[j] = tmp;
            }
        }
    }

    symbols_initialized = true;
    klog_info("Symbols initialized");
}
