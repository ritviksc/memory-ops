#include <stddef.h>
#include <stdint.h>
#include <immintrin.h>
#include <emmintrin.h>

typedef uint8_t  u8;
typedef uint32_t u32;
typedef uint64_t u64;

#if defined(__x86_64__)
    #define ALIGNMENT 8
#elif defined(__i386__)
    #define ALIGNMENT 4
#endif

#define AVX_512 64
#define AVX_256 32
#define SSE     16

static u8 isAligned(void *ptr, size_t algn)
{
    return (((uintptr_t)ptr) & (algn - 1)) == 0;
}

static void byteTransfer(void **restrict s1, const void **restrict s2, size_t count)
{
    if (count < 1)
        return;

    u8 *p1 = (u8*)*s1;
    const u8 *p2 = (const u8*)*s2;

    for (size_t i = 0; i < count; i++)
        p1[i] = p2[i];

    *s1 = p1 + count;
    *s2 = p2 + count;
}

static void wordTransfer(void **restrict s1, const void **restrict s2,
                  size_t count, size_t word_size)
{
    size_t chunks = count / word_size;

#if ALIGNMENT == 8
    u64 *p1 = (u64*)*s1;
    const u64 *p2 = (const u64*)*s2;
#else
    u32 *p1 = (u32*)*s1;
    const u32 *p2 = (const u32*)*s2;
#endif

    for (size_t i = 0; i < chunks; i++)
        p1[i] = p2[i];

    *s1 = (u8*)p1 + chunks * word_size;
    *s2 = (u8*)p2 + chunks * word_size;

    size_t left = count - (word_size * chunks);
    byteTransfer(s1, s2, left);
}

__attribute__((target("avx512f")))
static size_t memcpy_avx512(void *dst, const void *src, size_t count)
{
    u8 *d = dst;
    const u8 *s = src;

    size_t chunks = count / 64;
    for (size_t i = 0; i < chunks; i++) {
        __m512i v = _mm512_loadu_si512((const __m512i*)s);
        _mm512_store_si512((__m512i*)d, v);
        s += 64;
        d += 64;
    }
    return count - chunks * 64;
}

__attribute__((noinline)) void *memcpy1(void *restrict dst, const void *restrict src, size_t count)
{
    void *orig = dst;

    if (count < ALIGNMENT)
    {
        byteTransfer(&dst, &src, count);
        return orig;
    }

    /* AVX-512 */
    if (count > AVX_512 && __builtin_cpu_supports("avx512f"))
    {
        if (!isAligned(dst, AVX_512))
        {
            size_t head =
                (AVX_512 - ((uintptr_t)dst & (AVX_512 - 1))) & (AVX_512 - 1);
            count -= head;
            byteTransfer(&dst, &src, head);
        }

        size_t left = memcpy_avx512(dst, src, count);

        dst = (u8*)dst + (count - left);
    	src = (u8*)src + (count - left);
    	count = left;
        goto rem;
    }

    /* AVX2 */
    if (count > AVX_256 && __builtin_cpu_supports("avx2"))
    {
        if (!isAligned(dst, AVX_256))
        {
            size_t head =
                (AVX_256 - ((uintptr_t)dst & (AVX_256 - 1))) & (AVX_256 - 1);
            count -= head;
            byteTransfer(&dst, &src, head);
        }

        u8 *d = (u8*)dst;
        const u8 *s = (const u8*)src;

        size_t chunks = count / AVX_256;
        for (size_t i = 0; i < chunks; i++)
        {
            __m256i v = _mm256_loadu_si256((const __m256i*)s);
            _mm256_store_si256((__m256i*)d, v);
            s += AVX_256;
            d += AVX_256;
        }

        count -= chunks * AVX_256;
        dst = d;
        src = s;
        goto rem;
    }

    /* SSE2 */
    if (count > SSE && __builtin_cpu_supports("sse2"))
    {
        if (!isAligned(dst, SSE))
        {
            size_t head =
                (SSE - ((uintptr_t)dst & (SSE - 1))) & (SSE - 1);
            count -= head;
            byteTransfer(&dst, &src, head);
        }

        u8 *d = (u8*)dst;
        const u8 *s = (const u8*)src;

        size_t chunks = count / SSE;
        for (size_t i = 0; i < chunks; i++)
        {
            __m128i v = _mm_loadu_si128((const __m128i*)s);
            _mm_storeu_si128((__m128i*)d, v);
            s += SSE;
            d += SSE;
        }

        count -= chunks * SSE;
        dst = d;
        src = s;
        goto rem;
    }

rem:
    wordTransfer(&dst, &src, count, ALIGNMENT);
    return orig;
}

