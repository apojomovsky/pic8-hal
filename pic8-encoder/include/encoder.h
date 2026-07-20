/**
 * @file    encoder.h
 * @brief   Vendor-agnostic, interrupt-driven incremental quadrature decoder
 *          (x4 resolution), instantiable: one `encoder_t` per A/B channel
 *          pair.
 *
 * @details
 *   A decoder for a standard 2-channel (A/B) incremental quadrature encoder,
 *   x4 resolution (counts every edge of both channels). Composable with
 *   `pic8-pid` the way this whole module was motivated: feed
 *   `encoder_get_position()` into `pid_update()`'s `measurement` argument
 *   every control cycle.
 *
 *   Neither PIC16F87XA nor PIC18F2455/2550/4455/4550 has any encoder-aware
 *   peripheral (no QEI mode), so this is software, interrupt-driven, using
 *   the one thing both families do have: the RB<7:4> interrupt-on-change
 *   (IOC) source. Wire encoder channel A and B to two of RB4-RB7; any edge
 *   on either fires one shared interrupt; decode with a software Gray-code
 *   state-transition table.
 *
 *   Push, not poll (unlike `pic8-debounce`'s `debounce_poll()`): a quadrature
 *   signal can transition between poll calls faster than any reasonable poll
 *   period, so this is edge-triggered. The application registers one
 *   function as the HAL's RB-change callback (see
 *   `HAL_GPIO_RegisterChangeCallback` in `peripherals/hal_gpio.h`); that
 *   function forwards the received PORTB byte to `encoder_update()` for
 *   each `encoder_t` instance it owns. Fanning one byte out to N instances is
 *   application-level composition, not a HAL or encoder registry.
 *
 *   `encoder_update()` itself is pure C (no HAL dependency), but
 *   `encoder_get_position()` reads a `volatile int32_t` the ISR writes
 *   asynchronously to mainline, so it wraps the read in
 *   `HAL_IRQ_Disable()`/`HAL_IRQ_Restore()` (the same atomic-read pattern
 *   `pic8_tick_get()` uses). That one call is the entire HAL surface this
 *   module touches; only `encoder.c` includes the family-neutral
 *   `core/hal_irq.h`, keeping this header dependency-free (only <stdint.h>).
 *
 *   The glitch gate (optional, per-instance `min_edge_interval_ms`) uses
 *   `pic8_tick_get()` / `pic8_tick_elapsed_since()` for its timebase; only
 *   `encoder.c` includes `pic8_tick.h`. Only integer add/compare/shift are
 *   needed for decode, no multiply, so unlike `pic8-pid` this module does
 *   NOT link `pic8-math`.
 *
 *   See docs/ARCHITECTURE.md for the Gray-code table, the two-counter error
 *   model, and the at-most-two-encoders-per-port hardware ceiling, and
 *   docs/API.md for the per-function reference and wiring conventions.
 */

#ifndef ENCODER_H
#define ENCODER_H

#include <stdint.h>

/**
 * @brief  One quadrature decoder instance, caller-owned storage. One per
 *         A/B channel pair (at most two per PORTB, see docs/API.md).
 *
 * @details
 *   `position` is written only from ISR context (`encoder_update`, called
 *   from the application's RB-change callback); `encoder_get_position`
 *   reads it atomically. `error_count` counts impossible Gray transitions
 *   (always on, free); `glitch_count` counts edges rejected by the optional
 *   interval gate (only active when `min_edge_interval_ms != 0`). Keeping
 *   these as two counters (not one) is deliberate: a climbing `error_count`
 *   means "the software isn't keeping up or samples are being corrupted",
 *   a climbing `glitch_count` means "the gate is doing its job filtering
 *   real mechanical bounce"; conflating them would destroy the diagnostic
 *   value that was the actual point of adding either.
 */
typedef struct {
    uint8_t           pin_a;                /**< bit position 0-7 of channel A in the port byte. */
    uint8_t           pin_b;                /**< bit position 0-7 of channel B in the port byte. */
    volatile int32_t  position;             /**< x4 quadrature count; written only from ISR context. */
    uint8_t           last_state;           /**< 2-bit (a<<1|b), previous sample. */
    uint16_t          min_edge_interval_ms; /**< 0 = glitch gate disabled. */
    uint32_t          last_edge_tick;        /**< pic8_tick_get() at the last ACCEPTED edge;
                                                  *  meaningless when the gate is disabled. */
    volatile uint16_t error_count;          /**< impossible Gray transitions seen. */
    volatile uint16_t glitch_count;         /**< edges rejected by the interval gate. */
} encoder_t;

/**
 * @brief  Initialize an encoder instance.
 *
 * @param  enc                  the instance (caller-owned storage).
 * @param  pin_a                bit position 0-7 of channel A in the port byte.
 * @param  pin_b                bit position 0-7 of channel B in the port byte.
 * @param  min_edge_interval_ms 0 disables the glitch gate; otherwise two
 *                               valid-looking transitions closer than this
 *                               apart (in ms, on `pic8_tick`'s 1 ms timebase)
 *                               drop the second as a glitch.
 * @param  port_value            the current port byte (the same byte the
 *                               RB-change callback would receive, or a one-off
 *                               manual read at boot before interrupts are on).
 *                               Used to seed `last_state` so the very first
 *                               real edge is not misjudged against a state the
 *                               hardware was never in.
 *
 * @note   Swapping `pin_a` and `pin_b` inverts the count direction; do that
 *         rather than asking for a direction flag if your wiring produces the
 *         opposite sign (see docs/API.md).
 */
void encoder_init(encoder_t *enc, uint8_t pin_a, uint8_t pin_b,
                  uint16_t min_edge_interval_ms, uint8_t port_value);

/**
 * @brief  Re-sync `last_state` from `port_value` and zero `position`,
 *         `error_count`, `glitch_count`, and `last_edge_tick`. Gains
 *         (`pin_a`/`pin_b`/`min_edge_interval_ms`) are unchanged.
 *
 * For recovering after a fault without re-wiring the instance. The next
 * `encoder_update()` call for the next real edge then does not spuriously
 * register as an impossible transition (proving the resync actually took).
 */
void encoder_reset(encoder_t *enc, uint8_t port_value);

/**
 * @brief  Decode one port sample. Call from the application's
 *         `HAL_GPIO_RegisterChangeCallback` handler, once per registered
 *         instance, passing the same received port byte.
 *
 * @details
 *   Extracts this instance's 2-bit `(a<<1)|b` state, no-ops if it equals
 *   `last_state` (another instance's pins changed on the same byte, not an
 *   error), then (if the glitch gate is armed) drops a too-soon edge as a
 *   glitch without advancing `last_state`, else looks up the Gray-code step
 *   and applies it. An impossible transition (both bits flipped at once,
 *   `QUAD_TABLE` returns 0 for a genuine state change) increments
 *   `error_count` and leaves `position` unchanged.
 */
void encoder_update(encoder_t *enc, uint8_t port_value);

/**
 * @brief  Read the accumulated position atomically (interrupts disabled
 *         around the 32-bit read so an ISR update cannot tear it mid-read
 *         on an 8-bit core). Mirrors `pic8_tick_get()`'s pattern.
 */
int32_t  encoder_get_position(const encoder_t *enc);

/**
 * @brief  Read the impossible-transition counter atomically. A 16-bit read
 *         has the same tear risk in principle on an 8-bit core, so it is
 *         wrapped the same way for consistency even though a torn diagnostic
 *         counter is low-stakes.
 */
uint16_t encoder_get_error_count(const encoder_t *enc);

/** Read the rejected-by-gate counter atomically (see @ref encoder_get_error_count). */
uint16_t encoder_get_glitch_count(const encoder_t *enc);

#endif /* ENCODER_H */
