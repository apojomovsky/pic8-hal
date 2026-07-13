/**
 * @file    pic_math_div.c (PIC16 inline-asm backend)
 * @brief   divide/modulo primitives via the restoring shift-subtract loop.
 *
 * @details
 *   Same restoring shift-subtract algorithm as the PIC18 backend (and as
 *   AN526/AN544), using mid-range PIC16 instructions: `rlf`/`rrf` for
 *   shifts, `goto` for branches, and the `btfss STATUS,0` / `incf` idiom for
 *   the 16-bit subtract-with-borrow (mid-range has no `subwfb`). The asm
 *   mirrors the machine-verified C reference `ref_divmod_u16_algo` /
 *   `ref_divmod_u32_16_algo` in tests/test_div.c.
 *
 *   STATUS bits by number (C=0, Z=2). Each routine's scratch is a single
 *   struct (one object -> one bank -> one banksel; members by byte offset
 *   `(_m_x)+N`). 16/16 needs no 17-bit handling; 32/16 does, handled by
 *   branching on the rem shift's carry-out. Signed divide is plain C over
 *   the asm divmod_u16 with the negate-operands/negate-result sign handling.
 */

#include <xc.h>
#include "pic_math.h"

/* ─── pic_math_divmod_u16 ────────────────────────────────────────
 * 16/16 restoring division, 16 iterations. Scratch struct m_du16 offsets:
 *   num@0-1, rem@2-3, den@4-5, cnt@6  (7 bytes).
 * Mirrors ref_divmod_u16_algo:
 *   c = num>>15; num <<= 1; rem = (rem<<1)|c; if(rem>=den){rem-=den; num|=1;}
 * The subtract is in-place; on borrow (rem<den) rem is restored (rem+=den)
 * and the quotient bit stays 0. Hand-trace 0x0007/0x0002 = 3 r 1 -- see the
 * PIC18 backend comment for the full iteration walk (identical algorithm). */
static volatile struct {
    uint16_t num, rem, den;
    uint8_t  cnt;
} m_du16;

pic_math_udiv16_t pic_math_divmod_u16(uint16_t num, uint16_t den, bool *ok)
{
    pic_math_udiv16_t res = { 0u, 0u };
    if (den == 0u) { if (ok) *ok = false; return res; }
    if (ok) *ok = true;

    m_du16.num = num; m_du16.den = den; m_du16.rem = 0; m_du16.cnt = 16;
    asm("banksel _m_du16");
    asm("clrf  (_m_du16)+2");
    asm("clrf  (_m_du16)+3");
    asm("movlw 16");
    asm("movwf (_m_du16)+6");
    asm("_du16_loop:");
    asm("bcf   STATUS,0");
    asm("rlf   (_m_du16)+0,f");
    asm("rlf   (_m_du16)+1,f");
    asm("rlf   (_m_du16)+2,f");
    asm("rlf   (_m_du16)+3,f");
    /* subtract den from rem in place; C=1 if rem>=den */
    asm("movf  (_m_du16)+4,w");
    asm("subwf (_m_du16)+2,f");
    asm("movf  (_m_du16)+5,w");
    asm("btfss STATUS,0");
    asm("incf  (_m_du16)+5,w");
    asm("subwf (_m_du16)+3,f");
    /* C=1: set quotient bit; C=0: restore rem += den */
    asm("btfss STATUS,0");
    asm("goto  _du16_restore");
    asm("bsf   (_m_du16)+0,0");
    asm("goto  _du16_next");
    asm("_du16_restore:");
    asm("movf  (_m_du16)+4,w");
    asm("addwf (_m_du16)+2,f");
    asm("movf  (_m_du16)+5,w");
    asm("btfsc STATUS,0");
    asm("incf  (_m_du16)+5,w");
    asm("addwf (_m_du16)+3,f");
    asm("_du16_next:");
    asm("decfsz (_m_du16)+6,f");
    asm("goto  _du16_loop");

    res.quotient = m_du16.num;
    res.remainder = m_du16.rem;
    return res;
}

/* ─── pic_math_divmod_u32_16 ─────────────────────────────────────
 * 32/16 restoring division, 32 iterations. Scratch struct m_du32 offsets:
 *   num@0-3, rem@4-5, den@6-7, cnt@8  (9 bytes).
 * The partial remainder can reach 0x8000, so the rem shift can carry out:
 * the extended remainder is C:rem (17 bits). If C=1 after the shift, subtract
 * unconditionally (the 16-bit rem wraps to the correct low-16 of
 * extended-den) and set the bit; else do the plain 16-bit restoring subtract.
 * Mirrors ref_divmod_u32_16_algo. Quotient truncated to 16 bits (low word). */
static volatile struct {
    uint32_t num;
    uint16_t rem, den;
    uint8_t  cnt;
} m_du32;

pic_math_udiv16_t pic_math_divmod_u32_16(uint32_t num, uint16_t den, bool *ok)
{
    pic_math_udiv16_t res = { 0u, 0u };
    if (den == 0u) { if (ok) *ok = false; return res; }
    if (ok) *ok = true;

    m_du32.num = num; m_du32.den = den; m_du32.rem = 0; m_du32.cnt = 32;
    asm("banksel _m_du32");
    asm("clrf  (_m_du32)+4");
    asm("clrf  (_m_du32)+5");
    asm("movlw 32");
    asm("movwf (_m_du32)+8");
    asm("_du32_loop:");
    asm("bcf   STATUS,0");
    asm("rlf   (_m_du32)+0,f");
    asm("rlf   (_m_du32)+1,f");
    asm("rlf   (_m_du32)+2,f");
    asm("rlf   (_m_du32)+3,f");
    asm("rlf   (_m_du32)+4,f");
    asm("rlf   (_m_du32)+5,f");       /* C = old MSB of rem (the 17th bit)  */
    /* if C=1: force subtract (extended >= 0x10000 > den) */
    asm("btfss STATUS,0");
    asm("goto  _du32_cmp");
    asm("movf  (_m_du32)+6,w");
    asm("subwf (_m_du32)+4,f");
    asm("movf  (_m_du32)+7,w");
    asm("btfss STATUS,0");
    asm("incf  (_m_du32)+7,w");
    asm("subwf (_m_du32)+5,f");
    asm("bsf   (_m_du32)+0,0");
    asm("goto  _du32_next");
    /* C=0: plain 16-bit restoring subtract */
    asm("_du32_cmp:");
    asm("movf  (_m_du32)+6,w");
    asm("subwf (_m_du32)+4,f");
    asm("movf  (_m_du32)+7,w");
    asm("btfss STATUS,0");
    asm("incf  (_m_du32)+7,w");
    asm("subwf (_m_du32)+5,f");
    asm("btfss STATUS,0");
    asm("goto  _du32_restore");
    asm("bsf   (_m_du32)+0,0");
    asm("goto  _du32_next");
    asm("_du32_restore:");
    asm("movf  (_m_du32)+6,w");
    asm("addwf (_m_du32)+4,f");
    asm("movf  (_m_du32)+7,w");
    asm("btfsc STATUS,0");
    asm("incf  (_m_du32)+7,w");
    asm("addwf (_m_du32)+5,f");
    asm("_du32_next:");
    asm("decfsz (_m_du32)+8,f");
    asm("goto  _du32_loop");

    res.quotient = (uint16_t)m_du32.num;
    res.remainder = m_du32.rem;
    return res;
}

/* ─── pic_math_divmod_s16 ────────────────────────────────────────
 * Signed 16/16 over the asm unsigned path. Quotient sign = sign(num)^
 * sign(den); remainder sign follows the dividend (C99 truncated division).
 * INT16_MIN / -1 -> quotient 0x8000 (wrap of 32768), remainder 0. */
pic_math_sdiv16_t pic_math_divmod_s16(int16_t num, int16_t den, bool *ok)
{
    pic_math_sdiv16_t res = { 0, 0 };
    if (den == 0) { if (ok) *ok = false; return res; }
    if (ok) *ok = true;

    int neg_q = ((num < 0) != 0) ^ ((den < 0) != 0);
    uint16_t unum = (num < 0) ? (uint16_t)(0u - (uint16_t)num) : (uint16_t)num;
    uint16_t uden = (den < 0) ? (uint16_t)(0u - (uint16_t)den) : (uint16_t)den;

    pic_math_udiv16_t u = pic_math_divmod_u16(unum, uden, NULL);
    res.quotient  = (int16_t)(neg_q ? (uint16_t)(0u - u.quotient)  : u.quotient);
    res.remainder = (int16_t)((num < 0) ? (uint16_t)(0u - u.remainder) : u.remainder);
    return res;
}