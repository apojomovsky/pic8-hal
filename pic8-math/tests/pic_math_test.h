/**
 * @file    pic_math_test.h
 * @brief   Tiny shared test harness for the pic8-math Tier-1 host tests.
 *
 * @details
 *   No external framework (matches this repo's convention -- the HAL and
 *   task-manager "tests" are plain main()s returning a process exit code).
 *   Provides a CHECK macro (counts failures, logs the line), a fixed-seed
 *   deterministic LCG so randomized tests are reproducible, and a couple of
 *   boundary constants. Each test_*.c defines main() and returns
 *   pic_math_test_report() (0 on pass, 1 on any failure).
 *
 *   Linked only into the CMake host test executables; not built by the XC8
 *   target Makefiles (Tier-1 is host-only; the asm backends are validated
 *   by gpsim/Tier-3 + hand-traces, not these tests).
 */

#ifndef PIC_MATH_TEST_H
#define PIC_MATH_TEST_H

#include <stdint.h>
#include <stdio.h>

/** Running failure count, incremented by CHECK. */
static int g_pic_math_failures = 0;

/**
 * @brief Assert @p cond; on failure log the file/line/message and bump the
 *        failure count. Evaluates @p cond exactly once.
 */
#define CHECK(cond, msg)                                       \
    do {                                                       \
        if (!(cond)) {                                         \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, msg); \
            g_pic_math_failures++;                             \
        }                                                      \
    } while (0)

/** @brief Map the failure count to a process exit code (0=pass, 1=fail). */
static inline int pic_math_test_report(void)
{
    return (g_pic_math_failures == 0) ? 0 : 1;
}

/**
 * @brief Fixed-seed 32-bit LCG (Numerical Recipes constants) for
 *        reproducible randomized tests. Returns the next 32-bit value.
 *        Not a quality RNG -- just deterministic fuzz.
 */
static inline uint32_t pic_math_test_rand(uint32_t *state)
{
    *state = (1664525u * (*state) + 1013904223u);
    return *state;
}

#endif /* PIC_MATH_TEST_H */