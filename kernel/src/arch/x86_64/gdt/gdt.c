#include "arch/x86_64/gdt/gdt.h"

#include <stdint.h>

#include "arch/x86_64/gdt/gdt-sel.h"
#include "klog/klog.h"
#include "lib/compiler.h"
#include "lib/spinlock/spinlock.h"
#include "mp/cpu.h"

extern void gdt_reload_seg_internal(void *gdtr);

static uint8_t const GDT_DESC_TYPE_CODE =
    1 << 7 | 1 << 4 | 1 << 3 | 1 << 1 | 1 << 0;
static uint8_t const GDT_DESC_TYPE_DATA = 1 << 7 | 1 << 4 | 1 << 1 | 1 << 0;
static uint8_t const GDT_DESC_TYPE_TSS_AVL = 1 << 7 | 1 << 3 | 1 << 0;

static uint8_t const GDT_DESC_FLAG_LONG_MODE = 1 << 5;

static inline uint8_t GDT_DPL(uint8_t dpl) {
    return dpl << 5;
}

struct seg_descriptor_t {
    uint16_t limit_0_15;
    uint16_t base_0_15;
    uint8_t base_16_23;
    uint8_t type;
    uint8_t limit_16_19_and_flags;
    uint8_t base_24_31;
} ASTER_PACKED;

struct tss_descriptor_t {
    uint16_t limit_0_15;
    uint16_t base_0_15;
    uint8_t base_16_23;
    uint8_t type;
    uint8_t limit_16_19_and_flags;
    uint8_t base_24_31;
    uint32_t base_32_63;
    uint32_t reserved;
} ASTER_PACKED;

struct gdtr_t {
    uint16_t limit;
    uint64_t base;
} ASTER_PACKED;

// null, kcode, kdata, ucode, udata, tss (counts as 2 entries)
static struct seg_descriptor_t gdt[7] ASTER_ALIGNED(8);
static uint8_t gdt_index;

static struct gdtr_t gdtr;

static void gdt_add_seg_descriptor(uint8_t type, uint8_t flags) {
    struct seg_descriptor_t *desc = &gdt[gdt_index];

    desc->limit_0_15 = 0;
    desc->base_0_15 = 0;
    desc->base_16_23 = 0;
    desc->type = type;
    desc->limit_16_19_and_flags = flags;
    desc->base_24_31 = 0;

    gdt_index++;
}

static void gdt_set_tss_descriptor(struct tss_t *tss) {
    struct tss_descriptor_t *tss_desc =
        (struct tss_descriptor_t *) &gdt[gdt_index];

    uint64_t tss_base = (uint64_t) tss;
    uint32_t tss_limit = sizeof(struct tss_t) - 1;

    tss_desc->limit_0_15 = tss_limit;
    tss_desc->base_0_15 = tss_base & 0xffff;
    tss_desc->base_16_23 = (tss_base >> 16) & 0xff;
    tss_desc->type = GDT_DESC_TYPE_TSS_AVL;
    tss_desc->limit_16_19_and_flags = (tss_limit >> 16) & 0xf;
    tss_desc->base_24_31 = (tss_base >> 24) & 0xff;
    tss_desc->base_32_63 = tss_base >> 32;
    tss_desc->reserved = 0;
}

void gdt_init(void) {
    gdt_add_seg_descriptor(0, 0);
    gdt_add_seg_descriptor(GDT_DESC_TYPE_CODE | GDT_DPL(0),
                           GDT_DESC_FLAG_LONG_MODE);
    gdt_add_seg_descriptor(GDT_DESC_TYPE_DATA | GDT_DPL(0),
                           GDT_DESC_FLAG_LONG_MODE);
    gdt_add_seg_descriptor(GDT_DESC_TYPE_CODE | GDT_DPL(3),
                           GDT_DESC_FLAG_LONG_MODE);
    gdt_add_seg_descriptor(GDT_DESC_TYPE_DATA | GDT_DPL(3),
                           GDT_DESC_FLAG_LONG_MODE);

    gdtr.base = (uint64_t) &gdt, gdtr.limit = sizeof(gdt) - 1;

    klog_info("GDT initialized");
}

void gdt_reload_seg(void) {
    gdt_reload_seg_internal(&gdtr);
}

void gdt_reload_tss(void) {
    // not touched by interrupts
    static struct spinlock_t tss_desc_lock = SPINLOCK_STATIC_INIT;
    spin_lock(&tss_desc_lock);

    get_cpu()->tss.iopb_offset = sizeof(struct tss_t);
    gdt_set_tss_descriptor(&get_cpu()->tss);

    __asm__ volatile("ltr %%ax;" : : "a"(GDT_SEL_TSS));

    spin_unlock(&tss_desc_lock);
}
