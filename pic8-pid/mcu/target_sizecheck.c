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
