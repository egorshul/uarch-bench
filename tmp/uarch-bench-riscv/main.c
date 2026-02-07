/*
 * main.c
 *
 * Benchmark table and entry point for uarch-bench-riscv.
 */

#include "bench.h"
#include <stdio.h>

#define DEFAULT_ITERATIONS 100000

/* ================================================================== */
/*  Benchmark table                                                   */
/* ================================================================== */

/* Helper macro for strided-store table entries.
 * Function names: bench_strided_store_{w}_{s}_{k}  (no s/k suffixes to avoid ## issues) */
#define STRIDED_ENTRY(w, s, k) \
    { "strided-store-" #w "-" #s "s-" #k "k", \
      #w "-byte stores, stride " #s ", size " #k " KiB", \
      "memory/store", bench_strided_store_##w##_##s##_##k, 1, 0 },

#define ALL_KIB_ENTRIES(w, s) \
    STRIDED_ENTRY(w,s,4)    STRIDED_ENTRY(w,s,8)    STRIDED_ENTRY(w,s,16)   \
    STRIDED_ENTRY(w,s,32)   STRIDED_ENTRY(w,s,64)   STRIDED_ENTRY(w,s,128)  \
    STRIDED_ENTRY(w,s,256)  STRIDED_ENTRY(w,s,512)  STRIDED_ENTRY(w,s,1024) \
    STRIDED_ENTRY(w,s,2048)

#define ALL_STRIDE_ENTRIES(w) \
    ALL_KIB_ENTRIES(w,1)   ALL_KIB_ENTRIES(w,2)   ALL_KIB_ENTRIES(w,4)   \
    ALL_KIB_ENTRIES(w,8)   ALL_KIB_ENTRIES(w,16)  ALL_KIB_ENTRIES(w,32)  \
    ALL_KIB_ENTRIES(w,64)  ALL_KIB_ENTRIES(w,128)

/* Helper macro for misaligned-store table entries */
#define MISALIGNED_ENTRY(offset) \
    { "misaligned-sameloc-" #offset, "Same location stores with offset " #offset, \
      "memory/store-alignment", bench_misaligned_sameloc_##offset, 1, 0 }, \
    { "misaligned-rolling-" #offset, "1B stride overlapping stores offset " #offset, \
      "memory/store-alignment", bench_misaligned_rolling_##offset, 1, 0 }, \
    { "misaligned-twoloc-" #offset, "Two misaligned stores offset " #offset, \
      "memory/store-alignment", bench_misaligned_twoloc_##offset, 1, 0 },

benchmark_t all_benchmarks[] = {

    /* ---------------------------------------------------------- */
    /* Cache-line touch benchmarks                                */
    /* ---------------------------------------------------------- */
    { "touch-lines-1",    "touch one cache line 1 KiB",    "memory/touch-lines", bench_touch_lines_1,    1024/64,    1 },
    { "touch-lines-2",    "touch one cache line 2 KiB",    "memory/touch-lines", bench_touch_lines_2,    2048/64,    1 },
    { "touch-lines-4",    "touch one cache line 4 KiB",    "memory/touch-lines", bench_touch_lines_4,    4096/64,    1 },
    { "touch-lines-8",    "touch one cache line 8 KiB",    "memory/touch-lines", bench_touch_lines_8,    8192/64,    1 },
    { "touch-lines-16",   "touch one cache line 16 KiB",   "memory/touch-lines", bench_touch_lines_16,   16384/64,   1 },
    { "touch-lines-32",   "touch one cache line 32 KiB",   "memory/touch-lines", bench_touch_lines_32,   32768/64,   1 },
    { "touch-lines-64",   "touch one cache line 64 KiB",   "memory/touch-lines", bench_touch_lines_64,   65536/64,   1 },
    { "touch-lines-128",  "touch one cache line 128 KiB",  "memory/touch-lines", bench_touch_lines_128,  131072/64,  1 },
    { "touch-lines-256",  "touch one cache line 256 KiB",  "memory/touch-lines", bench_touch_lines_256,  262144/64,  1 },
    { "touch-lines-512",  "touch one cache line 512 KiB",  "memory/touch-lines", bench_touch_lines_512,  524288/64,  1 },
    { "touch-lines-1024", "touch one cache line 1024 KiB", "memory/touch-lines", bench_touch_lines_1024, 1048576/64, 1 },

    /* ---------------------------------------------------------- */
    /* Division benchmarks                                        */
    /* ---------------------------------------------------------- */
    { "div32_64-lat",  "Dependent 32b / 64b inline divisions",   "cpp/division", bench_div_lat_32_64,  1, 0 },
    { "div32_64-tput", "Independent 32b / 64b inline divisions", "cpp/division", bench_div_tput_32_64, 1, 0 },
    { "div64_64-lat",  "Dependent 64b / 64b inline divisions",   "cpp/division", bench_div_lat_64_64,  1, 0 },
    { "div64_64-tput", "Independent 64b / 64b inline divisions", "cpp/division", bench_div_tput_64_64, 1, 0 },
    { "div128_64-lat",  "Dependent 128b / 64b inline divisions",  "cpp/division", bench_div_lat_128_64,  1, 0 },
    { "div128_64-tput", "Independent 128b / 64b inline divisions","cpp/division", bench_div_tput_128_64, 1, 0 },

    /* ---------------------------------------------------------- */
    /* Utility / arithmetic benchmarks                            */
    /* ---------------------------------------------------------- */
    { "gettimeofday",      "gettimeofday() libc call",              "cpp", bench_gettimeofday,      1,    0 },
    { "crc8",              "crc8 loop",                             "cpp", bench_crc8,              4096, 0 },
    { "sum-halves",        "Sum 16-bit halves of array elems",      "cpp", bench_sum_halves,        2048, 0 },
    { "mul-4",             "Four multiplications",                  "cpp", bench_mul4,              4096, 0 },
    { "mul-chain",         "Chained multiplications",               "cpp", bench_mul_chain,         4096, 0 },
    { "mul-chain4",        "Chained multiplications, 4 chains",     "cpp", bench_mul_chain4,        4096, 0 },
    { "add-indirect",      "Indirect adds from memory",             "cpp", bench_add_indirect,      2048, 0 },
    { "add-indirect-shift","Indirect adds from memory, tricky",     "cpp", bench_add_indirect_shift,2048, 0 },

    /* ---------------------------------------------------------- */
    /* Linked-list benchmarks                                     */
    /* ---------------------------------------------------------- */
    { "linkedlist-sentinel","Linked-list w/ sentinel",  "cpp/linkedlist", bench_linkedlist_sentinel, 4000, 0 },
    { "linkedlist-counter", "Linked-list w/ count",     "cpp/linkedlist", bench_linkedlist_counter,  4000, 0 },

    /* ---------------------------------------------------------- */
    /* Strided store benchmarks — 1-byte                          */
    /* ---------------------------------------------------------- */
    { "strided-store-1-sameloc", "8 bit stores to same location", "memory/store", bench_strided_store_1_sameloc, 1, 0 },
    ALL_STRIDE_ENTRIES(1)

    /* ---------------------------------------------------------- */
    /* Strided store benchmarks — 4-byte                          */
    /* ---------------------------------------------------------- */
    { "strided-store-4-sameloc",       "32 bit stores to same location",            "memory/store", bench_strided_store_4_sameloc,       1, 0 },
    { "strided-store-4-sameloc-split", "32 bit cl split stores to same location",   "memory/store", bench_strided_store_4_sameloc_split, 1, 0 },
    ALL_STRIDE_ENTRIES(4)

    /* ---------------------------------------------------------- */
    /* Strided store benchmarks — 8-byte                          */
    /* ---------------------------------------------------------- */
    { "strided-store-8-sameloc",       "64 bit stores to same location",            "memory/store", bench_strided_store_8_sameloc,       1, 0 },
    { "strided-store-8-sameloc-split", "64 bit cl split stores to same location",   "memory/store", bench_strided_store_8_sameloc_split, 1, 0 },
    ALL_STRIDE_ENTRIES(8)

    /* ---------------------------------------------------------- */
    /* Volatile gap store benchmarks                              */
    /* ---------------------------------------------------------- */
    { "8gap0",  "8-bit stores with 0 elems gap",  "memory/store-volatile", bench_vs_8b_gap_0_elems,  1, 0 },
    { "8gap1",  "8-bit stores with 1 elems gap",  "memory/store-volatile", bench_vs_8b_gap_1_elems,  1, 0 },
    { "8gap2",  "8-bit stores with 2 elems gap",  "memory/store-volatile", bench_vs_8b_gap_2_elems,  1, 0 },
    { "8gap56", "8-bit stores with 56 bytes gap",  "memory/store-volatile", bench_vs_8b_gap_56_bytes, 1, 0 },
    { "8gap64", "8-bit stores with 64 bytes gap",  "memory/store-volatile", bench_vs_8b_gap_64_bytes, 1, 0 },

    { "32gap0",  "32-bit stores with 0 elems gap",  "memory/store-volatile", bench_vs_32b_gap_0_elems,  1, 0 },
    { "32gap1",  "32-bit stores with 1 elems gap",  "memory/store-volatile", bench_vs_32b_gap_1_elems,  1, 0 },
    { "32gap2",  "32-bit stores with 2 elems gap",  "memory/store-volatile", bench_vs_32b_gap_2_elems,  1, 0 },
    { "32gap56", "32-bit stores with 56 bytes gap",  "memory/store-volatile", bench_vs_32b_gap_56_bytes, 1, 0 },
    { "32gap64", "32-bit stores with 64 bytes gap",  "memory/store-volatile", bench_vs_32b_gap_64_bytes, 1, 0 },

    { "64gap0",  "64-bit stores with 0 elems gap",  "memory/store-volatile", bench_vs_64b_gap_0_elems,  1, 0 },
    { "64gap1",  "64-bit stores with 1 elems gap",  "memory/store-volatile", bench_vs_64b_gap_1_elems,  1, 0 },
    { "64gap2",  "64-bit stores with 2 elems gap",  "memory/store-volatile", bench_vs_64b_gap_2_elems,  1, 0 },
    { "64gap56", "64-bit stores with 56 bytes gap",  "memory/store-volatile", bench_vs_64b_gap_56_bytes, 1, 0 },
    { "64gap64", "64-bit stores with 64 bytes gap",  "memory/store-volatile", bench_vs_64b_gap_64_bytes, 1, 0 },

    /* ---------------------------------------------------------- */
    /* Misaligned store benchmarks (offsets 0..64)                 */
    /* ---------------------------------------------------------- */
    MISALIGNED_ENTRY(0)  MISALIGNED_ENTRY(1)  MISALIGNED_ENTRY(2)
    MISALIGNED_ENTRY(3)  MISALIGNED_ENTRY(4)  MISALIGNED_ENTRY(5)
    MISALIGNED_ENTRY(6)  MISALIGNED_ENTRY(7)  MISALIGNED_ENTRY(8)
    MISALIGNED_ENTRY(9)  MISALIGNED_ENTRY(10) MISALIGNED_ENTRY(11)
    MISALIGNED_ENTRY(12) MISALIGNED_ENTRY(13) MISALIGNED_ENTRY(14)
    MISALIGNED_ENTRY(15) MISALIGNED_ENTRY(16) MISALIGNED_ENTRY(17)
    MISALIGNED_ENTRY(18) MISALIGNED_ENTRY(19) MISALIGNED_ENTRY(20)
    MISALIGNED_ENTRY(21) MISALIGNED_ENTRY(22) MISALIGNED_ENTRY(23)
    MISALIGNED_ENTRY(24) MISALIGNED_ENTRY(25) MISALIGNED_ENTRY(26)
    MISALIGNED_ENTRY(27) MISALIGNED_ENTRY(28) MISALIGNED_ENTRY(29)
    MISALIGNED_ENTRY(30) MISALIGNED_ENTRY(31) MISALIGNED_ENTRY(32)
    MISALIGNED_ENTRY(33) MISALIGNED_ENTRY(34) MISALIGNED_ENTRY(35)
    MISALIGNED_ENTRY(36) MISALIGNED_ENTRY(37) MISALIGNED_ENTRY(38)
    MISALIGNED_ENTRY(39) MISALIGNED_ENTRY(40) MISALIGNED_ENTRY(41)
    MISALIGNED_ENTRY(42) MISALIGNED_ENTRY(43) MISALIGNED_ENTRY(44)
    MISALIGNED_ENTRY(45) MISALIGNED_ENTRY(46) MISALIGNED_ENTRY(47)
    MISALIGNED_ENTRY(48) MISALIGNED_ENTRY(49) MISALIGNED_ENTRY(50)
    MISALIGNED_ENTRY(51) MISALIGNED_ENTRY(52) MISALIGNED_ENTRY(53)
    MISALIGNED_ENTRY(54) MISALIGNED_ENTRY(55) MISALIGNED_ENTRY(56)
    MISALIGNED_ENTRY(57) MISALIGNED_ENTRY(58) MISALIGNED_ENTRY(59)
    MISALIGNED_ENTRY(60) MISALIGNED_ENTRY(61) MISALIGNED_ENTRY(62)
    MISALIGNED_ENTRY(63) MISALIGNED_ENTRY(64)

    /* ---------------------------------------------------------- */
    /* Volatile store study benchmarks                            */
    /* ---------------------------------------------------------- */
    { "volatile-store-64b",       "64-bit store study",                       "studies/memory/store-volatile", bench_volatile_store_64b,      1, 0 },
    { "arb_uint64_t_0_0_0_0",    "uint64_t stores with gaps 0_0_0_0",       "studies/memory/store-volatile", bench_arb_u64_0_0_0_0,         1, 0 },
    { "arb_uint64_t_0_1_0_1",    "uint64_t stores with gaps 0_1_0_1",       "studies/memory/store-volatile", bench_arb_u64_0_1_0_1,         1, 0 },
    { "arb_uint64_t_0_2_4_6",    "uint64_t stores with gaps 0_2_4_6",       "studies/memory/store-volatile", bench_arb_u64_0_2_4_6,         1, 0 },
    { "arb_uint64_t_0_xxx",      "uint64_t stores with gaps 0_xxx",          "studies/memory/store-volatile", bench_arb_u64_0_xxx,            1, 0 },
    { "arb_uint32_t_0_1_0_1",    "uint32_t stores with gaps 0_1_0_1",       "studies/memory/store-volatile", bench_arb_u32_0_1_0_1,         1, 0 },
    { "arb_uint32_t_0_2_4_6",    "uint32_t stores with gaps 0_2_4_6",       "studies/memory/store-volatile", bench_arb_u32_0_2_4_6,         1, 0 },
    { "arb_uint32_t_0_16_0_16",  "uint32_t stores with gaps 0_16_0_16",     "studies/memory/store-volatile", bench_arb_u32_0_16_0_16,       1, 0 },
    { "arb_uint32_t_0_16_16_16", "uint32_t stores with gaps 0_16_16_16",    "studies/memory/store-volatile", bench_arb_u32_0_16_16_16,      1, 0 },
    { "arb_uint32_t_0_16_17_18", "uint32_t stores with gaps 0_16_17_18",    "studies/memory/store-volatile", bench_arb_u32_0_16_17_18,      1, 0 },

    /* ---------------------------------------------------------- */
    /* Transcendental (math.h) benchmarks                         */
    /* ---------------------------------------------------------- */
    { "log",         "log(double x) throughput", "transcendental", bench_log_tput, 1, 0 },
    { "exp",         "exp(double x) throughput", "transcendental", bench_exp_tput, 1, 0 },
    { "pow",         "pow(double x) throughput", "transcendental", bench_pow_tput, 1, 0 },
    { "log_latency", "log(double x) latency",   "transcendental", bench_log_lat,  1, 0 },
    { "exp_latency", "exp(double x) latency",   "transcendental", bench_exp_lat,  1, 0 },
    { "pow_latency", "pow(double x) latency",   "transcendental", bench_pow_lat,  1, 0 },

    /* ---------------------------------------------------------- */
    /* Syscall benchmarks                                         */
    /* ---------------------------------------------------------- */
    { "getuid-glibc",     "getuid() glibc call",          "syscall", bench_getuid_glibc,     1, 0 },
    { "getuid-syscall",   "getuid using syscall()",       "syscall", bench_getuid_syscall,   1, 0 },
    { "getpid-syscall",   "getpid using syscall()",       "syscall", bench_getpid_syscall,   1, 0 },
    { "close-999",        "close() on a non-existent FD","syscall", bench_close_999,        1, 0 },
    { "getcpu-syscall",   "getcpu syscall (maybe VDSO)",  "syscall", bench_getcpu_syscall,   1, 0 },
    { "notexist-syscall", "non-existent syscall",         "syscall", bench_notexist_syscall, 1, 0 },
    { "sched_getcpu",     "sched_getcpu",                 "syscall", bench_sched_getcpu,     1, 0 },
};

size_t num_benchmarks = sizeof(all_benchmarks) / sizeof(all_benchmarks[0]);

/* ================================================================== */
/*  Entry point (stub — user will complete)                           */
/* ================================================================== */

int main(int argc, char **argv)
{
    printf("uarch-bench-riscv: %zu benchmarks registered\n", num_benchmarks);

    /* TODO: argument parsing, benchmark selection, iteration control */

    return 0;
}
