#include "fs/ustar.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "lib/align.h"
#include "lib/compiler.h"
#include "lib/memutil.h"
#include "lib/nanoprintf/nanoprintf.h"
#include "lib/strutil.h"

static size_t const USTAR_BLOCK_SIZE = 512;

struct ustar_hdr_t {
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char chksum[8];
    char typeflag;
    char linkname[100];
    char magic[6];
    char version[2];
    char uname[32];
    char gname[32];
    char devmajor[8];
    char devminor[8];
    char prefix[155];
} ASTER_PACKED;

enum ustar_hdr_typeflag {
    USTAR_REGULAR_A = '0',
    USTAR_REGULAR_B = '\0',
    USTAR_LINK = '1',
    USTAR_SYM = '2',
    USTAR_CHAR = '3',
    USTAR_BLOCK = '4',
    USTAR_DIR = '5',
    USTAR_FIFO = '6'
};

static uint64_t oct2int(char const *str, size_t len) {
    uint64_t n = 0;
    while (len > 0) {
        n = (n << 3) + *str - '0';
        str++;
        len--;
    }
    return n;
}

static bool ustar_hdr_valid(struct ustar_hdr_t *hdr) {
    return kmemcmp(&hdr->magic, "ustar", 5) == 0;
}

static char const *skip_prefix(char const *path) {
    if (path[0] == '.' && path[1] == '/') {
        return path + 2;
    }

    if (path[0] == '/') {
        return path + 1;
    }

    return path;
}

void ustar_open(void *address, size_t size, char const *path,
                struct ustar_file_t *file) {
    size_t offset = 0;
    struct ustar_hdr_t *hdr = (struct ustar_hdr_t *) address;
    while (ustar_hdr_valid(hdr) && offset + USTAR_BLOCK_SIZE < size) {
        uint64_t file_size = oct2int(hdr->size, 11);

        char file_path[257];
        if (hdr->prefix[0] == 0) {
            npf_snprintf(file_path, sizeof(file_path), "%.100s", hdr->name);
        } else {
            npf_snprintf(file_path, sizeof(file_path), "%.155s/%.100s",
                         hdr->prefix, hdr->name);
        }
        file_path[256] = 0;

        if (kstrcmp(skip_prefix(file_path), skip_prefix(path)) == 0) {
            file->address =
                (void *) ((uintptr_t) address + offset + USTAR_BLOCK_SIZE);
            file->size = size;
            return;
        }

        offset += USTAR_BLOCK_SIZE + align_up(file_size, USTAR_BLOCK_SIZE);
        hdr = (struct ustar_hdr_t *) ((uintptr_t) address + offset);
    }

    file->address = NULL;
    file->size = 0;
}
