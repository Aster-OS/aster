#pragma once

#include <limine.h>

void kprintf_init(struct limine_framebuffer *fb);
int kprintf(const char* format, ...);
