/**
 * @file    pic_math_sqrt.c (shared portable-C, no asm)
 * @brief   pic_math_sqrt_u16 -- integer square root via Newton-Raphson.
 *
 * @details
 *   One implementation, linked by every backend (host + PIC16 + PIC18). As
 *   AN544 does, the square root is BUILT ON THE DIVISION PRIMITIVE, not fresh
 *   asm: each Newton step is x = (x + value/x)/2, where value/x is computed
 *   by pic_math_divmod_u16 (the per-backend divide -- native `/` on host,
 *   restoring shift-subtract asm on the XC8 targets). This is the plan's
 *   "derived routines are portable C, built on the leaf primitives" rule.
 *
 *   Converges to floor(sqrt(value)) for all 0..65535 (the Tier-1 test checks
 *   every value against double-precision sqrt). The /2 is a shift (the
 *   compiler optimizes it); only the value/x division goes through the leaf
 *   divide primitive, as the plan specifies.
 */

#include "pic_math.h"
#include <stddef.h>     /* NULL (passed to divmod_u16 for the ok-out flag) */

uint16_t pic_math_sqrt_u16(uint16_t value)
{
    if (value < 2u) {
        return value;   /* 0 -> 0, 1 -> 1 */
    }
    /* Newton-Raphson: x_{k+1} = (x_k + n/x_k) / 2, converging down to
     * floor(sqrt(n)). Start from n (an over-estimate) and iterate while the
     * estimate keeps decreasing. */
    uint16_t x = value;
    uint16_t y = (uint16_t)((x + 1u) / 2u);
    while (y < x) {
        x = y;
        pic_math_udiv16_t d = pic_math_divmod_u16(value, x, NULL);
        y = (uint16_t)((x + d.quotient) / 2u);
    }
    return x;
}