/**
 * @file    target_smoke16.c
 * @brief   PIC16F87XA on-target smoke: links the real PIC16 inline-asm math
 *          backend and exercises a few routines, proving the backend builds
 *          and links for the target.
 *
 * @details
 *   The PIC16F87XA family tops out at 8 K words of flash and 368 bytes of
 *   RAM -- too small to hold the full math library + the HAL + the
 *   golden-vector self-test (tests/target_selftest.c) + golden_vectors.h.
 *   That full on-target self-test is the Tier-3 artifact on the 32 K-flash
 *   PIC18 family (mcu/pic18fxx5x-math-mplabx builds it). On PIC16, Tier-3
 *   validation therefore relies on Tier-1 (the exhaustive host tests, which
 *   cover the algorithm) + the asm build (this smoke proves the PIC16
 *   inline-asm backend compiles and links) + the worked hand-traces in the
 *   backend comments. See docs/ARCHITECTURE.md "Testing tiers".
 *
 *   This smoke calls a representative primitive from each asm-leaf group
 *   (multiply, divide, add, BCD) so the linker pulls in the real asm bodies,
 *   then idles. On a real target it runs the asm on silicon once at startup.
 */

#include "pic8_hal.h"
#include "core/pic8_harness.h"
#include "pic_math.h"

int main(void)
{
    pic8_harness_init(0UL);

    volatile uint16_t r1 = pic_math_mul_u16(0x0102u, 0x0103u);
    pic_math_udiv16_t d  = pic_math_divmod_u16(0x0007u, 0x0002u, 0);
    bool carry = false;
    volatile uint16_t r2 = pic_math_add_u16(0xFFFFu, 0x0002u, &carry);
    volatile uint8_t  r3 = pic_math_bcd_add8(0x55u, 0x55u, &carry);
    volatile uint16_t r4 = pic_math_sqrt_u16(100u);
    (void)r1; (void)d; (void)r2; (void)r3; (void)r4; (void)carry;

    for (uint32_t i = 0; pic8_harness_running(i); i++) {
        pic8_harness_tick();
    }
    return pic8_harness_report(1);
}