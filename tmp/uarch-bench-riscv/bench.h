/*
 * bench.h
 *
 * Benchmark framework definitions for uarch-bench-riscv.
 * Ported from uarch-bench (travisdowns) - C/C++ benchmarks only.
 */

#ifndef BENCH_H_
#define BENCH_H_

#include <stdint.h>
#include <stddef.h>

/* ------------------------------------------------------------------ */
/*  Core types                                                        */
/* ------------------------------------------------------------------ */

typedef void (*bench_fn)(uint64_t iterations);

typedef struct {
    const char *id;
    const char *name;
    const char *category;
    bench_fn    func;
    uint64_t    ops_per_iter;
} benchmark_t;

/* ------------------------------------------------------------------ */
/*  Timing interface (implemented in timing.c)                        */
/* ------------------------------------------------------------------ */

uint64_t rdtsc_start(void);
uint64_t rdtsc_end(void);
double   run_benchmark(bench_fn func, uint64_t iterations, uint64_t ops_per_iter);

/* ------------------------------------------------------------------ */
/*  Compiler hints (architecture-independent)                         */
/* ------------------------------------------------------------------ */

/* Prevent the compiler from optimising away a computed value. */
#define DO_NOT_OPTIMIZE(val) __asm__ volatile("" :: "r"(val))

/* Prevent the compiler from optimising away a computed double. */
#define DO_NOT_OPTIMIZE_DOUBLE(val) __asm__ volatile("" :: "f"(val))

/* Force the compiler to treat a variable as modified. */
#define FORCE_MODIFY(val) __asm__ volatile("" : "+r"(val))

/* Force the compiler to treat a double variable as modified. */
#define FORCE_MODIFY_DOUBLE(val) __asm__ volatile("" : "+f"(val))

/* Force materialisation of anything pointed to by ptr. */
#define SINK_PTR(ptr) __asm__ volatile("" :: "r"(ptr) : "memory")

#define NEVER_INLINE  __attribute__((noinline))
#define ALWAYS_INLINE __attribute__((always_inline)) inline

/* ------------------------------------------------------------------ */
/*  Division benchmarks                                               */
/* ------------------------------------------------------------------ */

extern void bench_div_lat_32_64(uint64_t iterations);
extern void bench_div_tput_32_64(uint64_t iterations);
extern void bench_div_lat_64_64(uint64_t iterations);
extern void bench_div_tput_64_64(uint64_t iterations);
extern void bench_div_lat_128_64(uint64_t iterations);
extern void bench_div_tput_128_64(uint64_t iterations);

/* ------------------------------------------------------------------ */
/*  Utility / arithmetic benchmarks                                   */
/* ------------------------------------------------------------------ */

extern void bench_gettimeofday(uint64_t iterations);
extern void bench_crc8(uint64_t iterations);
extern void bench_sum_halves(uint64_t iterations);
extern void bench_mul4(uint64_t iterations);
extern void bench_mul_chain(uint64_t iterations);
extern void bench_mul_chain4(uint64_t iterations);
extern void bench_add_indirect(uint64_t iterations);
extern void bench_add_indirect_shift(uint64_t iterations);

/* ------------------------------------------------------------------ */
/*  Linked-list benchmarks                                            */
/* ------------------------------------------------------------------ */

extern void bench_linkedlist_sentinel(uint64_t iterations);
extern void bench_linkedlist_counter(uint64_t iterations);

/* ------------------------------------------------------------------ */
/*  Strided store benchmarks                                          */
/* ------------------------------------------------------------------ */

/*
 * Naming: bench_strided_store_{width}_{stride}s_{kib}k
 *   width  = 1 | 4 | 8  (bytes)
 *   stride = 1,2,4,8,16,32,64,128
 *   kib    = 4,8,16,32,64,128,256,512,1024,2048
 *
 * Same-location variants:
 *   bench_strided_store_{width}_sameloc
 *   bench_strided_store_{width}_sameloc_split  (4 and 8 only)
 */

#define DECLARE_STRIDED_STORE(width, stride, kib) \
    extern void bench_strided_store_##width##_##stride##s_##kib##k(uint64_t iterations);

#define DECLARE_STRIDED_SAMELOC(width) \
    extern void bench_strided_store_##width##_sameloc(uint64_t iterations);

#define DECLARE_STRIDED_SPLIT(width) \
    extern void bench_strided_store_##width##_sameloc_split(uint64_t iterations);

/* --- 1-byte stores --- */
DECLARE_STRIDED_SAMELOC(1)
#define X_STRIDES_1(kib) \
    DECLARE_STRIDED_STORE(1, 1, kib) DECLARE_STRIDED_STORE(1, 2, kib) \
    DECLARE_STRIDED_STORE(1, 4, kib) DECLARE_STRIDED_STORE(1, 8, kib) \
    DECLARE_STRIDED_STORE(1, 16, kib) DECLARE_STRIDED_STORE(1, 32, kib) \
    DECLARE_STRIDED_STORE(1, 64, kib) DECLARE_STRIDED_STORE(1, 128, kib)
X_STRIDES_1(4) X_STRIDES_1(8) X_STRIDES_1(16) X_STRIDES_1(32) X_STRIDES_1(64)
X_STRIDES_1(128) X_STRIDES_1(256) X_STRIDES_1(512) X_STRIDES_1(1024) X_STRIDES_1(2048)

/* --- 4-byte stores --- */
DECLARE_STRIDED_SAMELOC(4)
DECLARE_STRIDED_SPLIT(4)
#define X_STRIDES_4(kib) \
    DECLARE_STRIDED_STORE(4, 1, kib) DECLARE_STRIDED_STORE(4, 2, kib) \
    DECLARE_STRIDED_STORE(4, 4, kib) DECLARE_STRIDED_STORE(4, 8, kib) \
    DECLARE_STRIDED_STORE(4, 16, kib) DECLARE_STRIDED_STORE(4, 32, kib) \
    DECLARE_STRIDED_STORE(4, 64, kib) DECLARE_STRIDED_STORE(4, 128, kib)
X_STRIDES_4(4) X_STRIDES_4(8) X_STRIDES_4(16) X_STRIDES_4(32) X_STRIDES_4(64)
X_STRIDES_4(128) X_STRIDES_4(256) X_STRIDES_4(512) X_STRIDES_4(1024) X_STRIDES_4(2048)

/* --- 8-byte stores --- */
DECLARE_STRIDED_SAMELOC(8)
DECLARE_STRIDED_SPLIT(8)
#define X_STRIDES_8(kib) \
    DECLARE_STRIDED_STORE(8, 1, kib) DECLARE_STRIDED_STORE(8, 2, kib) \
    DECLARE_STRIDED_STORE(8, 4, kib) DECLARE_STRIDED_STORE(8, 8, kib) \
    DECLARE_STRIDED_STORE(8, 16, kib) DECLARE_STRIDED_STORE(8, 32, kib) \
    DECLARE_STRIDED_STORE(8, 64, kib) DECLARE_STRIDED_STORE(8, 128, kib)
X_STRIDES_8(4) X_STRIDES_8(8) X_STRIDES_8(16) X_STRIDES_8(32) X_STRIDES_8(64)
X_STRIDES_8(128) X_STRIDES_8(256) X_STRIDES_8(512) X_STRIDES_8(1024) X_STRIDES_8(2048)

/* ------------------------------------------------------------------ */
/*  Volatile gap store benchmarks                                     */
/* ------------------------------------------------------------------ */

extern void bench_vs_8b_gap_0_elems(uint64_t iterations);
extern void bench_vs_8b_gap_1_elems(uint64_t iterations);
extern void bench_vs_8b_gap_2_elems(uint64_t iterations);
extern void bench_vs_8b_gap_56_bytes(uint64_t iterations);
extern void bench_vs_8b_gap_64_bytes(uint64_t iterations);

extern void bench_vs_32b_gap_0_elems(uint64_t iterations);
extern void bench_vs_32b_gap_1_elems(uint64_t iterations);
extern void bench_vs_32b_gap_2_elems(uint64_t iterations);
extern void bench_vs_32b_gap_56_bytes(uint64_t iterations);
extern void bench_vs_32b_gap_64_bytes(uint64_t iterations);

extern void bench_vs_64b_gap_0_elems(uint64_t iterations);
extern void bench_vs_64b_gap_1_elems(uint64_t iterations);
extern void bench_vs_64b_gap_2_elems(uint64_t iterations);
extern void bench_vs_64b_gap_56_bytes(uint64_t iterations);
extern void bench_vs_64b_gap_64_bytes(uint64_t iterations);

/* ------------------------------------------------------------------ */
/*  Misaligned store benchmarks                                       */
/* ------------------------------------------------------------------ */

/*
 * bench_misaligned_sameloc_{offset}   offset = 0..64
 * bench_misaligned_rolling_{offset}   offset = 0..64
 * bench_misaligned_twoloc_{offset}    offset = 0..64
 */

#define DECLARE_MISALIGNED(offset) \
    extern void bench_misaligned_sameloc_##offset(uint64_t iterations); \
    extern void bench_misaligned_rolling_##offset(uint64_t iterations); \
    extern void bench_misaligned_twoloc_##offset(uint64_t iterations);

DECLARE_MISALIGNED(0)  DECLARE_MISALIGNED(1)  DECLARE_MISALIGNED(2)
DECLARE_MISALIGNED(3)  DECLARE_MISALIGNED(4)  DECLARE_MISALIGNED(5)
DECLARE_MISALIGNED(6)  DECLARE_MISALIGNED(7)  DECLARE_MISALIGNED(8)
DECLARE_MISALIGNED(9)  DECLARE_MISALIGNED(10) DECLARE_MISALIGNED(11)
DECLARE_MISALIGNED(12) DECLARE_MISALIGNED(13) DECLARE_MISALIGNED(14)
DECLARE_MISALIGNED(15) DECLARE_MISALIGNED(16) DECLARE_MISALIGNED(17)
DECLARE_MISALIGNED(18) DECLARE_MISALIGNED(19) DECLARE_MISALIGNED(20)
DECLARE_MISALIGNED(21) DECLARE_MISALIGNED(22) DECLARE_MISALIGNED(23)
DECLARE_MISALIGNED(24) DECLARE_MISALIGNED(25) DECLARE_MISALIGNED(26)
DECLARE_MISALIGNED(27) DECLARE_MISALIGNED(28) DECLARE_MISALIGNED(29)
DECLARE_MISALIGNED(30) DECLARE_MISALIGNED(31) DECLARE_MISALIGNED(32)
DECLARE_MISALIGNED(33) DECLARE_MISALIGNED(34) DECLARE_MISALIGNED(35)
DECLARE_MISALIGNED(36) DECLARE_MISALIGNED(37) DECLARE_MISALIGNED(38)
DECLARE_MISALIGNED(39) DECLARE_MISALIGNED(40) DECLARE_MISALIGNED(41)
DECLARE_MISALIGNED(42) DECLARE_MISALIGNED(43) DECLARE_MISALIGNED(44)
DECLARE_MISALIGNED(45) DECLARE_MISALIGNED(46) DECLARE_MISALIGNED(47)
DECLARE_MISALIGNED(48) DECLARE_MISALIGNED(49) DECLARE_MISALIGNED(50)
DECLARE_MISALIGNED(51) DECLARE_MISALIGNED(52) DECLARE_MISALIGNED(53)
DECLARE_MISALIGNED(54) DECLARE_MISALIGNED(55) DECLARE_MISALIGNED(56)
DECLARE_MISALIGNED(57) DECLARE_MISALIGNED(58) DECLARE_MISALIGNED(59)
DECLARE_MISALIGNED(60) DECLARE_MISALIGNED(61) DECLARE_MISALIGNED(62)
DECLARE_MISALIGNED(63) DECLARE_MISALIGNED(64)

/* ------------------------------------------------------------------ */
/*  Volatile store study benchmarks                                   */
/* ------------------------------------------------------------------ */

extern void bench_volatile_store_64b(uint64_t iterations);

extern void bench_arb_u64_0_0_0_0(uint64_t iterations);
extern void bench_arb_u64_0_1_0_1(uint64_t iterations);
extern void bench_arb_u64_0_2_4_6(uint64_t iterations);
extern void bench_arb_u64_0_xxx(uint64_t iterations);
extern void bench_arb_u32_0_1_0_1(uint64_t iterations);
extern void bench_arb_u32_0_2_4_6(uint64_t iterations);
extern void bench_arb_u32_0_16_0_16(uint64_t iterations);
extern void bench_arb_u32_0_16_16_16(uint64_t iterations);
extern void bench_arb_u32_0_16_17_18(uint64_t iterations);

/* ------------------------------------------------------------------ */
/*  Transcendental (math.h) benchmarks                                */
/* ------------------------------------------------------------------ */

extern void bench_log_tput(uint64_t iterations);
extern void bench_exp_tput(uint64_t iterations);
extern void bench_pow_tput(uint64_t iterations);
extern void bench_log_lat(uint64_t iterations);
extern void bench_exp_lat(uint64_t iterations);
extern void bench_pow_lat(uint64_t iterations);

/* ------------------------------------------------------------------ */
/*  Syscall benchmarks                                                */
/* ------------------------------------------------------------------ */

extern void bench_getuid_glibc(uint64_t iterations);
extern void bench_getuid_syscall(uint64_t iterations);
extern void bench_getpid_syscall(uint64_t iterations);
extern void bench_close_999(uint64_t iterations);
extern void bench_getcpu_syscall(uint64_t iterations);
extern void bench_notexist_syscall(uint64_t iterations);
extern void bench_sched_getcpu(uint64_t iterations);

/* ------------------------------------------------------------------ */
/*  Benchmark table (defined in main.c)                               */
/* ------------------------------------------------------------------ */

extern benchmark_t all_benchmarks[];
extern size_t      num_benchmarks;

#endif /* BENCH_H_ */
