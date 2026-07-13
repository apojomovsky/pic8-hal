/**
 * @file    pic_math_bcd.c (PIC16 inline-asm backend)
 * @brief   BCD primitives: conversions + the manual digit-adjust.
 *
 * @details
 *   The BCD<->binary conversions are built on this backend's asm multiply/
 *   divide primitives (pic_math_mul_u8/u16, pic_math_divmod_u16), the same
 *   "built on the leaf primitives" approach the derived routines use -- the
 *   plan's "minimal asm surface" priority applied to BCD (the conversions
 *   are arithmetic, not new asm). They are identical to the PIC18 backend's
 *   conversion C; only the primitives they call differ at link time.
 *
 *   The DAW-style digit adjust IS new per-family asm. Mid-range PIC16 has NO
 *   `daw` instruction (that is PIC18), so bcd_add8 does the manual nibble
 *   adjust: add low nibbles, +6 if > 9 (carry to high), add high nibbles +
 *   carry, +6 if > 9 (carry out). bcd_sub8 is the nibble-wise subtract with
 *   borrow (same shape as the PIC18 backend, using `goto` instead of `bra`).
 *   Hand-traces below. The adjust is defined for valid BCD operands (0..99);
 *   the host tests exercise exactly those.
 */

#include <xc.h>
#include "pic_math.h"

uint8_t pic_math_bcd8_to_bin(uint8_t bcd2)
{
    uint8_t tens = (uint8_t)(bcd2 >> 4);
    uint8_t ones = (uint8_t)(bcd2 & 0x0Fu);
    return (uint8_t)(pic_math_mul_u8(tens, 10u) + ones);
}

uint8_t pic_math_bin_to_bcd8(uint8_t value)
{
    pic_math_udiv16_t d = pic_math_divmod_u16((uint16_t)value, 10u, NULL);
    return (uint8_t)((d.quotient << 4) | (d.remainder & 0x0Fu));
}

uint16_t pic_math_bcd16_to_bin(uint32_t bcd5)
{
    uint32_t bin = 0u;
    uint16_t place = 1u;
    for (int i = 0; i < 5; i++) {
        bin += pic_math_mul_u16((uint16_t)(bcd5 & 0x0Fu), place);
        bcd5 >>= 4;
        place = (uint16_t)(place * 10u);
    }
    return (uint16_t)bin;
}

uint32_t pic_math_bin_to_bcd16(uint16_t value)
{
    uint32_t bcd = 0u;
    for (int i = 0; i < 5; i++) {
        pic_math_udiv16_t d = pic_math_divmod_u16(value, 10u, NULL);
        bcd |= (uint32_t)(d.remainder & 0x0Fu) << (i * 4);
        value = d.quotient;
    }
    return bcd;
}

/* ─── pic_math_bcd_add8 ──────────────────────────────────────────
 * Manual DAW: lo=(a&0xF)+(b&0xF); if lo>9 lo+=6 (carry to high); hi=(a>>4)+
 * (b>>4)+carry; if hi>9 hi+=6 (carry out). r=((hi&0xF)<<4)|(lo&0xF).
 * Scratch struct m_ba offsets: a@0, b@1, r@2, co@3, lo@4, hi@5 (6 bytes).
 * Hand-trace a=0x55 b=0x55 (110 -> 0x10, carry=1):
 *   lo=5+5=0xA(10)>9 -> lo=16, adj_low=0, cy=1
 *   hi=5+5+1=0xB(11)>9 -> hi=17, adj_high=1, co=1
 *   r=(1<<4)|0=0x10, co=1. Matches host: 110 mod 100=10, carry=1.            */
static volatile struct { uint8_t a, b, r, co, lo, hi; } m_ba;

uint8_t pic_math_bcd_add8(uint8_t a, uint8_t b, bool *carry_out)
{
    m_ba.a = a; m_ba.b = b; m_ba.co = 0;
    asm("banksel _m_ba");
    asm("movf  (_m_ba)+0,w");
    asm("andlw 0x0F");
    asm("movwf (_m_ba)+4");          /* lo = a_lo                            */
    asm("movf  (_m_ba)+1,w");
    asm("andlw 0x0F");
    asm("addwf (_m_ba)+4,f");         /* lo = a_lo + b_lo                     */
    /* if lo > 9: lo += 6, carry=1 */
    asm("clrf  (_m_ba)+3");           /* co = 0 (reused as the low carry)     */
    asm("movlw 10");
    asm("subwf (_m_ba)+4,w");         /* w = lo - 10, C=1 if lo>=10           */
    asm("btfss STATUS,0");
    asm("goto  _ba_lo_ok");           /* lo < 10: no low adjust               */
    asm("movlw 6");
    asm("addwf (_m_ba)+4,f");         /* lo += 6                              */
    asm("incf  (_m_ba)+3,f");         /* carry = 1                            */
    asm("_ba_lo_ok:");
    /* hi = (a>>4) + (b>>4) + carry */
    asm("swapf (_m_ba)+0,w");
    asm("andlw 0x0F");
    asm("movwf (_m_ba)+5");           /* hi = a_hi                             */
    asm("swapf (_m_ba)+1,w");
    asm("andlw 0x0F");
    asm("addwf (_m_ba)+5,f");         /* hi += b_hi                           */
    asm("movf  (_m_ba)+3,w");
    asm("addwf (_m_ba)+5,f");         /* hi += carry                          */
    /* if hi > 9: hi += 6, co=1 */
    asm("clrf  (_m_ba)+3");           /* co = 0                               */
    asm("movlw 10");
    asm("subwf (_m_ba)+5,w");         /* w = hi - 10, C=1 if hi>=10           */
    asm("btfss STATUS,0");
    asm("goto  _ba_hi_ok");
    asm("movlw 6");
    asm("addwf (_m_ba)+5,f");         /* hi += 6                              */
    asm("incf  (_m_ba)+3,f");         /* co = 1                               */
    asm("_ba_hi_ok:");
    /* r = ((hi&0xF)<<4) | (lo&0xF) */
    asm("movf  (_m_ba)+5,w");
    asm("andlw 0x0F");
    asm("movwf (_m_ba)+2");
    asm("swapf (_m_ba)+2,f");         /* r = adjusted_high << 4              */
    asm("movf  (_m_ba)+4,w");
    asm("andlw 0x0F");
    asm("iorwf (_m_ba)+2,f");         /* r |= adjusted_low                    */
    if (carry_out) *carry_out = (bool)m_ba.co;
    return m_ba.r;
}

/* ─── pic_math_bcd_sub8 ──────────────────────────────────────────
 * a - b, BCD, nibble-wise subtract with borrow. borrow_out = (a < b) decimal.
 * Scratch struct m_bs offsets: a@0, b@1, r@2, bo@3, aL@4, bL@5, br@6, aH@7.
 *   dL = aL - bL; if aL<bL: dL += 10, br=1 else br=0
 *   dH = aH - (bH + br); if borrow: dH += 10, bo=1 else bo=0
 *   r = (dH<<4)|dL
 * Hand-trace a=0x12 b=0x34 (12-34 = -22 -> 78, borrow=1):
 *   aL=2,bL=4; 2-4 borrow -> dL=8, br=1
 *   aH=1; bH+br=3+1=4; 1-4 borrow -> dH=7, bo=1
 *   r=(7<<4)|8=0x78, bo=1. Matches host: 12-34+100=78, borrow=1.            */
static volatile struct { uint8_t a, b, r, bo, aL, bL, br, aH; } m_bs;

uint8_t pic_math_bcd_sub8(uint8_t a, uint8_t b, bool *borrow_out)
{
    m_bs.a = a; m_bs.b = b; m_bs.bo = 0; m_bs.br = 0;
    asm("banksel _m_bs");
    asm("movf  (_m_bs)+0,w");
    asm("andlw 0x0F");
    asm("movwf (_m_bs)+4");           /* aL = a_lo                           */
    asm("movf  (_m_bs)+1,w");
    asm("andlw 0x0F");
    asm("movwf (_m_bs)+5");           /* bL = b_lo                           */
    asm("movf  (_m_bs)+5,w");
    asm("subwf (_m_bs)+4,f");         /* aL = aL - bL, C=1 no borrow         */
    asm("movlw 10");
    asm("btfsc STATUS,0");
    asm("goto  _bs_lo_ok");            /* no borrow: no low adjust            */
    asm("addwf (_m_bs)+4,f");         /* aL += 10                            */
    asm("incf  (_m_bs)+6,f");         /* br = 1                              */
    asm("_bs_lo_ok:");
    asm("swapf (_m_bs)+0,w");
    asm("andlw 0x0F");
    asm("movwf (_m_bs)+7");           /* aH = tens_a                          */
    asm("swapf (_m_bs)+1,w");
    asm("andlw 0x0F");                /* W = tens_b                          */
    asm("addwf (_m_bs)+6,w");         /* W = tens_b + br                      */
    asm("subwf (_m_bs)+7,f");         /* aH = tens_a - (tens_b + br), C=1 ok */
    asm("movlw 10");
    asm("btfsc STATUS,0");
    asm("goto  _bs_hi_ok");
    asm("addwf (_m_bs)+7,f");         /* aH += 10                            */
    asm("incf  (_m_bs)+3,f");         /* bo = 1                              */
    asm("_bs_hi_ok:");
    asm("swapf (_m_bs)+7,w");         /* W = aH << 4                          */
    asm("iorwf (_m_bs)+4,w");         /* W |= aL  (aL is dL, in low nibble)   */
    asm("movwf (_m_bs)+2");
    if (borrow_out) *borrow_out = (bool)m_bs.bo;
    return m_bs.r;
}