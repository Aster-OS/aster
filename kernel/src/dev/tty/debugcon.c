#include <stddef.h>

#include "arch/x86_64/asm_wrappers.h"
#include "dev/tty/debugcon.h"

static struct tty_t debugcon_tty;

static uint16_t DEBUGCON_PORT = 0xe9;

static void putchar(char c) {
    outb(DEBUGCON_PORT, c);
}

struct tty_t *debugcon_tty_init(void) {
    debugcon_tty.do_flush = false;
    debugcon_tty.putchar = putchar;
    return &debugcon_tty;
}
