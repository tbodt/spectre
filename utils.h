#include <sched.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

static inline long trycall(long result, const char *thing) {
    if (result == -1 && errno != 0) {
        perror(thing);
        exit(errno);
    }
    return result;
}

static inline void check(const char *thing) {
    if (errno != 0) {
        perror(thing);
        exit(errno);
    }
}

static inline void set_cpu(int cpu) {
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(cpu, &set);
    sched_setaffinity(getpid(), sizeof(set), &set);
}
