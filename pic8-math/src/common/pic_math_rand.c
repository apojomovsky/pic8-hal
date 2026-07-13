/**
 * @file    pic_math_rand.c (shared portable-C, no asm)
 * @brief   pic_math_rand_next (16-bit LFSR PRNG) and pic_math_rand_gauss
 *          (Gaussian via the Central Limit Theorem over rand_next).
 *
 * @details
 *   One implementation, linked by every backend. These are NOT cryptographic
 *   -- they are the deterministic pseudo-random generators AN544 ships, with
 *   the plan's two fixes: explicit `uint16_t *state` in/out (no hidden global
 *   seed -- reentrant and testable), and a documented escape from the all-
 *   zero state the LFSR could not otherwise recover from.
 *
 *   rand_next: a 16-bit Fibonacci LFSR with the maximal-length taps
 *   {0,2,3,5} (polynomial x^16 + x^14 + x^13 + x^11 + 1), right-shifting with
 *   the feedback bit into the MSB. Period 2^16 - 1 = 65535 (every nonzero
 *   16-bit state). The all-zero state is a fixed point of an XOR LFSR (it
 *   never recovers), so a zero *state is mapped to the documented nonzero
 *   seed 0xACE1 on entry. A shift + XOR-tap is not worth hand-optimizing in
 *   asm (per the plan), so this is portable C.
 *
 *   rand_gauss: the sum of four rand_next samples, mean-subtracted and
 *   scaled to int16. By the Central Limit Theorem the sum of independent
 *   uniforms approaches a Gaussian; four is enough for an approximately bell-
 *   shaped distribution (mirrors AN544 Figure 3). The LFSR output is uniform
 *   over 1..65535 (mean 32768), so four samples have mean 131072; centered and
 *   >>2 the result lands in int16 range.
 */

#include "pic_math.h"

#define PIC_MATH_RAND_SEED 0xACE1u   /* documented nonzero seed for state==0 */

uint16_t pic_math_rand_next(uint16_t *state)
{
    uint16_t s = *state;
    if (s == 0u) {
        s = PIC_MATH_RAND_SEED;      /* escape the all-zero fixed point      */
    }
    /* maximal-length 16-bit LFSR, taps {0,2,3,5} -> x^16+x^14+x^13+x^11+1 */
    uint16_t bit = (uint16_t)(((s >> 0) ^ (s >> 2) ^ (s >> 3) ^ (s >> 5)) & 1u);
    s = (uint16_t)((s >> 1) | (bit << 15));
    *state = s;
    return s;
}

int16_t pic_math_rand_gauss(uint16_t *state)
{
    int32_t sum = 0;
    for (int i = 0; i < 4; i++) {
        sum += (int32_t)pic_math_rand_next(state);
    }
    sum -= 131072;   /* 4 * 32768 (mean of the 1..65535 uniform LFSR output) */
    return (int16_t)(sum >> 2);   /* scale the +/-131070 range into int16     */
}