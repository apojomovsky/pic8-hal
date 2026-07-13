/**
 * @file    pic_math_div.c (host reference backend)
 * @brief   Portable-C divide/modulo primitives -- the Tier-1 oracle.
 *
 * @details
 *   Linked by the CMake host build. The same symbols are provided by
 *   src/pic16/pic_math_div.c and src/pic18/pic_math_div.c (both use the
 *   restoring shift-subtract algorithm AN526/AN544 use -- no hardware
 *   divide on either core) for the XC8 target builds.
 *
 *   The host reference uses native wider types and is trivially correct.
 *   Divide-by-zero returns zeroed fields with *ok=false (the plan's
 *   documented improvement over AN526/AN544, which produced garbage and
 *   required the caller to avoid zero). INT16_MIN / -1, the one signed
 *   divide whose true quotient (32768) does not fit int16, is computed in
 *   int32 and cast back, so it returns the two's-complement wrap
 *   (INT16_MIN, 0) -- documented, not silent (see docs/API.md).
 */

#include "pic_math.h"

pic_math_udiv16_t pic_math_divmod_u16(uint16_t num, uint16_t den, bool *ok)
{
    pic_math_udiv16_t r = { 0u, 0u };
    if (den == 0u) { if (ok) *ok = false; return r; }
    if (ok) *ok = true;
    r.quotient  = (uint16_t)(num / den);
    r.remainder = (uint16_t)(num % den);
    return r;
}

pic_math_sdiv16_t pic_math_divmod_s16(int16_t num, int16_t den, bool *ok)
{
    pic_math_sdiv16_t r = { 0, 0 };
    if (den == 0) { if (ok) *ok = false; return r; }
    if (ok) *ok = true;
    /* int is 32-bit on the host; computing in int32 avoids INT16_MIN/-1 UB.
     * The cast back to int16 wraps 32768 -> INT16_MIN, documented. */
    int32_t q = (int32_t)num / (int32_t)den;
    int32_t m = (int32_t)num % (int32_t)den;
    r.quotient  = (int16_t)q;
    r.remainder = (int16_t)m;
    return r;
}

pic_math_udiv16_t pic_math_divmod_u32_16(uint32_t num, uint16_t den, bool *ok)
{
    pic_math_udiv16_t r = { 0u, 0u };
    if (den == 0u) { if (ok) *ok = false; return r; }
    if (ok) *ok = true;
    /* Quotient is truncated to 16 bits: caller must ensure num < den*65536
     * or the high quotient bits are lost (documented). */
    r.quotient  = (uint16_t)(num / (uint32_t)den);
    r.remainder = (uint16_t)(num % (uint32_t)den);
    return r;
}