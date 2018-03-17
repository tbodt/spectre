#include <stdio.h>
#include <sys/mman.h>
#include "utils.h"
#include "cache.h"

void evict(void *ptr) {
    static char *space = NULL;
    if (space == NULL) {
        space = mmap(NULL, 0x4000000, PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
        check("mmap eviction space");
    }

    unsigned long off = ((unsigned long) ptr) & 0xfff;
    volatile char *ptr1 = space + off;
    volatile char *ptr2 = ptr1 + 0x2000;
    for (int i = 0; i < 4000; i++) {
        *ptr2;
        *ptr1;
        ptr2 += 0x1000;
        ptr1 += 0x1000;
    }
}
