/**
 * @file    pic_math_addsub.c (host reference backend)
 * @brief   Portable-C add/sub/negate primitives -- the Tier-1 oracle.
 *
 * @details
 *   Linked by the CMake host build. The same symbols are provided by
 *   src/pic16/pic_math_addsub.c (mid-range: `addwf` + the `btfsc STATUS,0`/
 *   `incf` carry idiom -- no ADDWFC) and src/pic18/pic_math_addsub.c
 *   (`addwfc`/`subfwb`, single-instruction carry-add/subtract) for the XC8
 *   target builds.
 *
 *   The host reference uses native wider types and is trivially correct.
 *   Negation is done in unsigned to avoid the -INT_MIN overflow: 0 - (unsigned)v
 *   wraps to the two's-complement result, so negate(INT16_MIN) = INT16_MIN
 *   and negate(INT32_MIN) = INT32_MIN (documented).
 */

#include "pic_math.h"

uint16_t pic_math_add_u16(uint16_t a, uint16_t b, bool *carry_out)
{
    uint32_t s = (uint32_t)a + (uint32_t)b;
    if (carry_out) *carry_out = (s > 0xFFFFu);
    return (uint16_t)s;
}

uint16_t pic_math_sub_u16(uint16_t a, uint16_t b, bool *borrow_out)
{
    if (borrow_out) *borrow_out = (a < b);
    return (uint16_t)(a - b);
}

int16_t pic_math_negate_s16(int16_t v)
{
    return (int16_t)(0u - (uint16_t)v);
}

int32_t pic_math_negate_s32(int32_t v)
{
    return (int32_t)(0u - (uint32_t)v);
}