/*
 * c-test.c
 *
 * All pure-C benchmark implementations, ported from uarch-bench (travisdowns).
 * Logic is preserved 1:1 from the original C++ code.
 */

#include "bench.h"

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>

/* ================================================================== */
/*  Helper: aligned allocation                                        */
/* ================================================================== */

static void *aligned_alloc_helper(size_t alignment, size_t size)
{
    void *ptr = NULL;
    if (posix_memalign(&ptr, alignment, size) != 0)
        return NULL;
    return ptr;
}

/*
 * The original uses a single static region reused across calls.
 * We replicate that behaviour.
 */
static void *aligned_ptr(size_t alignment, size_t size)
{
    static void *ptr = NULL;
    static size_t cur_size = 0;
    if (!ptr || size > cur_size) {
        free(ptr);
        ptr = aligned_alloc_helper(alignment < sizeof(void *) ? sizeof(void *) : alignment, size);
        cur_size = size;
    }
    return ptr;
}

/*
 * Returns a value the compiler cannot prove is zero.
 * Original: int always_zero();
 */
static int always_zero_val = 0;
static int always_zero(void)
{
    int r = always_zero_val;
    FORCE_MODIFY(r);
    return r;
}

/* ================================================================== */
/*  Cache-line touch benchmarks                                       */
/* ================================================================== */

/*
 * Ported from mem-benches-oneshot.cpp: touch_bench + touch_lines (util.cpp).
 * touch_lines reads one volatile byte per cache line across the region.
 */

#define UB_CACHE_LINE_SIZE 64

static long touch_lines(void *region, size_t size)
{
    if (size == 0) return 0;
    char sum = 0;
    volatile char *cregion = (volatile char *)region;
    for (volatile char *r = cregion; r < cregion + size; r += UB_CACHE_LINE_SIZE) {
        sum += *r;
    }
    sum += cregion[size - 1];
    return sum;
}

#define DEFINE_TOUCH_LINES(kib)                                              \
void bench_touch_lines_##kib(uint64_t iters)                                 \
{                                                                            \
    static void *region = NULL;                                              \
    size_t size = (size_t)(kib) * 1024;                                      \
    if (!region) region = aligned_alloc_helper(64, size);                     \
    (void)iters; /* oneshot: runs once */                                    \
    long r = touch_lines(region, size);                                      \
    DO_NOT_OPTIMIZE(r);                                                      \
}

DEFINE_TOUCH_LINES(1)
DEFINE_TOUCH_LINES(2)
DEFINE_TOUCH_LINES(4)
DEFINE_TOUCH_LINES(8)
DEFINE_TOUCH_LINES(16)
DEFINE_TOUCH_LINES(32)
DEFINE_TOUCH_LINES(64)
DEFINE_TOUCH_LINES(128)
DEFINE_TOUCH_LINES(256)
DEFINE_TOUCH_LINES(512)
DEFINE_TOUCH_LINES(1024)

/* ================================================================== */
/*  Division benchmarks                                               */
/* ================================================================== */

static inline uint64_t div32_64_op(uint64_t a) { return 0x12345678u / a; }
static inline uint64_t div64_64_op(uint64_t a) { return 0x1234567812345678ull / a; }

/*
 * 128b / 64b: the original uses inline asm "div" on x86.
 * For RISC-V / portable we use the portable fallback: __uint128_t.
 */
static inline uint64_t div128_64_op(uint64_t a)
{
    a |= 0xF234567890123456ull;
    __uint128_t num = ((__uint128_t)123 << 64) | 2;
    return (uint64_t)(num / a);
}

/*
 * Dependent (latency) division loop: each division depends on the previous
 * result via (sum & zero), creating a serial chain.
 */
#define DEFINE_DIV_LAT(name, op)                                 \
void name(uint64_t iters)                                        \
{                                                                \
    uint64_t sum = 0;                                            \
    int zero = always_zero();                                    \
    for (uint64_t k = 1; k <= iters; k++) {                     \
        uint64_t d = k + (sum & (uint64_t)zero);                \
        sum += op(d);                                            \
    }                                                            \
    DO_NOT_OPTIMIZE(sum);                                        \
}

/*
 * Independent (throughput) division loop: each division is independent.
 */
#define DEFINE_DIV_TPUT(name, op)                                \
void name(uint64_t iters)                                        \
{                                                                \
    uint64_t sum = 0;                                            \
    for (uint64_t k = 1; k <= iters; k++) {                     \
        sum += op(k);                                            \
    }                                                            \
    DO_NOT_OPTIMIZE(sum);                                        \
}

DEFINE_DIV_LAT (bench_div_lat_32_64,  div32_64_op)
DEFINE_DIV_TPUT(bench_div_tput_32_64, div32_64_op)
DEFINE_DIV_LAT (bench_div_lat_64_64,  div64_64_op)
DEFINE_DIV_TPUT(bench_div_tput_64_64, div64_64_op)
DEFINE_DIV_LAT (bench_div_lat_128_64, div128_64_op)
DEFINE_DIV_TPUT(bench_div_tput_128_64,div128_64_op)

/* ================================================================== */
/*  gettimeofday benchmark                                            */
/* ================================================================== */

void bench_gettimeofday(uint64_t iters)
{
    struct timeval tv;
    for (uint64_t i = 0; i < iters; i++) {
        gettimeofday(&tv, NULL);
    }
    DO_NOT_OPTIMIZE(tv.tv_usec);
}

/* ================================================================== */
/*  CRC-8 benchmark                                                   */
/* ================================================================== */

/* The table is intentionally all-zeros; same as original */
static uint8_t crc8_table[256] = {0};

static uint32_t crc8(uint32_t crc, const uint8_t *data, size_t len)
{
    crc &= 0xff;
    const uint8_t *end = data + len;
    while (data < end)
        crc = crc8_table[crc ^ *data++];
    return crc;
}

void bench_crc8(uint64_t iters)
{
    uint8_t buf[4096];
    SINK_PTR(buf);
    uint32_t crc = 0;
    do {
        crc = crc8(crc, buf, sizeof(buf));
    } while (--iters != 0);
    DO_NOT_OPTIMIZE(crc);
}

/* ================================================================== */
/*  Sum-halves benchmark                                              */
/* ================================================================== */

static void sum_halves(const uint32_t *data, size_t len,
                       uint32_t *out_top, uint32_t *out_bottom)
{
    uint32_t top = 0, bottom = 0;
    for (size_t i = 0; i < len; i += 2) {
        uint32_t elem;

        elem = data[i];
        top    += elem >> 16;
        bottom += elem & 0xFFFF;

        elem = data[i + 1];
        top    += elem >> 16;
        bottom += elem & 0xFFFF;
    }
    *out_top = top;
    *out_bottom = bottom;
}

void bench_sum_halves(uint64_t iters)
{
    uint32_t buf[4096];
    SINK_PTR(buf);
    do {
        uint32_t top, bottom;
        sum_halves(buf, sizeof(buf) / sizeof(buf[0]), &top, &bottom);
        uint32_t r = top + bottom;
        DO_NOT_OPTIMIZE(r);
    } while (--iters != 0);
}

/* ================================================================== */
/*  Multiplication benchmarks                                         */
/* ================================================================== */

static NEVER_INLINE uint32_t mul_by(const uint32_t *data, size_t len, uint32_t m)
{
    uint32_t sum = 0;
    for (size_t i = 0; i < len - 1; i++) {
        uint32_t x = data[i], y = data[i + 1];
        sum += x * y * m * (uint32_t)i * (uint32_t)i;
    }
    DO_NOT_OPTIMIZE(sum);
    return sum;
}

static NEVER_INLINE uint32_t mul_chain_inner(const uint32_t *data, size_t len, uint32_t m)
{
    uint32_t product = 1;
    for (size_t i = 0; i < len; i++) {
        product *= data[i];
    }
    DO_NOT_OPTIMIZE(product);
    return product;
}

static NEVER_INLINE uint32_t mul_chain4_inner(const uint32_t *data, size_t len, uint32_t m)
{
    uint32_t p1 = 1, p2 = 1, p3 = 1, p4 = 1;
    for (size_t i = 0; i < len; i += 4) {
        p1 *= data[i + 0];
        p2 *= data[i + 1];
        p3 *= data[i + 2];
        p4 *= data[i + 3];
    }
    uint32_t product = p1 * p2 * p3 * p4;
    DO_NOT_OPTIMIZE(product);
    return product;
}

#define DEFINE_MUL_BENCH(name, inner_fn)                          \
void name(uint64_t iters)                                         \
{                                                                 \
    uint32_t buf[4096];                                           \
    SINK_PTR(buf);                                                \
    uint32_t x = 123;                                             \
    FORCE_MODIFY(x);                                              \
    do {                                                          \
        uint32_t r = inner_fn(buf, sizeof(buf)/sizeof(buf[0]), x);\
        DO_NOT_OPTIMIZE(r);                                       \
    } while (--iters != 0);                                       \
}

DEFINE_MUL_BENCH(bench_mul4,       mul_by)
DEFINE_MUL_BENCH(bench_mul_chain,  mul_chain_inner)
DEFINE_MUL_BENCH(bench_mul_chain4, mul_chain4_inner)

/* ================================================================== */
/*  Indirect-add benchmarks                                           */
/* ================================================================== */

static NEVER_INLINE uint32_t add_indirect_inner(const uint32_t *data,
                                                 const uint32_t *offsets,
                                                 size_t len)
{
    assert(len >= 2 && len % 2 == 0);
    uint32_t sum1 = 0, sum2 = 0;
    size_t i = len;
    do {
        sum1 += data[offsets[i - 1]];
        sum2 += data[offsets[i - 2]];
        i -= 2;
    } while (i);
    uint32_t r = sum1 + sum2;
    DO_NOT_OPTIMIZE(r);
    return r;
}

static NEVER_INLINE uint32_t add_indirect_shift_inner(const uint32_t *data,
                                                       const uint32_t *offsets,
                                                       size_t len)
{
    uint32_t sum1 = 0, sum2 = 0;
    size_t i = len;
    do {
        uint64_t twooffsets;
        memcpy(&twooffsets, offsets + i - 2, sizeof(uint64_t));
        sum1 += data[twooffsets >> 32];
        sum2 += data[twooffsets & 0xFFFFFFFF];
        i -= 2;
    } while (i);
    uint32_t r = sum1 + sum2;
    DO_NOT_OPTIMIZE(r);
    return r;
}

#define DEFINE_ADD_INDIRECT(name, inner_fn)                        \
void name(uint64_t iters)                                         \
{                                                                 \
    uint32_t buf[4096];                                           \
    uint32_t offsets[4096];                                       \
    memset(offsets, 0, sizeof(offsets));                           \
    SINK_PTR(buf);                                                \
    SINK_PTR(offsets);                                            \
    uint32_t x = 123;                                             \
    FORCE_MODIFY(x);                                              \
    do {                                                          \
        uint32_t r = inner_fn(buf, offsets, sizeof(buf)/sizeof(buf[0])); \
        DO_NOT_OPTIMIZE(r);                                       \
    } while (--iters != 0);                                       \
}

DEFINE_ADD_INDIRECT(bench_add_indirect,       add_indirect_inner)
DEFINE_ADD_INDIRECT(bench_add_indirect_shift, add_indirect_shift_inner)

/* ================================================================== */
/*  Linked-list benchmarks                                            */
/* ================================================================== */

#define LIST_COUNT 4000
#define NODE_COUNT 5

struct list_node {
    int value;
    struct list_node *next;
};

struct list_head {
    int size;
    struct list_node *first;
};

static struct list_head make_list(int size)
{
    struct list_head head;
    head.size = size;
    head.first = NULL;
    if (size != 0) {
        struct list_node *all_nodes = (struct list_node *)calloc((size_t)size,
                                                                  sizeof(struct list_node));
        head.first = (struct list_node *)calloc(1, sizeof(struct list_node));
        head.first->value = 1;
        head.first->next = NULL;
        struct list_node *cur = head.first;
        int remaining = size;
        while (--remaining > 0) {
            struct list_node *n = all_nodes++;
            cur->next = n;
            cur = n;
        }
    }
    return head;
}

static struct list_head *list_of_lists = NULL;

static void init_lists(void)
{
    if (list_of_lists) return;
    list_of_lists = (struct list_head *)malloc(LIST_COUNT * sizeof(struct list_head));
    for (int i = 0; i < LIST_COUNT; i++) {
        list_of_lists[i] = make_list(NODE_COUNT);
    }
}

static long sum_sentinel(struct list_head list)
{
    int sum = 0;
    for (struct list_node *cur = list.first; cur; cur = cur->next) {
        sum += cur->value;
    }
    return sum;
}

static long sum_counter(struct list_head list)
{
    int sum = 0;
    struct list_node *cur = list.first;
    for (int i = 0; i < list.size; cur = cur->next, i++) {
        sum += cur->value;
    }
    return sum;
}

void bench_linkedlist_sentinel(uint64_t iters)
{
    init_lists();
    int sum = 0;
    while (iters-- > 0) {
        for (size_t li = 0; li < LIST_COUNT; li++) {
            sum += sum_sentinel(list_of_lists[li]);
        }
    }
    DO_NOT_OPTIMIZE(sum);
}

void bench_linkedlist_counter(uint64_t iters)
{
    init_lists();
    int sum = 0;
    while (iters-- > 0) {
        for (size_t li = 0; li < LIST_COUNT; li++) {
            sum += sum_counter(list_of_lists[li]);
        }
    }
    DO_NOT_OPTIMIZE(sum);
}

/* ================================================================== */
/*  Strided store benchmarks                                          */
/* ================================================================== */

/*
 * The original C++ template strided_stores<T> is expanded into a macro
 * that generates per-width functions.  The loop body is identical:
 *   4 stores per iteration, advancing by stride each time.
 */

/* Per-width inner loop, mirrors the original exactly. */
#define DEFINE_STRIDED_INNER(width, ctype)                                   \
static void strided_stores_##width##b(uint64_t iters,                        \
        char *region, uint64_t stride, uint64_t mask)                        \
{                                                                            \
    for (uint64_t i = 0; i < iters; i += 4) {                               \
        uint64_t offset = i * stride & mask;                                 \
        char *base = region + offset;                                        \
        *(ctype *)base = 0;                                                  \
        base += stride;                                                      \
        *(ctype *)base = 0;                                                  \
        base += stride;                                                      \
        *(ctype *)base = 0;                                                  \
        base += stride;                                                      \
        *(ctype *)base = 0;                                                  \
    }                                                                        \
    SINK_PTR(region);                                                        \
}

DEFINE_STRIDED_INNER(1, uint8_t)
DEFINE_STRIDED_INNER(4, uint32_t)
DEFINE_STRIDED_INNER(8, uint64_t)

/* --- same-location stores --- */

#define DEFINE_STRIDED_SAMELOC_IMPL(width)                                   \
void bench_strided_store_##width##_sameloc(uint64_t iters)                   \
{                                                                            \
    static char *region = NULL;                                              \
    if (!region) region = (char *)aligned_alloc_helper(64, 1024);            \
    strided_stores_##width##b(iters, region, 0, 0);                          \
}

DEFINE_STRIDED_SAMELOC_IMPL(1)
DEFINE_STRIDED_SAMELOC_IMPL(4)
DEFINE_STRIDED_SAMELOC_IMPL(8)

/* --- same-location cache-line-split stores (4 and 8 byte only) --- */

#define DEFINE_STRIDED_SPLIT_IMPL(width)                                     \
void bench_strided_store_##width##_sameloc_split(uint64_t iters)             \
{                                                                            \
    static char *region = NULL;                                              \
    if (!region) region = (char *)aligned_alloc_helper(64, 1024) + 63;       \
    strided_stores_##width##b(iters, region, 0, 0);                          \
}

DEFINE_STRIDED_SPLIT_IMPL(4)
DEFINE_STRIDED_SPLIT_IMPL(8)

/* --- parametric strided stores --- */

#define DEFINE_STRIDED_STORE_IMPL(width, stride_val, kib_val)                \
void bench_strided_store_##width##_##stride_val##s_##kib_val##k(uint64_t iters) \
{                                                                            \
    static char *region = NULL;                                              \
    size_t region_bytes = (size_t)(kib_val) * 1024;                          \
    if (!region) region = (char *)aligned_alloc_helper(64, ((kib_val)+1)*1024); \
    strided_stores_##width##b(iters, region, (stride_val), region_bytes - 1);\
}

#define ALL_KIB_FOR(width, stride) \
    DEFINE_STRIDED_STORE_IMPL(width, stride, 4)    \
    DEFINE_STRIDED_STORE_IMPL(width, stride, 8)    \
    DEFINE_STRIDED_STORE_IMPL(width, stride, 16)   \
    DEFINE_STRIDED_STORE_IMPL(width, stride, 32)   \
    DEFINE_STRIDED_STORE_IMPL(width, stride, 64)   \
    DEFINE_STRIDED_STORE_IMPL(width, stride, 128)  \
    DEFINE_STRIDED_STORE_IMPL(width, stride, 256)  \
    DEFINE_STRIDED_STORE_IMPL(width, stride, 512)  \
    DEFINE_STRIDED_STORE_IMPL(width, stride, 1024) \
    DEFINE_STRIDED_STORE_IMPL(width, stride, 2048)

#define ALL_STRIDES_FOR(width) \
    ALL_KIB_FOR(width, 1)   ALL_KIB_FOR(width, 2)   \
    ALL_KIB_FOR(width, 4)   ALL_KIB_FOR(width, 8)   \
    ALL_KIB_FOR(width, 16)  ALL_KIB_FOR(width, 32)  \
    ALL_KIB_FOR(width, 64)  ALL_KIB_FOR(width, 128)

ALL_STRIDES_FOR(1)
ALL_STRIDES_FOR(4)
ALL_STRIDES_FOR(8)

/* ================================================================== */
/*  Volatile gap store benchmarks                                     */
/* ================================================================== */

/*
 * volatile_stores_bytes<dist, type>: two alternating volatile stores
 * separated by 'gap' elements, unrolled 8x.
 *
 * volatile_stores_elems<dist, type>: dist is in elements, converted to bytes.
 */

#define DEFINE_VS_GAP_BYTES(name, ctype, dist_bytes)                         \
void name(uint64_t iters)                                                    \
{                                                                            \
    enum { UNROLL = 8, gap = (dist_bytes) / (int)sizeof(ctype) };            \
    unsigned char buf_raw[sizeof(ctype) * (1 + UNROLL * (dist_bytes)) + 64]  \
        __attribute__((aligned(64)));                                         \
    volatile ctype *vptr = (volatile ctype *)buf_raw;                        \
    for (uint64_t i = 0; i < iters; i += UNROLL) {                          \
        for (size_t j = 0; j < UNROLL / 2; j++) {                           \
            vptr[0 * gap] = 1;                                              \
            vptr[1 * gap] = 2;                                              \
        }                                                                    \
    }                                                                        \
}

#define DEFINE_VS_GAP_ELEMS(name, ctype, dist_elems) \
    DEFINE_VS_GAP_BYTES(name, ctype, (dist_elems) * (int)sizeof(ctype))

/*
 * 8-bit: elems gap 0,1,2 => bytes gap 0,1,2; bytes gap 56,64
 * 32-bit: elems gap 0,1,2 => bytes gap 0,4,8; bytes gap 56,64
 * 64-bit: elems gap 0,1,2 => bytes gap 0,8,16; bytes gap 56,64
 */
DEFINE_VS_GAP_ELEMS(bench_vs_8b_gap_0_elems,   uint8_t,  0)
DEFINE_VS_GAP_ELEMS(bench_vs_8b_gap_1_elems,   uint8_t,  1)
DEFINE_VS_GAP_ELEMS(bench_vs_8b_gap_2_elems,   uint8_t,  2)
DEFINE_VS_GAP_BYTES(bench_vs_8b_gap_56_bytes,   uint8_t,  56)
DEFINE_VS_GAP_BYTES(bench_vs_8b_gap_64_bytes,   uint8_t,  64)

DEFINE_VS_GAP_ELEMS(bench_vs_32b_gap_0_elems,  uint32_t, 0)
DEFINE_VS_GAP_ELEMS(bench_vs_32b_gap_1_elems,  uint32_t, 1)
DEFINE_VS_GAP_ELEMS(bench_vs_32b_gap_2_elems,  uint32_t, 2)
DEFINE_VS_GAP_BYTES(bench_vs_32b_gap_56_bytes,  uint32_t, 56)
DEFINE_VS_GAP_BYTES(bench_vs_32b_gap_64_bytes,  uint32_t, 64)

DEFINE_VS_GAP_ELEMS(bench_vs_64b_gap_0_elems,  uint64_t, 0)
DEFINE_VS_GAP_ELEMS(bench_vs_64b_gap_1_elems,  uint64_t, 1)
DEFINE_VS_GAP_ELEMS(bench_vs_64b_gap_2_elems,  uint64_t, 2)
DEFINE_VS_GAP_BYTES(bench_vs_64b_gap_56_bytes,  uint64_t, 56)
DEFINE_VS_GAP_BYTES(bench_vs_64b_gap_64_bytes,  uint64_t, 64)

/* ================================================================== */
/*  Misaligned store benchmarks                                       */
/* ================================================================== */

/*
 * aligned_stores_helper<0>: all stores go to the same location (roll=0).
 * aligned_stores_helper<1>: stores advance by 1 element each (roll=1).
 * Original unrolls 16x via BOOST_PP_REPEAT; we replicate manually.
 */

void bench_misaligned_sameloc_impl(uint64_t iters, size_t offset)
{
    unsigned char buf_raw[sizeof(uint64_t) * 1024 + 128]
        __attribute__((aligned(128)));
    volatile uint64_t *vptr = (volatile uint64_t *)(buf_raw + offset);

    /* roll = 0: all 16 stores go to vptr[0] */
    for (uint64_t i = 0; i < iters; i += 16) {
        vptr[0]=1; vptr[0]=1; vptr[0]=1; vptr[0]=1;
        vptr[0]=1; vptr[0]=1; vptr[0]=1; vptr[0]=1;
        vptr[0]=1; vptr[0]=1; vptr[0]=1; vptr[0]=1;
        vptr[0]=1; vptr[0]=1; vptr[0]=1; vptr[0]=1;
    }
}

void bench_misaligned_rolling_impl(uint64_t iters, size_t offset)
{
    unsigned char buf_raw[sizeof(uint64_t) * 1024 + 128]
        __attribute__((aligned(128)));
    volatile uint64_t *vptr = (volatile uint64_t *)(buf_raw + offset);

    /* roll = 1: stores go to vptr[0], vptr[1], ... vptr[15] */
    for (uint64_t i = 0; i < iters; i += 16) {
        vptr[0]=1;  vptr[1]=1;  vptr[2]=1;  vptr[3]=1;
        vptr[4]=1;  vptr[5]=1;  vptr[6]=1;  vptr[7]=1;
        vptr[8]=1;  vptr[9]=1;  vptr[10]=1; vptr[11]=1;
        vptr[12]=1; vptr[13]=1; vptr[14]=1; vptr[15]=1;
    }
}

void bench_misaligned_twoloc_impl(uint64_t iters, size_t offset)
{
    unsigned char buf_raw[sizeof(uint64_t) * 1024 + 128]
        __attribute__((aligned(128)));
    volatile uint64_t *vptr0 = (volatile uint64_t *)(buf_raw + offset);
    volatile uint64_t *vptr1 = (volatile uint64_t *)(buf_raw + 61 - offset);

    /* 8 pairs = 16 stores */
    for (uint64_t i = 0; i < iters; i += 16) {
        *vptr0=1; *vptr1=1; *vptr0=1; *vptr1=1;
        *vptr0=1; *vptr1=1; *vptr0=1; *vptr1=1;
        *vptr0=1; *vptr1=1; *vptr0=1; *vptr1=1;
        *vptr0=1; *vptr1=1; *vptr0=1; *vptr1=1;
    }
}

/* Generate per-offset wrappers (0..64) with macros */

#define DEFINE_MISALIGNED_SET(offset)                                        \
void bench_misaligned_sameloc_##offset(uint64_t iters)                       \
    { bench_misaligned_sameloc_impl(iters, offset); }                        \
void bench_misaligned_rolling_##offset(uint64_t iters)                       \
    { bench_misaligned_rolling_impl(iters, offset); }                        \
void bench_misaligned_twoloc_##offset(uint64_t iters)                        \
    { bench_misaligned_twoloc_impl(iters, offset); }

DEFINE_MISALIGNED_SET(0)  DEFINE_MISALIGNED_SET(1)  DEFINE_MISALIGNED_SET(2)
DEFINE_MISALIGNED_SET(3)  DEFINE_MISALIGNED_SET(4)  DEFINE_MISALIGNED_SET(5)
DEFINE_MISALIGNED_SET(6)  DEFINE_MISALIGNED_SET(7)  DEFINE_MISALIGNED_SET(8)
DEFINE_MISALIGNED_SET(9)  DEFINE_MISALIGNED_SET(10) DEFINE_MISALIGNED_SET(11)
DEFINE_MISALIGNED_SET(12) DEFINE_MISALIGNED_SET(13) DEFINE_MISALIGNED_SET(14)
DEFINE_MISALIGNED_SET(15) DEFINE_MISALIGNED_SET(16) DEFINE_MISALIGNED_SET(17)
DEFINE_MISALIGNED_SET(18) DEFINE_MISALIGNED_SET(19) DEFINE_MISALIGNED_SET(20)
DEFINE_MISALIGNED_SET(21) DEFINE_MISALIGNED_SET(22) DEFINE_MISALIGNED_SET(23)
DEFINE_MISALIGNED_SET(24) DEFINE_MISALIGNED_SET(25) DEFINE_MISALIGNED_SET(26)
DEFINE_MISALIGNED_SET(27) DEFINE_MISALIGNED_SET(28) DEFINE_MISALIGNED_SET(29)
DEFINE_MISALIGNED_SET(30) DEFINE_MISALIGNED_SET(31) DEFINE_MISALIGNED_SET(32)
DEFINE_MISALIGNED_SET(33) DEFINE_MISALIGNED_SET(34) DEFINE_MISALIGNED_SET(35)
DEFINE_MISALIGNED_SET(36) DEFINE_MISALIGNED_SET(37) DEFINE_MISALIGNED_SET(38)
DEFINE_MISALIGNED_SET(39) DEFINE_MISALIGNED_SET(40) DEFINE_MISALIGNED_SET(41)
DEFINE_MISALIGNED_SET(42) DEFINE_MISALIGNED_SET(43) DEFINE_MISALIGNED_SET(44)
DEFINE_MISALIGNED_SET(45) DEFINE_MISALIGNED_SET(46) DEFINE_MISALIGNED_SET(47)
DEFINE_MISALIGNED_SET(48) DEFINE_MISALIGNED_SET(49) DEFINE_MISALIGNED_SET(50)
DEFINE_MISALIGNED_SET(51) DEFINE_MISALIGNED_SET(52) DEFINE_MISALIGNED_SET(53)
DEFINE_MISALIGNED_SET(54) DEFINE_MISALIGNED_SET(55) DEFINE_MISALIGNED_SET(56)
DEFINE_MISALIGNED_SET(57) DEFINE_MISALIGNED_SET(58) DEFINE_MISALIGNED_SET(59)
DEFINE_MISALIGNED_SET(60) DEFINE_MISALIGNED_SET(61) DEFINE_MISALIGNED_SET(62)
DEFINE_MISALIGNED_SET(63) DEFINE_MISALIGNED_SET(64)

/* ================================================================== */
/*  Volatile store study benchmarks                                   */
/* ================================================================== */

#define VS_STUDY_UNROLL 10

void bench_volatile_store_64b(uint64_t iters)
{
    unsigned char buf_raw[sizeof(uint64_t) * 1024 * 128 + 64]
        __attribute__((aligned(64)));
    volatile uint64_t *vptr = (volatile uint64_t *)buf_raw;

    for (uint64_t i = 0; i < iters; i += 4 * VS_STUDY_UNROLL) {
        /* Unroll 10 times, 4 stores each — matches BOOST_PP_REPEAT(10,...) */
        vptr[0]=1; vptr[8]=1; vptr[0]=1; vptr[8]=1;
        vptr[0]=1; vptr[8]=1; vptr[0]=1; vptr[8]=1;
        vptr[0]=1; vptr[8]=1; vptr[0]=1; vptr[8]=1;
        vptr[0]=1; vptr[8]=1; vptr[0]=1; vptr[8]=1;
        vptr[0]=1; vptr[8]=1; vptr[0]=1; vptr[8]=1;
        vptr[0]=1; vptr[8]=1; vptr[0]=1; vptr[8]=1;
        vptr[0]=1; vptr[8]=1; vptr[0]=1; vptr[8]=1;
        vptr[0]=1; vptr[8]=1; vptr[0]=1; vptr[8]=1;
        vptr[0]=1; vptr[8]=1; vptr[0]=1; vptr[8]=1;
        vptr[0]=1; vptr[8]=1; vptr[0]=1; vptr[8]=1;
    }
}

/* Arbitrary-offset store study: write_at_offsets<T, O0, O1, O2, O3>(vptr) */

#define DEFINE_ARB_OFFSET(name, ctype, o0, o1, o2, o3)                       \
void name(uint64_t iters)                                                    \
{                                                                            \
    unsigned char buf_raw[sizeof(ctype) * 1024 * 16 + 64]                    \
        __attribute__((aligned(64)));                                         \
    volatile ctype *vptr = (volatile ctype *)buf_raw;                        \
    for (uint64_t i = 0; i < iters; i += 2 * 4) {                           \
        vptr[o0]=1; vptr[o1]=1; vptr[o2]=1; vptr[o3]=1;                     \
        vptr[o0]=1; vptr[o1]=1; vptr[o2]=1; vptr[o3]=1;                     \
    }                                                                        \
}

DEFINE_ARB_OFFSET(bench_arb_u64_0_0_0_0,    uint64_t, 0, 0, 0, 0)
DEFINE_ARB_OFFSET(bench_arb_u64_0_1_0_1,    uint64_t, 0, 1, 0, 1)
DEFINE_ARB_OFFSET(bench_arb_u64_0_2_4_6,    uint64_t, 0, 2, 4, 6)
DEFINE_ARB_OFFSET(bench_arb_u64_0_xxx,       uint64_t, 0, 0, 0, 1)
DEFINE_ARB_OFFSET(bench_arb_u32_0_1_0_1,    uint32_t, 0, 1, 0, 1)
DEFINE_ARB_OFFSET(bench_arb_u32_0_2_4_6,    uint32_t, 0, 2, 4, 6)
DEFINE_ARB_OFFSET(bench_arb_u32_0_16_0_16,  uint32_t, 0, 16, 0, 16)
DEFINE_ARB_OFFSET(bench_arb_u32_0_16_16_16, uint32_t, 0, 16, 16, 16)
DEFINE_ARB_OFFSET(bench_arb_u32_0_16_17_18, uint32_t, 0, 16, 17, 18)

/* ================================================================== */
/*  Transcendental (math.h) benchmarks                                */
/* ================================================================== */

/* Throughput: independent calls, input not dependent on output */

#define DEFINE_TRAN_TPUT(name, mathfn, use_y)                                \
void name(uint64_t iters)                                                    \
{                                                                            \
    double x0 = 0.123;                                                       \
    double y0 = 0.456;                                                       \
    while (iters--) {                                                        \
        double x = x0, y = y0;                                              \
        FORCE_MODIFY_DOUBLE(x);                                              \
        double r;                                                            \
        if (use_y) r = mathfn(x, y); else r = mathfn(x);                    \
        DO_NOT_OPTIMIZE_DOUBLE(r);                                           \
    }                                                                        \
}

/* Latency: output feeds back into input via x += t * z */

#define DEFINE_TRAN_LAT(name, mathfn, use_y)                                 \
void name(uint64_t iters)                                                    \
{                                                                            \
    double x = 0.123;                                                        \
    double y = 0.456;                                                        \
    double z = 0.0;                                                          \
    FORCE_MODIFY_DOUBLE(z);                                                  \
    while (iters--) {                                                        \
        double t;                                                            \
        if (use_y) t = mathfn(x, y); else t = mathfn(x);                    \
        x += t * z;                                                          \
        DO_NOT_OPTIMIZE_DOUBLE(x);                                           \
    }                                                                        \
}

DEFINE_TRAN_TPUT(bench_log_tput, log,  0)
DEFINE_TRAN_TPUT(bench_exp_tput, exp,  0)
DEFINE_TRAN_TPUT(bench_pow_tput, pow,  1)
DEFINE_TRAN_LAT (bench_log_lat,  log,  0)
DEFINE_TRAN_LAT (bench_exp_lat,  exp,  0)
DEFINE_TRAN_LAT (bench_pow_lat,  pow,  1)

/* ================================================================== */
/*  Syscall benchmarks                                                */
/* ================================================================== */

void bench_getuid_glibc(uint64_t iters)
{
    while (iters-- > 0) {
        getuid();
    }
}

void bench_getuid_syscall(uint64_t iters)
{
    while (iters-- > 0) {
        syscall(SYS_getuid);
    }
}

void bench_getpid_syscall(uint64_t iters)
{
    while (iters-- > 0) {
        syscall(SYS_getpid);
    }
}

void bench_close_999(uint64_t iters)
{
    while (iters-- > 0) {
        close(999);
    }
}

void bench_getcpu_syscall(uint64_t iters)
{
    unsigned cpu;
    long total = 0;
    while (iters-- > 0) {
        total += syscall(SYS_getcpu, &cpu, NULL, NULL);
    }
    DO_NOT_OPTIMIZE(total);
}

void bench_notexist_syscall(uint64_t iters)
{
    while (iters-- > 0) {
        syscall(123456);
    }
}

void bench_sched_getcpu(uint64_t iters)
{
    unsigned total = 0;
    while (iters-- > 0) {
        total += (unsigned)sched_getcpu();
    }
    DO_NOT_OPTIMIZE(total);
}
