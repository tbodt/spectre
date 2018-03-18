#include <unistd.h>
#include <termios.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "cache.h"
#include "utils.h"

static const char *argv0;
static void usage() {
    const char *usage =
        "watch:    %1$s -w file address\n"
        "poke:     %1$s -p file address\n"
        "transmit: %1$s -t byte file address\n"
        "recieve:  %1$s -r file address\n";
    fprintf(stderr, usage, argv0);
    exit(1);
}

static void *map_file(const char *file, size_t offset) {
    int fd = trycall(open(file, O_RDONLY, 0666), "open file");
    char *mem = mmap(NULL, 0x10000, PROT_READ, MAP_SHARED, fd, offset & ~0xfff);
    if (mem == MAP_FAILED) { perror("mmap"); exit(errno); }
    /* mlock(mem, 0x1000); */
    return mem + (offset & 0xfff);
}

#define LOOP 2500

int main(int argc, char *const argv[]) {
    argv0 = argv[0];
    enum { POKE, WATCH, TRANSMIT, RECEIVE } mode;
    int opt;
    char txbyte;
    while ((opt = getopt(argc, argv, "wprt:")) != -1) {
        switch (opt) {
            case 'p': mode = POKE; break;
            case 'w': mode = WATCH; break;
            case 't': mode = TRANSMIT; txbyte = strtol(optarg, NULL, 0); break;
            case 'r': mode = RECEIVE; break;
            case '?': usage();
        }
    }
    argc -= optind;
    argv += optind;
    if (argc != 2)
        usage();
    const char *file = argv[0];
    char *end;
    size_t offset = strtoul(argv[1], &end, 0);
    if (*end != '\0')
        usage();

    char *mem = map_file(file, offset);

    switch (mode) {
        case WATCH:
            for (;;) {
                uint64_t start = rdtsc();
                uint64_t clocks = time_load(mem);
                flush(mem);
                if (clocks < THRESHOLD)
                    printf("hit at 0x%lx (%lu)\n", offset, (long) clocks);
                while (rdtsc() - start < LOOP)
                    __asm__ volatile("");
            }
            break;
        case RECEIVE:
            for (;;) {
                char hist[256] = {};
                for (int i = 0; i < 10000; i++) {
                    uint64_t start = rdtsc();
                    for (int j = 0; j < 256; j++) {
                        int i = ((j * 167) + 13) & 0xff;
                        char *ptr = &mem[i * 0x100];
                        uint64_t clocks = time_load(ptr);
                        flush(ptr);
                        if (clocks < THRESHOLD)
                            hist[i]++;
                    }
                    while (rdtsc() - start < LOOP)
                        __asm__ volatile("");
                }
                int max = 0;
                unsigned char ch = 0;
                int secondmax;
                unsigned char secondch;
                for (int i = 0; i < 256; i++) {
                    if (hist[i] > max) {
                        secondmax = max;
                        secondch = ch;
                        max = hist[i];
                        ch = i;
                    }
                }
                if (max > 5) {
                    printf("%c", ch);
                    /* printf("%u ", ch); */
                    fflush(stdout);
                    /* printf("'%c' @%d (%u)\t", ch, max, ch); */
                    /* printf("'%c' @%d (%u)\n", secondch, secondmax, secondch); */
                }
            }
            break;
        case POKE:
            load(mem);
            break;
        case TRANSMIT:
            for (int i = 0; i < 100000; i++)
                load(&mem[txbyte * 0x100]);
            break;
    }
    return 0;
}
