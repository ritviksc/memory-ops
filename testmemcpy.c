#include <stdio.h>
#include <string.h>
#include <time.h>

#include "memcpy1.h"

#define BUF_SIZE 1024

__attribute__((noinline)) void naive_memcpy(char *dst, const char *src, size_t n)
{
    for(size_t i=0;i<n;i++)
        dst[i] = src[i];
}

int main() {
    struct timespec start, end;
    volatile char buf1[BUF_SIZE];
    char buf2[BUF_SIZE];
    for(int i = 0; i < BUF_SIZE; i++)
    {
	buf2[i] = (char)i;
    }

    // Warm-up (cache + branch predictor)
    for(volatile int i=0;i<100;i++)
        memcpy((char *)buf1, buf2, BUF_SIZE);

    // Benchmark naive_memcpy
    clock_gettime(CLOCK_MONOTONIC, &start);
    for(int i=0;i<1000000;i++)
        naive_memcpy((char *)buf1, buf2, BUF_SIZE);
    clock_gettime(CLOCK_MONOTONIC, &end);

    long ns1 = (end.tv_sec - start.tv_sec) * 1000000000L + (end.tv_nsec - start.tv_nsec);
    printf("naive_memcpy: %ld ns per copy\n", ns1/1000000);

    // Benchmark fast_memcpy
    clock_gettime(CLOCK_MONOTONIC, &start);
    for(int i=0;i<1000000;i++)
        memcpy1((char*)buf1, buf2, BUF_SIZE);
    clock_gettime(CLOCK_MONOTONIC, &end);

    long ns2 = (end.tv_sec - start.tv_sec) * 1000000000L + (end.tv_nsec - start.tv_nsec);
    printf("memcpy1: %ld ns per copy\n", ns2/1000000);

    // Benchmark standard memcpy
    clock_gettime(CLOCK_MONOTONIC, &start);
    for(int i=0;i<1000000;i++)
        memcpy((char*)buf1, buf2, BUF_SIZE);
    clock_gettime(CLOCK_MONOTONIC, &end);

    long ns3 = (end.tv_sec - start.tv_sec) * 1000000000L + (end.tv_nsec - start.tv_nsec);
    printf("memcpy: %ld ns per copy\n", ns3/1000000);

    return 0;
}

