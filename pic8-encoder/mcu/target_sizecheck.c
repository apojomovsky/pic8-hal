/**
 * @file    target_sizecheck.c
 * @brief   Minimal on-target program exercising encoder.c, used only by the
 *          mcu build (see the Makefiles under mcu/) to prove encoder.c (and
 *          the pic8-tick timebase it links) cross-compiles cleanly with real
 *          XC8 for PIC16/PIC18 silicon and to report flash/RAM footprint.
 *          Not a correctness test: encoder_update correctness is fully
 *          covered on host by ../tests/test_encoder.c (there is no
 *          per-family variant of encoder.c, so the host suite already proves
 *          the shipped code).
 *
 *          It exercises every code path so the linker pulls the bodies in:
 *          encoder_init (which calls pic8_tick_get, linking pic8-tick +
 *          HAL_IRQ), encoder_update with the glitch gate armed (the
 *          pic8_tick_get/pic8_tick_elapsed_since path) and with a real
 *          transition (the QUAD_TABLE path), and all three atomic getters
 *          (HAL_IRQ_Disable/Restore). Links the now-extended HAL (the RB-change
 *          hook from Phase 0a) and pic8-tick -- the same set an on-target
 *          application using a quadrature encoder off RB<7:4> would link.
 *
 *          Measured footprint is recorded in docs/ARCHITECTURE.md.
 *          `encoder_t` is a small plain struct (pin_a/pin_b/last_state/min_edge_interval_ms
 *          + a volatile int32_t position + last_edge_tick + two volatile
 *          uint16_t counters), expected roughly 16-18 bytes, similar order to
 *          pic8-pid's 21-byte pid_t.
 */

#include "encoder.h"
#include "pic8_tick.h"

static encoder_t g_enc;

int main(void)
{
    pic8_tick_init(FOSC_HZ);

    /* Gate armed (min_edge_interval_ms != 0) so the pic8_tick timebase path
     * in encoder_update is linked, not just the disabled-gate path. */
    encoder_init(&g_enc, 4, 5, 5, 0x00U);

    for (;;) {
        /* A couple of edges to exercise the QUAD_TABLE path. */
        encoder_update(&g_enc, 0x20U);   /* RB5 high -> state 01 */
        encoder_update(&g_enc, 0x30U);   /* RB4+RB5 high -> state 11 */

        (void)encoder_get_position(&g_enc);
        (void)encoder_get_error_count(&g_enc);
        (void)encoder_get_glitch_count(&g_enc);
    }
}
