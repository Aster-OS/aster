#include "dev/tty/flanterm.h"

#include <stdbool.h>
#include <stddef.h>

#include "dev/tty/tty.h"
#include "kassert/kassert.h"
#include "lib/flanterm/backends/fb.h"
#include "lib/flanterm/flanterm.h"
#include "limine.h"

static struct flanterm_context *ft_ctx;
static struct tty_t flanterm_tty;

static void flush(void) {
    ft_ctx->double_buffer_flush(ft_ctx);
}

static void putchar(char c) {
    flanterm_putchar(ft_ctx, c);
}

struct tty_t *flanterm_tty_init(struct limine_framebuffer *fb) {
    // clang-format off
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
    // clang-format on

    kassert(ft_ctx != NULL);

    flanterm_tty.do_flush = true;
    flanterm_tty.flush = flush;
    flanterm_tty.putchar = putchar;
    return &flanterm_tty;
}
