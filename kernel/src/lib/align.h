#pragma once

#define ALIGN_DOWN(x, align) ((x) / (align) * (align))
#define ALIGN_UP(x, align) (((x) + (align) - 1) / (align) * (align))
#define DIV_ALIGN_UP(x, y) (((x) + (y) - 1) / (y))
