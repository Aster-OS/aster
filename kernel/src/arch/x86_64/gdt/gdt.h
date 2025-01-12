#pragma once

struct __attribute__((packed)) tss_t {
    uint32_t reserved0;
    uint64_t rsp[3];
    uint64_t reserved1;
    uint64_t ist[7];
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iopb_offset;
};

void gdt_init(void);
void gdt_reload_segments(void);
void gdt_reload_tss(void);
