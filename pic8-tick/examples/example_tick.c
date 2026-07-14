/**
 * @file    example_tick.c
 * @brief   pic8-tick smoke test: verify the 1 ms timebase advances and
 *          `pic8_tick_delay_ms`/`pic8_tick_elapsed_since` behave.
 *
 * @details
 *   Builds and runs on both the host simulator and a real target. On host,
 *   `pic8_tick_delay_ms` pumps `pic8_harness_tick()` so simulated time
 *   advances (one tick = `prescaler*(PR2+1)*postscaler` sim steps, e.g. 5000
 *   at 20 MHz); on target the Timer2 ISR advances the counter in real time.
 *   The test delays 10 ms and 5 ms and checks the elapsed counts are in
 *   range (the delay guarantees at least N ms, overshooting by at most ~1
 *   tick), then reports PASS/FAIL via the harness.
 */

#include "pic8_tick.h"
#include "core/pic8_harness.h"

#ifndef FOSC_HZ
#define FOSC_HZ 20000000UL
#endif

#define SIM_CYCLES 4000000UL

int main(void)
{
    pic8_harness_init(SIM_CYCLES);
    pic8_tick_init(FOSC_HZ);

    uint32_t t0 = pic8_tick_get();
    pic8_tick_delay_ms(10u);
    uint32_t e10 = pic8_tick_get() - t0;
    pic8_harness_log("tick: delay(10) -> %lu ms\n", (unsigned long)e10);

    uint32_t s = pic8_tick_get();
    pic8_tick_delay_ms(5u);
    uint32_t e5 = pic8_tick_elapsed_since(s);
    pic8_harness_log("tick: delay(5)  -> %lu ms\n", (unsigned long)e5);

    int ok = (e10 >= 10u) && (e10 <= 12u) && (e5 >= 5u) && (e5 <= 7u);
    return pic8_harness_report(ok);
}