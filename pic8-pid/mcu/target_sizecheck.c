/**
 * @file    target_sizecheck.c
 * @brief   Minimal on-target program exercising pid.c, used only by the
 *          mcu build (see the Makefiles under mcu/) to prove pid.c
 *          cross-compiles cleanly with real XC8 for PIC16/PIC18 silicon
 *          and to report flash/RAM footprint. Not a correctness test:
 *          pid_update correctness is fully covered on host by
 *          ../tests/test_pid.c (there is no per-family variant of
 *          pid.c, so the host suite already proves the shipped code).
 *
 *          The pid.c Q8.8 algorithm itself doesn't depend on the signed
 *          shift being arithmetic on a particular family (the Q8.8 `>> 8`
 *          on a negative int32_t is exercised by the host tests, and the
 *          Phase 0 probe in docs/ARCHITECTURE.md records the confirming
 *          detail from the generated .s); this sizecheck just needs to
 *          link the algorithm body in and let the linker show its
 *          footprint.
 *
 *          Measured footprint, XC8 v3.10 -O2, linking the full pic8-math
 *          (mul/div/addsub/bcd/sqrt/numeric/rand, the same set pic8-fsm
 *          and pic8-debounce's mcu builds link -- a real on-target
 *          application pulling in pid.c may also want any of these, so
 *          the reported size is the "all of math available" upper bound,
 *          not the absolute-minimum "only pic_math_mul_s16" lower bound).
 *          `pid_t` itself is 21 bytes on both PIC16 and PIC18 (XC8 packs
 *          the bools and the post-int32_t int16_t without padding):
 *
 *              Target       Program space         Data space (full)
 *              PIC16F877A   1787 words (21.8% of 8 KW)   142 B (38.6% of 368 B)
 *              PIC16F873A   1789 words (43.7% of 4 KW)   140 B (72.9% of 192 B)
 *              PIC18F4550   1734 B    (5.3%  of 32 KB)   163 B (8.0%  of 2 KB)
 *
 *          The per-instance `pid_t` cost is 21 B -- the remainder is
 *          pic8-math's scratch state. A real application is unlikely to
 *          use every math routine, so the real on-target RAM is lower;
 *          this sizecheck reports the upper bound the pic8-math+pid
 *          combination requires when all of math is available.
 */

#include "pid.h"

static pid_t g_pid;

int main(void)
{
    pid_init(&g_pid, (int16_t)0x0100, (int16_t)0x0001, (int16_t)0x0000,
             (int16_t)-1000, (int16_t)1000);
    for (;;) {
        (void)pid_update(&g_pid, (int16_t)0, (int16_t)0);
    }
}
