#pragma once

void gdt_init(void);
void gdt_reload_segments(void);
void gdt_reload_tss(void);
