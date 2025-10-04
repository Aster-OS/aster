#include <stddef.h>
#include <stdint.h>

#include "arch/x86_64/asm_wrappers.h"
#include "dev/tty/serial.h"

static struct tty_t serial_tty;

static const uint16_t SERIAL_PORT = 0x3f8;

static bool is_transmit_empty(void) {
    return (inb(SERIAL_PORT + 5) & 0x20) >> 5;
}

static void putchar(char c) {
    while (!is_transmit_empty());
    outb(SERIAL_PORT, c);
}

struct tty_t *serial_tty_init(void) {
    outb(SERIAL_PORT + 1, 0x00);    // Disable all interrupts
    outb(SERIAL_PORT + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    outb(SERIAL_PORT + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
    outb(SERIAL_PORT + 1, 0x00);    //                  (hi byte)
    outb(SERIAL_PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(SERIAL_PORT + 2, 0xc7);    // Enable FIFO, clear them, with 14-byte threshold
    outb(SERIAL_PORT + 4, 0x0b);    // IRQs enabled, RTS/DSR set
    outb(SERIAL_PORT + 4, 0x1e);    // Set in loopback mode, test the serial chip

    outb(SERIAL_PORT + 0, 0xae);
    if (inb(SERIAL_PORT + 0) != 0xae) {
        return NULL;
    }

    // If serial is not faulty set it in normal operation mode
    // (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled)
    outb(SERIAL_PORT + 4, 0x0f);

    serial_tty.do_flush = false;
    serial_tty.putchar = putchar;
    return &serial_tty;
}
