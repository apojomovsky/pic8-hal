/**
 * @file    test_smoke.c
 * @brief   Phase 0 link/compile smoke test for the pic8-math host build.
 *
 * @details
 *   Proves the CMake host pipeline works end to end -- the pic_math static
 *   library (src/host/ + src/common/) links, the public header pic_math.h
 *   compiles, and the API types are visible -- before any real arithmetic
 *   lands in Phase 1. Phase 1 replaces this with real per-primitive tests
 *   (test_mul.c, test_div.c, ...); the smoke test stays as the minimal
 *   "did the build wire up" check.
 */

#include "pic_math.h"

int main(void)
{
    /* The API types must be visible from the header alone. */
    pic_math_udiv16_t u16;
    pic_math_sdiv16_t s16;
    (void)u16;
    (void)s16;

    /* No routines are implemented yet in Phase 0; reaching here means the
     * library and header both link and compile. */
    return 0;
}