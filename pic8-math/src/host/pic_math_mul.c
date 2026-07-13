/**
 * @file    pic_math_mul.c (host reference backend)
 * @brief   Portable-C multiply primitives -- the Tier-1 oracle.
 *
 * @details
 *   Linked by the CMake host build. The same symbols are provided by
 *   src/pic16/pic_math_mul.c (shift-and-add; no hardware multiply on
 *   mid-range PIC16F87XA) and src/pic18/pic_math_mul.c (hardware MULWF,
 *   three partial products for 16x16) for the XC8 target builds; the build
 *   selects one, so no #ifdef anywhere.
 *
 *   The host reference uses native wider types: trivially correct, and
 *   the independent oracle the Tier-1 tests cross-check the algorithm
 *   against. The XC8 optimizer already emits `mulwf` for the idiomatic
 *   8x8 form here on PIC18, so the asm backend's value is on the structured
 *   16x16 / signed paths (see docs/ARCHITECTURE.md).
 */

#include "pic_math.h"

uint16_t pic_math_mul_u8(uint8_t a, uint8_t b)
{
    return (uint16_t)a * (uint16_t)b;
}

uint32_t pic_math_mul_u16(uint16_t a, uint16_t b)
{
    return (uint32_t)a * (uint32_t)b;
}

int32_t pic_math_mul_s16(int16_t a, int16_t b)
{
    /* int is 32-bit on the host, so this cannot overflow. */
    return (int32_t)a * (int32_t)b;
}