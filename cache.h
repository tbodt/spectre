#include <stdint.h>
#include <sys/mman.h>

static inline uint64_t rdtsc() {
    uint64_t low, high;
    __asm__ volatile("rdtsc" : "=a" (low), "=d" (high));
    return (high << 32) | low;
}

static inline void load(void *ptr) {
    *(volatile char *) ptr;
}

static inline uint64_t time_load(void *ptr) {
    uint32_t start, end;
    __asm__ volatile(
            "mfence\n"
            "rdtsc\n"
            "mfence\n"
            "mov %%eax, %0\n"
            "mov (%2), %%al\n"
            "mfence\n"
            "rdtsc\n"
            "mfence\n"
            : "=&r" (start), "=&a" (end)
            : "r" (ptr)
            : "edx", "ecx");
    return end - start;
}

static inline void flush(void *ptr) {
    __asm__ volatile("clflush (%0)" : : "r" (ptr));
}

static inline uint64_t time_flush(void *ptr) {
    uint32_t start, end;
    __asm__ volatile(
            "mfence\n"
            "rdtsc\n"
            "mfence\n"
            "mov %%eax, %0\n"
            "clflush (%2)\n"
            "mfence\n"
            "rdtsc\n"
            "mfence\n"
            : "=&r" (start), "=&a" (end)
            : "r" (ptr)
            : "edx", "ecx");
    return end - start;
}

void evict(void *ptr);
