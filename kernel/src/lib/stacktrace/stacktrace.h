#pragma once

#include "arch/x86_64/interrupts/interrupts.h"

void stacktrace(struct int_ctx_t *ctx);
