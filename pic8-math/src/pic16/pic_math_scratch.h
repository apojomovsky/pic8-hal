/**
 * @file    pic_math_scratch.h (PIC16 internal)
 * @brief   Shared file-scratch buffer for the PIC16 inline-asm backends.
 *
 * @details
 *   One 16-byte buffer, shared across every PIC16 asm leaf routine and
 *   addressed by byte offset. This is the plan-consistent resolution of two
 *   PIC16 constraints the probe surfaced:
 *
 *   1. XC8 inline asm can only address file-scope symbols (not params or
 *      locals), so the asm backends need file-scratch (ARCHITECTURE.md
 *      "Inline-asm binding").
 *   2. The PIC16F87XA family has only 368 bytes of RAM. Per-ROUTINE scratch
 *      structs (~77 bytes across mul/div/addsub/bcd) plus the HAL overflow
 *      bank 0 and spill into bank 1/3, where inline-asm operand fixups
 *      overflow. A single shared 16-byte buffer (one object -> one bank,
 *      bank 0 -> addresses < 0x80 -> no fixup overflow) fits comfortably and
 *      frees ~60 bytes.
 *
 *   The buffer is HIDDEN behind the by-value public API -- callers never
 *   touch it, so the testability hazard AN526's shared ACCa had (two call
 *   sites colliding on fixed RAM) is eliminated. The routines are
 *   non-reentrant by construction (a routine writes its operands to offsets
 *   0..N before running), so the only residual hazard is interrupt
 *   re-entrancy of the SAME routine -- documented in ARCHITECTURE.md. Each
 *   routine uses offsets 0..N within its own layout; routines do not nest
 *   (no pic_math leaf calls another pic_math leaf with scratch in use), so
 *   the layouts may overlap across routines.
 *
 *   The asm instructions per routine are IDENTICAL to the per-struct form
 *   (same algorithm, same offsets -- only the symbol changes from
 *   (_m_x)+N to pic16_mscratch+N), so the hand-traces in the backend
 *   comments remain valid.
 */

#ifndef PIC16_MATH_SCRATCH_H
#define PIC16_MATH_SCRATCH_H

#include <stdint.h>

/** Shared 16-byte scratch for all PIC16 asm leaf routines (see file header). */
extern volatile uint8_t pic16_mscratch[16];

#endif /* PIC16_MATH_SCRATCH_H */