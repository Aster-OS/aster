#pragma once

#include <stdint.h>

#define IDT_MAX_DESCRIPTORS 256

void idt_init(void);
void idt_reload(void);
void idt_set_user(uint8_t vec);
