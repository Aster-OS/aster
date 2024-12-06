#pragma once

#include <stdarg.h>

#include "limine.h"

void kprintf_init(struct limine_framebuffer *fb);
int kprintf(const char* format, ...);
int kvprintf(const char* format, va_list va);
