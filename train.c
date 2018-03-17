#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <pthread.h>
#include "utils.h"
#include "cache.h"

#define DIV_ROUND_UP(n, to) (((n) - 1) / (to) + 1)

static int pt_wait(pid_t pid) {
    int info;
    trycall(waitpid(pid, &info, 0), "wait");
    if (WIFSTOPPED(info))
        return WSTOPSIG(info);
    return 0;
}

static void pt_read(int pid, unsigned long addr, void *buf, size_t size) {
    size_t size_longs = DIV_ROUND_UP(size, sizeof(long));
    unsigned long longs[size_longs];
    for (size_t i = 0; i < size_longs; i++) {
        longs[i] = trycall(ptrace(PTRACE_PEEKDATA, pid, addr + i*sizeof(long), 0), "ptrace read");
    }
    memcpy(buf, longs, size);
}

static void pt_write(int pid, unsigned long addr, void *buf, size_t size) {
    size_t size_longs = DIV_ROUND_UP(size, sizeof(long));
    unsigned long longs[size_longs];
    memcpy(longs, buf, size);
    size_t extra = (size_longs * sizeof(long)) - size;
    unsigned long mask = -1ul << ((sizeof(long) - extra) * 8);
    if (extra == 0) mask = 0; // pesky cpus, modding the shift count
    unsigned long value = trycall(ptrace(PTRACE_PEEKDATA, pid, addr + (size_longs-1)*sizeof(long)), "ptrace write read");
    longs[size_longs-1] &= ~mask;
    longs[size_longs-1] |= value & mask;
    for (size_t i = 0; i < size_longs; i++) {
        trycall(ptrace(PTRACE_POKEDATA, pid, addr + i*sizeof(long), longs[i]), "ptrace write");
    }
}

void start_trainer(const char *exe, unsigned long plt, unsigned long target, unsigned long *got_out, int cpu) {
    // start process
    int pid = trycall(fork(), "fork child");
    if (pid == 0) {
        set_cpu(cpu);
        char *const arg[] = {NULL};
        trycall(ptrace(PTRACE_TRACEME, 0, 0, 0), "ptrace traceme");
        trycall(execve(exe, arg, arg), "exec child");
    }
    pt_wait(pid);
    
    // get IP
    struct user_regs_struct regs;
    trycall(ptrace(PTRACE_GETREGS, pid, 0, &regs), "ptrace getregs");
#if defined(__i386__)
    unsigned long ip = regs.eip;
#elif defined(__x86_64__)
    unsigned long ip = regs.rip;
#else
#error this only works on x86
#endif

    // check if there's a plt jump at the plt address, obtain got address
    unsigned char plt_jmp[6];
    pt_read(pid, plt, plt_jmp, sizeof(plt_jmp));
    if (plt_jmp[0] != 0xff || plt_jmp[1] != 0x25) {
        fprintf(stderr, "error: plt address does not have an indirect jump\n");
        exit(1);
    }
    unsigned long got = *(unsigned int *) &plt_jmp[2];
#if defined(__x86_64__)
    got += plt + 6; // rip relative addressing
#endif
    *got_out = got;

    // write target address to got address
    pt_write(pid, got, &target, sizeof(target));
    // write "ret" to target address
    char ret = 0xc3;
    pt_write(pid, target, &ret, 1);

    // write "call; jmp" loop to IP
    struct {
        char mov_ax[2];
        unsigned long addr;
        char call_ax[2];
        char jmp_back[2];
    } __attribute__((packed)) code = {{0x48, 0xb8}, plt, {0xff, 0xd0}, {0xeb, 0xfc}};
    pt_write(pid, ip, &code, sizeof(code));
    pt_read(pid, ip, &code, sizeof(code));

    // go
    /* for (;;) { */
        /* trycall(ptrace(PTRACE_GETREGS, pid, 0, &regs), "getregs"); */
        /* trycall(ptrace(PTRACE_SINGLESTEP, pid, 0, 0), "step"); */
        /* pt_wait(pid); */
    /* } */
    trycall(ptrace(PTRACE_DETACH, pid, 0, 0, 0), "detach");
}

static void start_evictor(unsigned long got) {
    int pid = trycall(fork(), "fork evictor");
    if (pid == 0)
        for (;;)
            evict((void *) got);
}

static const char *argv0;

static void usage() {
    const char *usage =
        "usage: %1$s [-n threads] exe plt target\n";
    fprintf(stderr, usage, argv0);
    exit(-1);
}

static unsigned long parse_long(const char *str) {
    char *end;
    unsigned long res = strtoul(str, &end, 0);
    if (*end != '\0')
        usage();
    return res;
}

int main(int argc, char *const argv[]) {
    argv0 = argv[0];

    int opt;
    int threads = sysconf(_SC_NPROCESSORS_ONLN);
    while ((opt = getopt(argc, argv, "n:")) != -1) {
        switch (opt) {
            case 'n':
                threads = parse_long(optarg);
                break;
            case '?':
                usage();
                break;
        }
    }
    argc -= optind;
    argv += optind;
    if (argc > 3)
        usage();

    const char *exe = argv[0];
    unsigned long plt = parse_long(argv[1]);
    unsigned long target = parse_long(argv[2]);
    unsigned long got = 0;
    for (int i = 0; i < threads; i++) {
        unsigned long this_got;
        start_trainer(exe, plt, target, &this_got, i);
        if (got == 0)
            got = this_got;
        else if (this_got != got)
            printf("warning: thread %d got (0x%lx) differs from thread 0 got (0x%lx)\n", i, got, this_got);
    }
    start_evictor(got);
    for (;;) pause();
    return 0;
}
