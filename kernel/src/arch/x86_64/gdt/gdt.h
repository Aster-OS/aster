#pragma once

void gdt_init(void);
void gdt_reload_seg(void);
void gdt_reload_tss(void);
