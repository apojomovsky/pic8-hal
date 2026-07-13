/**
 * @file    pic_math_numeric.c (shared portable-C, no asm)
 * @brief   pic_math_diff3 (3-point derivative) and pic_math_integrate_simpson38
 *          (Simpson's-3/8 integration) in fixed point.
 *
 * @details
 *   One implementation, linked by every backend. These are the "derived
 *   routines built on the leaf primitives" the plan keeps portable C -- here
 *   the only leaf primitive they use is the 16x16 multiply (pic_math_mul_s16)
 *   for the fixed-point product; the rest is integer add/shift. The fixed-
 *   point scale factors are passed in explicitly (as AN544 does -- it
 *   precomputes 1/(2h) and 3h/8 into RAM constants once, since multiply is
 *   cheaper than divide), so the caller controls the Q-format. See
 *   include/pic_math.h for the precise Q-format of each argument.
 *
 *   diff3 is the 3-point central first derivative: (x_now - x_prev2)/(2h).
 *   The central difference is EXACT for polynomials of degree <= 2 (error
 *   O(h^2)), so the Tier-1 test checks linear and quadratic sequences for an
 *   exact Q8.8 result (within one Q8.8 ULP for non-integer slopes) and a
 *   higher-order sequence within a documented bound.
 *
 *   integrate_simpson38 is Simpson's-3/8-rule integration over four samples:
 *   (3h/8)*(f0 + 3f1 + 3f2 + f3). The 3/8 rule is EXACT for polynomials of
 *   degree <= 3, so the Tier-1 test checks a cubic for an exact Q16.16 result
 *   and a quartic within a documented bound.
 */

#include "pic_math.h"

int16_t pic_math_diff3(int16_t x_prev2, int16_t x_prev1, int16_t x_now,
                       int16_t inv_2h_q8)
{
    /* 3-point central first derivative: (x_now - x_prev2) / (2h). x_prev1 is
     * the midpoint sample; the central-difference formula does not use it
     * (it is in the signature for the 3-sample window context, per the API).
     *
     * Fixed point: inv_2h_q8 is Q8.8 (1/(2h) * 2^8). The product
     * (x_now - x_prev2) * inv_2h_q8 is the derivative in Q8.8 (Q16.0 * Q8.8 =
     * Q8.8 when the result fits the 16-bit return). Returned as int16,
     * truncated if the product exceeds the Q8.8 range (documented). */
    (void)x_prev1;
    int32_t diff = (int32_t)x_now - (int32_t)x_prev2;
    int32_t prod = diff * (int32_t)inv_2h_q8;
    return (int16_t)prod;
}

int32_t pic_math_integrate_simpson38(int16_t f0, int16_t f1, int16_t f2,
                                     int16_t f3, int32_t three_h_over_8_q16)
{
    /* Simpson's 3/8 rule: integral ~= (3h/8) * (f0 + 3f1 + 3f2 + f3).
     *
     * Fixed point: three_h_over_8_q16 is Q16.16 (3h/8 * 2^16). The product
     * three_h_over_8_q16 * sum is the integral in Q16.16. Computed in 32-bit
     * (int32*int32 -> int32): the product truncates to the int32 Q16.16
     * return if it overflows, so the caller must scale the inputs so the
     * result fits int32 -- documented in the header. (PIC16 XC8 has no
     * 64-bit integer type, so the product is intentionally 32-bit, not
     * widened to 64.) */
    int32_t sum = (int32_t)f0 + 3 * (int32_t)f1 + 3 * (int32_t)f2 + (int32_t)f3;
    int32_t prod = three_h_over_8_q16 * sum;
    return prod;
}