#pragma once

#include "dev/tty/tty.h"
#include "limine.h"

struct tty_t *flanterm_tty_init(struct limine_framebuffer *fb);
