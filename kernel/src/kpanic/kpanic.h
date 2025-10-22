#pragma once

#include "arch/x86_64/interrupts/interrupts.h"

__attribute__((noreturn))
void kpanic(const char *reason, ...);

__attribute__((noreturn))
void kpanic_int_ctx(struct int_ctx_t *ctx, const char *reason, ...);
