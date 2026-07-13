/**
 * @file    pic_math_addsub.c (PIC16 inline-asm backend)
 * @brief   add/sub/negate primitives via mid-range PIC16 instructions.
 *
 * @details
 *   The mid-range PIC16F87XA core (14-bit word) has NO `addwfc`/`subwfb` --
 *   that pair arrived with enhanced mid-range/PIC18 -- so carry propagation
 *   uses AN526's own idiom: `btfsc STATUS,0` (test carry) + `incf` (add the
 *   carry into the high byte's addend). This ports AN526's carry idiom to
 *   PIC16F87XA basically unchanged (mid-range is a superset of the baseline
 *   core AN526 targeted, for arithmetic). The exact sequence matches what
 *   XC8's own optimizer emits for plain-C 16-bit add/sub.
 *
 *   STATUS bits by NUMBER (C=0, Z=2): XC8's assembler does not know the
 *   C/Z/DC aliases. Mid-range has NO `setf` (that is enhanced mid-range/
 *   PIC18), so carry/borrow-out is recorded with `incf` (1 = true).
 *
 *   Banking: each routine's scratch is a single struct (one object, so XC8
 *   places it whole in ONE bank -- it cannot split an object across banks,
 *   which is what scattered per-variable scratch did and caused fixup
 *   overflows when the group landed partly in bank 1). One `banksel` per
 *   routine covers the whole struct; members are accessed by byte offset,
 *   `(_m_x)+N`. See ARCHITECTURE.md "Inline-asm binding".
 */

#include <xc.h>
#include "pic_math.h"

/* ─── pic_math_add_u16 ───────────────────────────────────────────
 * 16-bit add with the carry idiom. Scratch struct offsets: a@0, b@2, r@4,
 * co@6. Hand-trace a=0xFFFF b=0x0002:
 *   movf b+0,w     ; w=0x02
 *   addwf a+0,w    ; w=0xFF+0x02=0x01, C=1
 *   movwf r+0      ; r_lo=0x01
 *   movf b+1,w     ; w=0x00 (movf keeps C)             ; C=1
 *   btfsc STATUS,0 ; C=1 -> don't skip incf
 *   incf b+1,w     ; w=0x01
 *   addwf a+1,w    ; w=0xFF+0x01=0x00, C=1             ; C=1
 *   movwf r+1      ; r_hi=0x00 -> r=0x0001
 *   clrf co; btfsc STATUS,0 (C=1); incf co ; co=1
 * Matches host: 0xFFFF+0x0002=0x10001 -> 0x0001, carry=1.                 */
static volatile struct { uint16_t a, b, r; uint8_t co; } m_add;

uint16_t pic_math_add_u16(uint16_t a, uint16_t b, bool *carry_out)
{
    m_add.a = a; m_add.b = b; m_add.co = 0;
    asm("banksel _m_add");
    asm("movf  (_m_add)+2,w");        /* w = bLO                            */
    asm("addwf (_m_add)+0,w");        /* w = aLO + bLO, C = carry            */
    asm("movwf (_m_add)+4");          /* rLO                                */
    asm("movf  (_m_add)+3,w");        /* w = bHI (movf preserves C)         */
    asm("btfsc STATUS,0");           /* if C=1 (carry), add it to bHI       */
    asm("incf  (_m_add)+3,w");       /* w = bHI + carry                    */
    asm("addwf (_m_add)+1,w");       /* w = aHI + (bHI + carry), C=final   */
    asm("movwf (_m_add)+5");         /* rHI                                */
    asm("clrf  (_m_add)+6");
    asm("btfsc STATUS,0");           /* if C=1 (carry out)                  */
    asm("incf  (_m_add)+6,f");       /* co = 1                             */
    if (carry_out) *carry_out = (bool)m_add.co;
    return m_add.r;
}

/* ─── pic_math_sub_u16 ───────────────────────────────────────────
 * a - b, borrow_out = (a < b). Scratch offsets: a@0, b@2, r@4, bo@6.
 * Hand-trace a=0x0002 b=0xFFFF:
 *   movf b+0,w    ; w=0xFF
 *   subwf a+0,w   ; w=0x02-0xFF=0x03, C=0 (borrow)        ; C=0
 *   movwf r+0     ; r_lo=0x03
 *   movf b+1,w    ; w=0x00 (C preserved)                  ; C=0
 *   btfss STATUS,0 ; C=0 -> don't skip incf
 *   incf b+1,w    ; w=0x01 (borrow into high)
 *   subwf a+1,w   ; w=0x00-0x01=0xFF, C=0                ; C=0
 *   movwf r+1     ; r_hi=0xFF -> r=0xFF03 (wraps to 0x0003)
 *   clrf bo; btfss STATUS,0 (C=0, no skip); incf bo ; bo=1
 * Matches host: 0x0002-0xFFFF=0x0003, borrow=1.                          */
static volatile struct { uint16_t a, b, r; uint8_t bo; } m_sub;

uint16_t pic_math_sub_u16(uint16_t a, uint16_t b, bool *borrow_out)
{
    m_sub.a = a; m_sub.b = b; m_sub.bo = 0;
    asm("banksel _m_sub");
    asm("movf  (_m_sub)+2,w");
    asm("subwf (_m_sub)+0,w");        /* w = aLO - bLO, C=1 if no borrow    */
    asm("movwf (_m_sub)+4");
    asm("movf  (_m_sub)+3,w");
    asm("btfss STATUS,0");            /* if C=0 (borrow), add 1 to subtrahend hi */
    asm("incf  (_m_sub)+3,w");
    asm("subwf (_m_sub)+1,w");        /* w = aHI - (bHI + borrow), C=1 if ok */
    asm("movwf (_m_sub)+5");
    asm("clrf  (_m_sub)+6");
    asm("btfss STATUS,0");            /* if C=0 (borrowed)                  */
    asm("incf  (_m_sub)+6,f");        /* bo = 1                             */
    if (borrow_out) *borrow_out = (bool)m_sub.bo;
    return m_sub.r;
}

/* ─── pic_math_negate_s16 ────────────────────────────────────────
 * -v = ~v + 1. Scratch offsets: v@0, r@2. The +1 is a 16-bit increment with
 * carry: inc low; if it wrapped (Z), inc high. Hand-trace v=5 (0x0005) ->
 * -5 (0xFFFB): comf v+0->0xFA, comf v+1->0xFF (r=0xFFFA); incf r+0->0xFB,
 * Z=0 -> r+1 stays 0xFF -> r=0xFFFB. v=INT16_MIN (0x8000): comf->0x7FFF,
 * inc low wraps Z=1 -> inc high 0x7F->0x80 -> 0x8000 (negate of INT16_MIN
 * is itself).                                                            */
static volatile struct { int16_t v, r; } m_neg;

int16_t pic_math_negate_s16(int16_t v)
{
    m_neg.v = v;
    asm("banksel _m_neg");
    asm("comf  (_m_neg)+0,w");
    asm("movwf (_m_neg)+2");
    asm("comf  (_m_neg)+1,w");
    asm("movwf (_m_neg)+3");
    asm("incf  (_m_neg)+2,f");        /* rLO++ ; Z if wrapped to 0          */
    asm("btfsc STATUS,2");            /* if Z (rLO wrapped), inc rHI         */
    asm("incf  (_m_neg)+3,f");
    return m_neg.r;
}

/* ─── pic_math_negate_s32 ────────────────────────────────────────
 * Same ~v + 1, carry cascade across 4 bytes. Scratch offsets: v@0, r@4. */
static volatile struct { int32_t v, r; } m_neg32;

int32_t pic_math_negate_s32(int32_t v)
{
    m_neg32.v = v;
    asm("banksel _m_neg32");
    asm("comf  (_m_neg32)+0,w");  asm("movwf (_m_neg32)+4");
    asm("comf  (_m_neg32)+1,w");  asm("movwf (_m_neg32)+5");
    asm("comf  (_m_neg32)+2,w");  asm("movwf (_m_neg32)+6");
    asm("comf  (_m_neg32)+3,w");  asm("movwf (_m_neg32)+7");
    asm("incf  (_m_neg32)+4,f");
    asm("btfsc STATUS,2");
    asm("incf  (_m_neg32)+5,f");
    asm("btfsc STATUS,2");
    asm("incf  (_m_neg32)+6,f");
    asm("btfsc STATUS,2");
    asm("incf  (_m_neg32)+7,f");
    return m_neg32.r;
}