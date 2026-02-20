#pragma once

#include "arch/x86_64/interrupts/interrupts.h"
#include "lib/compiler.h"

ASTER_NORETURN
void kpanic(const char *reason, ...);

ASTER_NORETURN
void kpanic_int_ctx(struct int_ctx_t *ctx, const char *reason, ...);
