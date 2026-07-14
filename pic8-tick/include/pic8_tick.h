/**
 * @file    pic8_tick.h
 * @brief   Family-agnostic 1 ms timebase: `pic8_tick_get()` / `pic8_tick_delay_ms()`,
 *          the STM32Cube `HAL_GetTick`/`HAL_Delay` equivalent for 8-bit PICs.
 *
 * @details
 *   A millisecond timebase built on the HAL's Timer2 (auto-reload, so no
 *   manual reload in the ISR -- unlike the task manager's Timer0). A Timer2
 *   period-match ISR increments a volatile 32-bit counter; this header's
 *   functions read it. Works on every PIC this repo supports (PIC16F87XA and
 *   PIC18F2455/2550/4455/4550) and on the host simulator.
 *
 *   The public API is family-neutral (only <stdint.h>); the Timer2 contract
 *   is pulled in only by the implementation via the family-neutral
 *   `peripherals/hal_timer2.h` shim. The tick ISR is installed through the
 *   Timer2 handle's `OverflowCallback` (the HAL owns `TIMER2_IRQHandler`),
 *   exactly the pattern `pic8-taskmgr` uses for its Timer0 tick.
 *
 *   On the host sim, simulated time advances only when the main loop pumps
 *   `pic8_harness_tick()`; `pic8_tick_delay_ms()` pumps it internally so it
 *   works unchanged on both builds. On a real target the Timer2 ISR advances
 *   the counter in the background and `pic8_harness_tick()` is a no-op.
 *
 *   See docs/ARCHITECTURE.md for the timebase math and the host/target
 *   execution models, and docs/API.md for the per-function reference.
 */

#ifndef PIC8_TICK_H
#define PIC8_TICK_H

#include <stdint.h>

/**
 * @brief  Start the 1 ms timebase. Configures Timer2 for a ~1 ms period-match
 *         from @p fosc_hz (the PR2/prescaler/postscaler are computed for the
 *         closest achievable 1 ms; for common Fosc -- 4/8/16/20/32/48 MHz --
 *         it is exact) and enables its ISR. Call once at startup.
 * @param  fosc_hz  the system oscillator frequency in Hz (e.g. 20000000UL).
 */
void pic8_tick_init(uint32_t fosc_hz);

/**
 * @brief  Read the elapsed milliseconds since `pic8_tick_init`. Monotonic;
 *         wraps every ~49.7 days (2^32 ms). The 32-bit read is made atomic
 *         against the ISR (interrupts disabled around the read) so a
 *         mid-update tear cannot occur.
 * @return the millisecond tick count.
 */
uint32_t pic8_tick_get(void);

/**
 * @brief  Block for @p ms milliseconds. On the host sim this pumps
 *         `pic8_harness_tick()` so simulated time advances; on a real target
 *         it spins while the Timer2 ISR advances the counter. Guarantees at
 *         least @p ms (may overshoot by up to ~1 tick).
 */
void pic8_tick_delay_ms(uint32_t ms);

/**
 * @brief  Non-blocking elapsed-time helper: `pic8_tick_get() - t0`, the
 *         idiom for `if (pic8_tick_elapsed_since(t0) >= timeout)` without
 *         blocking. Wraparound-safe (unsigned subtraction).
 */
uint32_t pic8_tick_elapsed_since(uint32_t t0);

#endif /* PIC8_TICK_H */