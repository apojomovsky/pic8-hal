/**
 * @file    test_sqrt.c
 * @brief   Tier-1 host test for pic_math_sqrt_u16.
 *
 * @details
 *   EXHAUSTIVE over 0..65535 against (uint16_t)floor(sqrt((double)v) -- cheap
 *   on a host, and it removes any doubt about Newton-Raphson convergence or
 *   rounding at every single input. sqrt is the portable-C routine built on
 *   the div primitive (src/common/), so this also indirectly exercises the
 *   host divmod_u16 oracle through the Newton loop.
 */

#include "pic_math.h"
#include "pic_math_test.h"
#include <math.h>

static void test_sqrt_exhaustive(void)
{
    for (uint32_t v = 0; v <= 0xFFFFu; v++) {
        uint16_t got = pic_math_sqrt_u16((uint16_t)v);
        uint16_t exp = (uint16_t)floor(sqrt((double)v));
        if (got != exp) {
            CHECK(0, "sqrt mismatch");
            if (g_pic_math_failures < 5)
                printf("  v=%lu got=%u exp=%u\n",
                       (unsigned long)v, got, exp);
        }
    }
}

int main(void)
{
    test_sqrt_exhaustive();
    printf("test_sqrt: %u checks failed\n", (unsigned)g_pic_math_failures);
    return pic_math_test_report();
}