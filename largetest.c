#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#include "memcpy1.h"

__attribute__((noinline))
void naive_memcpy(char *dst, const char *src, size_t n)
{
    for (size_t i = 0; i < n; i++)
        dst[i] = src[i];
}

static long elapsed_ns(struct timespec a, struct timespec b)
{
    return (b.tv_sec - a.tv_sec) * 1000000000L +
           (b.tv_nsec - a.tv_nsec);
}

int main(void)
{
    size_t sizes[] = {
        64, 128, 256, 512,
        1024, 2048, 4096,
        8192, 16384, 32768,
        65536, 131072, 262144,
        524288, 1048576,
        2097152, 4194304
    };

    for (int s = 0; s < sizeof(sizes)/sizeof(sizes[0]); s++) {
        size_t size = sizes[s];

        char *buf1 = aligned_alloc(64, size);
        char *buf2 = aligned_alloc(64, size);

        for (size_t i = 0; i < size; i++)
            buf2[i] = (char)i;

        /* warm-up */
        for (int i = 0; i < 100; i++)
            memcpy(buf1, buf2, size);

        int iters = 1000000 / (size / 64);
        if (iters < 100) iters = 100;

        struct timespec a, b;

        clock_gettime(CLOCK_MONOTONIC, &a);
        for (int i = 0; i < iters; i++)
            naive_memcpy(buf1, buf2, size);
        clock_gettime(CLOCK_MONOTONIC, &b);
        long naive = elapsed_ns(a, b) / iters;

        clock_gettime(CLOCK_MONOTONIC, &a);
        for (int i = 0; i < iters; i++)
            memcpy1(buf1, buf2, size);
        clock_gettime(CLOCK_MONOTONIC, &b);
        long custom = elapsed_ns(a, b) / iters;

        clock_gettime(CLOCK_MONOTONIC, &a);
        for (int i = 0; i < iters; i++)
            memcpy(buf1, buf2, size);
        clock_gettime(CLOCK_MONOTONIC, &b);
        long libc = elapsed_ns(a, b) / iters;

        printf("%7zu B | naive %4ld ns | memcpy1 %4ld ns | libc %4ld ns\n",
               size, naive, custom, libc);

        free(buf1);
        free(buf2);
    }
    return 0;
}

