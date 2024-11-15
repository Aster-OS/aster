#include <stdbool.h>
#include <stddef.h>

#include "limine.h"

__attribute__((used, section(".limine_requests")))
static volatile LIMINE_BASE_REVISION(3);

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request fb_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests_start")))
static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile LIMINE_REQUESTS_END_MARKER;

static void halt(void) {
    asm volatile("hlt");
}

void kmain(void) {
    if (LIMINE_BASE_REVISION_SUPPORTED == false) {
        halt();
    }

    if (fb_request.response == NULL || fb_request.response->framebuffer_count < 1) {
        halt();
    }

    struct limine_framebuffer *fb = fb_request.response->framebuffers[0];

    for (size_t i = 0; i < 100; i++) {
        volatile uint32_t *fb_ptr = fb->address;
        fb_ptr[i * (fb->pitch / 4) + i] = 0x00ffff;
    }

    halt();
}
