/**
 * @file    encoder.c
 * @brief   x4 quadrature decode via a Gray-code transition table, one
 *          implementation for host, PIC16, and PIC18 alike (no per-family
 *          backend, no inline asm).
 *
 * @details
 *   The decode is the standard, widely-used technique for quadrature with no
 *   dedicated hardware: track a 2-bit state `(A<<1)|B`, look up
 *   `QUAD_TABLE[(prev_state<<2)|new_state]` for a `{-1, 0, +1}` step. With
 *   state ordering 00,01,11,10 (true Gray code, adjacent states differ by one
 *   bit) the table below is a well-established reference (used in numerous
 *   open-source rotary-encoder libraries), not novel work. Phase 1's test
 *   suite walks a full rotation forward and backward and asserts the total,
 *   the known-good-against-the-table check this repo's convention requires.
 *
 *   Sign convention: the shipped table counts the physical direction
 *   00->10->11->01->00 positive (+4 per electrical cycle) and the opposite
 *   00->01->11->10->00 negative. Which physical rotation is "forward" is a
 *   wiring convention: swap `pin_a` and `pin_b` at `encoder_init` to invert
 *   direction (see docs/API.md). No `direction_invert` flag, swapping two
 *   constructor arguments already solves it.
 *
 *   Only integer add/compare/shift are needed, no multiply, so unlike
 *   `pic8-pid` this module does NOT link `pic8-math`.
 *
 *   `encoder_update()` is pure C, no HAL dependency. The only HAL surface is
 *   `encoder_get_position()`'s atomic 32-bit read (a `volatile int32_t` the
 *   ISR writes asynchronously to mainline; an 8-bit core reads it in 4 bytes
 *   and could tear mid-read), wrapped in `HAL_IRQ_Disable()`/`HAL_IRQ_Restore()`
 *   exactly like `pic8_tick_get()`. The 16-bit diagnostic counters are wrapped
 *   the same way for consistency even though a torn counter is low-stakes.
 *
 *   The optional per-instance glitch gate uses `pic8_tick_get()` /
 *   `pic8_tick_elapsed_since()` (a 1 ms timebase). `pic8-tick`'s 1 ms
 *   resolution is coarse relative to true electrical bounce timescales but
 *   adequate for mechanical detent bounce (typically resolves within a few
 *   ms); documented plainly, not oversold. See docs/ARCHITECTURE.md for the
 *   two-counter error model (impossible transitions vs. rejected-by-gate).
 */

#include "encoder.h"
#include "pic8_tick.h"        /* glitch-gate timebase                       */
#include "core/hal_irq.h"     /* HAL_IRQ_Disable / Restore (atomic reads)  */

/* Gray-code quadrature step table. Indexed by (last_state<<2)|new_state
 * where state = (A<<1)|B (raw 2-bit value: 00=0, 01=1, 10=2, 11=3).
 * +1 / -1 = a valid single-bit Gray transition (one edge counted, x4);
 * 0 = no transition OR an impossible transition (both bits flipped at once,
 * a missed edge or sample corruption), which the caller counts as an error.
 * This is the verbatim table from docs/pic8-encoder-plan.md; it is a
 * well-established reference, not novel work, so it is not redesigned here. */
static const int8_t QUAD_TABLE[16] = {
     0, -1, +1,  0,
    +1,  0,  0, -1,
    -1,  0,  0, +1,
     0, +1, -1,  0,
};

/* Extract this instance's 2-bit (a<<1)|b state from a port byte. */
static uint8_t extract_state(const encoder_t *enc, uint8_t port_value)
{
    uint8_t a = (uint8_t)((port_value >> enc->pin_a) & 1U);
    uint8_t b = (uint8_t)((port_value >> enc->pin_b) & 1U);
    return (uint8_t)((a << 1) | b);
}

void encoder_init(encoder_t *enc, uint8_t pin_a, uint8_t pin_b,
                  uint16_t min_edge_interval_ms, uint8_t port_value)
{
    enc->pin_a                = pin_a;
    enc->pin_b                = pin_b;
    enc->min_edge_interval_ms = min_edge_interval_ms;
    enc->position             = 0;
    enc->error_count          = 0;
    enc->glitch_count         = 0;
    enc->last_edge_tick       = pic8_tick_get();
    enc->last_state           = extract_state(enc, port_value);
}

void encoder_reset(encoder_t *enc, uint8_t port_value)
{
    /* Gains (pin_a/pin_b/min_edge_interval_ms) are unchanged; only the
     * accumulators and the resync state are reset. */
    enc->position       = 0;
    enc->error_count    = 0;
    enc->glitch_count   = 0;
    enc->last_edge_tick = pic8_tick_get();
    enc->last_state     = extract_state(enc, port_value);
}

void encoder_update(encoder_t *enc, uint8_t port_value)
{
    uint8_t new_state = extract_state(enc, port_value);

    /* Nothing changed for THIS instance's pins (another instance's pins
     * changed on the same port byte, or a same-edge re-read): no-op, not an
     * error. This is why multiple encoders can share one port byte safely. */
    if (new_state == enc->last_state) return;

    /* Optional per-instance minimum-edge-interval gate. Active only when
     * min_edge_interval_ms != 0, so a clean Hall-effect channel on the same
     * firmware isn't held to a timing gate a bouncy mechanical one needs. */
    if (enc->min_edge_interval_ms != 0U) {
        uint32_t now = pic8_tick_get();
        if (pic8_tick_elapsed_since(enc->last_edge_tick) <
            (uint32_t)enc->min_edge_interval_ms) {
            /* Drop it as a glitch. last_state is intentionally NOT updated,
             * so the next sample compares against the last ACCEPTED state,
             * not this rejected one. */
            enc->glitch_count++;
            return;
        }
        enc->last_edge_tick = now;
    }

    int8_t delta = QUAD_TABLE[(uint8_t)((enc->last_state << 2) | new_state)];
    if (delta == 0) {
        /* new_state != last_state (checked above) yet not a valid single
         * quadrature step: both bits appeared to flip between samples, a
         * missed edge or sample corruption. Count it; position is unchanged
         * (delta == 0), and last_state still advances so the decoder
         * resyncs to the new sample rather than stuck against the old one. */
        enc->error_count++;
    }
    enc->position  += delta;
    enc->last_state = new_state;
}

int32_t encoder_get_position(const encoder_t *enc)
{
    /* Atomic 32-bit read: an 8-bit core reads the volatile int32_t in 4
     * bytes; the ISR (encoder_update from the RB-change callback) could
     * update it mid-read. Mirrors pic8_tick_get()'s exact pattern. */
    uint8_t s = HAL_IRQ_Disable();
    int32_t p = enc->position;
    HAL_IRQ_Restore(s);
    return p;
}

uint16_t encoder_get_error_count(const encoder_t *enc)
{
    /* 16-bit read has the same tear risk in principle on an 8-bit core; wrap
     * it the same way for consistency even though a torn diagnostic counter
     * is low-stakes. */
    uint8_t s = HAL_IRQ_Disable();
    uint16_t c = enc->error_count;
    HAL_IRQ_Restore(s);
    return c;
}

uint16_t encoder_get_glitch_count(const encoder_t *enc)
{
    uint8_t s = HAL_IRQ_Disable();
    uint16_t c = enc->glitch_count;
    HAL_IRQ_Restore(s);
    return c;
}