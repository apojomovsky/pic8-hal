/**
 * @file    pic_math_mul.c (host reference backend)
 * @brief   Portable-C multiply primitives -- the Tier-1 oracle.
 *
 * @details
 *   Linked by the CMake host build alongside src/common/. The same symbols
 *   are provided by src/pic16/pic_math_mul.c and src/pic18/pic_math_mul.c
 *   for the XC8 target builds (PIC16 shift-add; PIC18 hardware MULWF). The
 *   build selects one, so no #ifdef anywhere.
 *
 *   Phase 0: stub. Phase 1 fills the bodies.
 */

#include "pic_math.h"

/* Phase 1: pic_math_mul_u8 / pic_math_mul_u16 / pic_math_mul_s16. */