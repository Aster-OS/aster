#include "lib/flanterm/flanterm.h"
#include "lib/flanterm/backends/fb.h"
#include "lib/printf/printf.h"
#include "kprintf/kprintf.h"

static struct flanterm_context *ft_ctx;

void kprintf_init(struct limine_framebuffer *fb) {
    ft_ctx = flanterm_fb_init(
        NULL,
        NULL,
        fb->address, fb->width, fb->height, fb->pitch,
        fb->red_mask_size, fb->red_mask_shift,
        fb->green_mask_size, fb->green_mask_shift,
        fb->blue_mask_size, fb->blue_mask_shift,
        NULL,
        NULL, NULL,
        NULL, NULL,
        NULL, NULL,
        NULL, 0, 0, 1,
        0, 0,
        0
    );
}

void _putchar(char character) {
    // we made flanterm_putchar available globally
    flanterm_putchar(ft_ctx, character);
}

// kernel printf based on flanterm and mpaland/printf
// flanterm flushes the buffer after every flanterm_write
// so we built a wrapper for printf which calls vprintf (which calls _putchar)
// after printing, we flush the terminal just like in flanterm_write
int kprintf(const char* format, ...) {
    va_list va;
    va_start(va, format);
    const int ret = vprintf_(format, va);
    va_end(va);

    if (ft_ctx->autoflush) {
        ft_ctx->double_buffer_flush(ft_ctx);
    }

    return ret;
}

int kvprintf(const char* format, va_list va) {
    const int ret = vprintf_(format, va);

    if (ft_ctx->autoflush) {
        ft_ctx->double_buffer_flush(ft_ctx);
    }

    return ret;
}
