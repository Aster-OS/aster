#include <stdint.h>

#include "arch/x86_64/gdt/gdt.h"
#include "arch/x86_64/gdt/gdt_selectors.h"
#include "klog/klog.h"

static uint8_t GDT_DESC_TYPE_CODE = 1 << 7 | 1 << 4 | 1 << 3 | 1 << 1 | 1 << 0;
static uint8_t GDT_DESC_TYPE_DATA = 1 << 7 | 1 << 4 | 1 << 1 | 1 << 0;
static uint8_t GDT_DESC_TYPE_TSS_AVL = 1 << 7 | 1 << 3 | 1 << 0;

static inline uint8_t gdt_desc_type_dpl(uint8_t dpl) {
    return dpl << 5;
}

static uint8_t GDT_DESC_FLAG_LONG_MODE = 1 << 5;

struct __attribute__((packed)) seg_descriptor_t {
    uint16_t limit_0_15;
    uint16_t base_0_15;
    uint8_t base_16_23;
    uint8_t type;
    uint8_t limit_16_19_and_flags;
    uint8_t base_24_31;
};

struct __attribute__((packed)) tss_descriptor_t {
    uint16_t limit_0_15;
    uint16_t base_0_15;
    uint8_t base_16_23;
    uint8_t type;
    uint8_t limit_16_19_and_flags;
    uint8_t base_24_31;
    uint32_t base_32_63;
    uint32_t reserved;
};

struct __attribute__((packed)) gdtr_t {
    uint16_t limit;
    uint64_t base;
};

struct __attribute__((packed)) tss_t {
    uint32_t reserved0;
    uint64_t rsp[3];
    uint64_t reserved1;
    uint64_t ist[7];
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iopb_offset;
};

// null, kcode, kdata, ucode, udata, tss (counts as 2 entries)
static __attribute__((aligned(8))) struct seg_descriptor_t gdt[7];
static uint8_t last_used_gdt_index;

static struct tss_t tss;

static void gdt_add_seg_descriptor(uint8_t type, uint8_t flags) {
    struct seg_descriptor_t *seg_descriptor = &gdt[last_used_gdt_index];

    seg_descriptor->limit_0_15 = 0;
    seg_descriptor->base_0_15 = 0;
    seg_descriptor->base_16_23 = 0;
    seg_descriptor->type = type;
    seg_descriptor->limit_16_19_and_flags = flags;
    seg_descriptor->base_24_31 = 0;

    last_used_gdt_index++;
}

static void gdt_set_tss_descriptor(struct tss_t *tss_ptr) {
    // place the TSS descriptor after the segment descriptors
    struct tss_descriptor_t *tss_descriptor = (struct tss_descriptor_t *) &gdt[last_used_gdt_index];

    uint64_t tss_descriptor_base = (uint64_t) tss_ptr;
    uint32_t tss_descriptor_limit = sizeof(struct tss_t) - 1;

    tss_descriptor->limit_0_15 = tss_descriptor_limit;
    tss_descriptor->base_0_15 = tss_descriptor_base & 0xffff;
    tss_descriptor->base_16_23 = (tss_descriptor_base >> 16) & 0xff;
    tss_descriptor->type = GDT_DESC_TYPE_TSS_AVL;
    tss_descriptor->limit_16_19_and_flags = (tss_descriptor_limit >> 16) & 0xf;
    tss_descriptor->base_24_31 = (tss_descriptor_base >> 24) & 0xff;
    tss_descriptor->base_32_63 = tss_descriptor_base >> 32;
    tss_descriptor->reserved = 0;
}

void gdt_init(void) {
    // 0x00 - null descriptor
    gdt_add_seg_descriptor(0, 0);

    // 0x08 - kernel code
    gdt_add_seg_descriptor(GDT_DESC_TYPE_CODE | gdt_desc_type_dpl(0), GDT_DESC_FLAG_LONG_MODE);

    // 0x10 - kernel data
    gdt_add_seg_descriptor(GDT_DESC_TYPE_DATA | gdt_desc_type_dpl(0), GDT_DESC_FLAG_LONG_MODE);

    // 0x18 - user code
    gdt_add_seg_descriptor(GDT_DESC_TYPE_CODE | gdt_desc_type_dpl(3), GDT_DESC_FLAG_LONG_MODE);

    // 0x20 - user data
    gdt_add_seg_descriptor(GDT_DESC_TYPE_DATA | gdt_desc_type_dpl(3), GDT_DESC_FLAG_LONG_MODE);

    tss.iopb_offset = sizeof(struct tss_t); // we don't use the IOPB
    gdt_set_tss_descriptor(&tss);

    struct gdtr_t gdtr = {
        .base = (uint64_t) &gdt,
        .limit = sizeof(gdt) - 1
    };

    __asm__ volatile(
        "lgdt %0;"
        "push %1;"
        "lea reload_data_segments(%%rip), %%rax;"
        "push %%rax;"
        "lretq;"
        "reload_data_segments:"
        "mov %2, %%ax;"
        "mov %%ax, %%ss;"
        "mov $0x0, %%ax;"
        "mov %%ax, %%ds;"
        "mov %%ax, %%es;"
        "mov %%ax, %%fs;"
        "mov %%ax, %%gs;"
        : : "m" (gdtr), "i" (GDT_SELECTOR_KERNEL_CODE), "i" (GDT_SELECTOR_KERNEL_DATA) : "%rax", "memory"
    );

    __asm__ volatile("ltr %%ax;" : : "a" (GDT_SELECTOR_TSS));

    klog_info("GDT initialized");
}
