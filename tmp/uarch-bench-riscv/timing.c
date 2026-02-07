/*
 * timing.c
 *
 * RISC-V timing primitives and benchmark runner.
 * TODO: fill in the actual implementations.
 */

#include "bench.h"

/* ------------------------------------------------------------------ */
/*  Low-level cycle counter (RISC-V rdcycle)                          */
/* ------------------------------------------------------------------ */

static inline uint64_t rdcycle(void)
{
    /* TODO: implement for your target, e.g.:
     *   uint64_t val;
     *   __asm__ volatile("rdcycle %0" : "=r"(val));
     *   return val;
     */
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Fences                                                            */
/* ------------------------------------------------------------------ */

static inline void fence_before(void)
{
    /* TODO: e.g. __asm__ volatile("fence" ::: "memory"); */
}

static inline void fence_after(void)
{
    /* TODO: e.g. __asm__ volatile("fence" ::: "memory"); */
}

/* ------------------------------------------------------------------ */
/*  Public timing API                                                 */
/* ------------------------------------------------------------------ */

uint64_t rdtsc_start(void)
{
    fence_before();
    return rdcycle();
}

uint64_t rdtsc_end(void)
{
    uint64_t val = rdcycle();
    fence_after();
    return val;
}

double run_benchmark(bench_fn func, uint64_t iterations, uint64_t ops_per_iter)
{
    uint64_t start = rdtsc_start();
    func(iterations);
    uint64_t end = rdtsc_end();

    double total_cycles = (double)(end - start);
    double total_ops    = (double)iterations * (double)ops_per_iter;
    return total_cycles / total_ops;
}
