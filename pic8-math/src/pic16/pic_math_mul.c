/**
 * @file    pic_math_mul.c (PIC16 inline-asm backend)
 * @brief   multiply primitives via AN526's shift-and-add (no HW multiply).
 *
 * @details
 *   The mid-range PIC16F87XA has no hardware multiply. So both 8x8 and
 *   16x16 use AN526's shift-and-add: accumulate a<<i into the result for each
 *   set bit i of the multiplier. The PIC_MATH_OPTIMIZE_FOR_SIZE knob
 *   (default 1 = looped) selects the looped (small) vs straight-line unrolled
 *   (fast) 8x8 form -- a real trade-off on a 1.5 KB-flash part.
 *
 *   16x16 -> 32 is the 16-iteration form (a 16-bit multiplicand shifted left
 *   in a 32-bit working register, added into a 32-bit accumulator when the
 *   multiplier's current bit is set). Signed multiply is plain C over the
 *   asm unsigned path with the negate-operands/negate-result trick.
 *
 *   STATUS bits by number (C=0, Z=2). Each routine's scratch is a single
 *   struct (one object -> one bank -> one banksel covers all of it; members
 *   accessed by byte offset `(_m_x)+N`). 16-bit add uses the movf/addwf/
 *   btfsc STATUS,0/incf/addwf idiom (matches XC8's emitted 16-bit add).
 */

#include <xc.h>
#include "pic_math.h"

/* ─── pic_math_mul_u8 ────────────────────────────────────────────
 * 8x8 -> 16 shift-add. tmp = a (16-bit, shifted left each step -> a<<i); for
 * each set bit i of b, r += tmp. Scratch struct m_mul8 offsets:
 *   a@0, b@1, bk@2, cnt@3, r@4-5, t@6-7  (8 bytes, one bank).
 * Hand-trace a=0x0C b=0x14 (12*20=240=0x00F0): b=0b00010100, bits 2,4 set.
 *   t=0x000C. bit0,1 clear (tmp->0x30). bit2 set: r+=0x30; tmp->0x60. bit3
 *   clear (tmp->0xC0). bit4 set: r+=0xC0 -> 0x30+0xC0=0xF0 -> r=0x00F0.     */

/* Straight-line (speed) form: one inlined step per multiplier bit. `bit` is
 * the literal bit index; #bit stringizes for the asm operand and a unique
 * skip label. Gated off by default (size form below). */
#define MUL8_STEP(bit) do {                                   \
    asm("btfss (_m_mul8)+1," #bit);                           \
    asm("goto  _m8_skip_" #bit);                              \
    asm("movf  (_m_mul8)+6,w");   /* tLO */                    \
    asm("addwf (_m_mul8)+4,f");   /* rLO += tLO, C */           \
    asm("movf  (_m_mul8)+7,w");   /* tHI */                    \
    asm("btfsc STATUS,0");                                    \
    asm("incf  (_m_mul8)+7,w");   /* tHI + carry */             \
    asm("addwf (_m_mul8)+5,f");   /* rHI += tHI + carry */      \
    asm("_m8_skip_" #bit ":");                                \
    asm("bcf   STATUS,0");                                    \
    asm("rlf   (_m_mul8)+6,f");   /* tmp <<= 1 */               \
    asm("rlf   (_m_mul8)+7,f");                                \
} while (0)

static volatile struct {
    uint8_t  a, b, bk, cnt;
    uint16_t r, t;
} m_mul8;

uint16_t pic_math_mul_u8(uint8_t a, uint8_t b)
{
    m_mul8.a = a; m_mul8.b = b; m_mul8.r = 0; m_mul8.t = a;
    asm("banksel _m_mul8");
    asm("clrf  (_m_mul8)+4");
    asm("clrf  (_m_mul8)+5");
    asm("clrf  (_m_mul8)+6");
    asm("clrf  (_m_mul8)+7");
    asm("movf  (_m_mul8)+0,w");
    asm("movwf (_m_mul8)+6");          /* tmp = a (16-bit, low)            */

#if PIC_MATH_OPTIMIZE_FOR_SIZE
    /* Looped form: shift b right, test its LSB each pass. */
    m_mul8.bk = b; m_mul8.cnt = 8;
    asm("movf  (_m_mul8)+1,w");        /* w = b (reload; bk has the working copy) */
    asm("movwf (_m_mul8)+2");
    asm("movlw 8");
    asm("movwf (_m_mul8)+3");
    asm("_m8_loop:");
    asm("btfss (_m_mul8)+2,0");        /* if LSB set, do the add           */
    asm("goto  _m8_lskip");
    asm("movf  (_m_mul8)+6,w");
    asm("addwf (_m_mul8)+4,f");
    asm("movf  (_m_mul8)+7,w");
    asm("btfsc STATUS,0");
    asm("incf  (_m_mul8)+7,w");
    asm("addwf (_m_mul8)+5,f");
    asm("_m8_lskip:");
    asm("bcf   STATUS,0");
    asm("rlf   (_m_mul8)+6,f");         /* tmp <<= 1                       */
    asm("rlf   (_m_mul8)+7,f");
    asm("bcf   STATUS,0");
    asm("rrf   (_m_mul8)+2,f");         /* bk >>= 1                        */
    asm("decfsz (_m_mul8)+3,f");
    asm("goto  _m8_loop");
#else
    /* Straight-line (speed) form: one step per fixed multiplier bit. */
    MUL8_STEP(0);
    MUL8_STEP(1);
    MUL8_STEP(2);
    MUL8_STEP(3);
    MUL8_STEP(4);
    MUL8_STEP(5);
    MUL8_STEP(6);
    MUL8_STEP(7);
#endif

    return m_mul8.r;
}

/* ─── pic_math_mul_u16 ───────────────────────────────────────────
 * 16x16 -> 32 shift-add, 16 iterations. tmp = a in a 32-bit register
 * (shifted left each pass -> a<<i); for each set bit of b, r += tmp (32-bit
 * add, carry idiom across 4 bytes). Scratch struct m_mul16 offsets:
 *   a@0-1, b@2-3, bk@4-5, r@6-9, t@10-13, cnt@14  (15 bytes).
 * Hand-trace a=0x0102 b=0x0103 (258*259=66822=0x00010506): b bits 0,1,8 set.
 *   r += a<<0 (0x102) + a<<1 (0x204) + a<<8 (0x10200) = 0x10506.           */
static volatile struct {
    uint16_t a, b, bk;
    uint32_t r, t;
    uint8_t  cnt;
} m_mul16;

uint32_t pic_math_mul_u16(uint16_t a, uint16_t b)
{
    m_mul16.a = a; m_mul16.b = b; m_mul16.r = 0; m_mul16.t = a;
    asm("banksel _m_mul16");
    asm("clrf  (_m_mul16)+6");
    asm("clrf  (_m_mul16)+7");
    asm("clrf  (_m_mul16)+8");
    asm("clrf  (_m_mul16)+9");
    asm("clrf  (_m_mul16)+10");
    asm("clrf  (_m_mul16)+11");
    asm("clrf  (_m_mul16)+12");
    asm("clrf  (_m_mul16)+13");
    asm("movf  (_m_mul16)+0,w");
    asm("movwf (_m_mul16)+10");
    asm("movf  (_m_mul16)+1,w");
    asm("movwf (_m_mul16)+11");        /* tmp = a (32-bit, low 16)          */

    m_mul16.cnt = 16; m_mul16.bk = b;
    asm("movf  (_m_mul16)+2,w");
    asm("movwf (_m_mul16)+4");
    asm("movf  (_m_mul16)+3,w");
    asm("movwf (_m_mul16)+5");         /* bk = b                            */
    asm("movlw 16");
    asm("movwf (_m_mul16)+14");
    asm("_m16_loop:");
    /* if bk LSB set, r += tmp (32-bit add, carry idiom across 4 bytes) */
    asm("btfss (_m_mul16)+4,0");
    asm("goto  _m16_skip");
    asm("movf  (_m_mul16)+10,w");
    asm("addwf (_m_mul16)+6,f");
    asm("movf  (_m_mul16)+11,w");
    asm("btfsc STATUS,0");
    asm("incf  (_m_mul16)+11,w");
    asm("addwf (_m_mul16)+7,f");
    asm("movf  (_m_mul16)+12,w");
    asm("btfsc STATUS,0");
    asm("incf  (_m_mul16)+12,w");
    asm("addwf (_m_mul16)+8,f");
    asm("movf  (_m_mul16)+13,w");
    asm("btfsc STATUS,0");
    asm("incf  (_m_mul16)+13,w");
    asm("addwf (_m_mul16)+9,f");
    asm("_m16_skip:");
    /* tmp <<= 1 (32-bit left shift through carry, LSB <- 0) */
    asm("bcf   STATUS,0");
    asm("rlf   (_m_mul16)+10,f");
    asm("rlf   (_m_mul16)+11,f");
    asm("rlf   (_m_mul16)+12,f");
    asm("rlf   (_m_mul16)+13,f");
    /* bk >>= 1 (test the next multiplier bit) */
    asm("bcf   STATUS,0");
    asm("rrf   (_m_mul16)+4,f");
    asm("rrf   (_m_mul16)+5,f");
    asm("decfsz (_m_mul16)+14,f");
    asm("goto  _m16_loop");

    return m_mul16.r;
}

/* ─── pic_math_mul_s16 ──────────────────────────────────────────
 * Signed 16x16 on top of the asm unsigned path: abs the operands (unsigned,
 * so INT16_MIN abs = 0x8000 with no 16-bit-int overflow), call mul_u16, and
 * negate the 32-bit result if the signs differed. Plain C calling this
 * backend's asm primitives (mul_u16 + negate_s32). */
int32_t pic_math_mul_s16(int16_t a, int16_t b)
{
    int neg = ((a < 0) != 0) ^ ((b < 0) != 0);
    uint16_t ua = (a < 0) ? (uint16_t)(0u - (uint16_t)a) : (uint16_t)a;
    uint16_t ub = (b < 0) ? (uint16_t)(0u - (uint16_t)b) : (uint16_t)b;
    uint32_t ur = pic_math_mul_u16(ua, ub);
    if (neg) {
        ur = (uint32_t)pic_math_negate_s32((int32_t)ur);
    }
    return (int32_t)ur;
}