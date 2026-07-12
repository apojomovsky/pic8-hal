/**
 * @file    example_smoke.c
 * @brief   Trivial smoke test: prove the shared harness contract links and
 *          runs against an empty PIC18 family backend.
 *
 * @details
 *   Phase 1 of the multi-family plan (docs/multi-family-plan.md) stands up
 *   the pic18f2455-hal skeleton with no real drivers yet. This example does
 *   nothing but exercise the four-function host/target harness contract
 *   (core/pic8_harness.h): init, a bounded tick loop, a log line, and a
 *   pass/fail report. It is the proof that `pic8_harness_*` really is
 *   family-blind, since PIC18's empty backend links against the exact same
 *   header and contract PIC16 uses.
 *
 *   No GPIO, no Timer0, no interrupts are touched. When the loop runs for
 *   exactly the requested number of cycles, the test passes.
 */

#include "pic18f2455.h"
#include "core/pic8_harness.h"

/** Bounded run length (host only). */
#define SIM_CYCLES  10UL

int main(void)
{
    pic8_harness_init(SIM_CYCLES);

    uint32_t ticks = 0U;
    for (uint32_t i = 0; pic8_harness_running(i); i++) {
        pic8_harness_tick();
        ticks++;
    }

    pic8_harness_log("smoke: %u ticks, device %s\n",
                     (unsigned)ticks, PIC18F2455_DEVICE_NAME);
    return pic8_harness_report(ticks == SIM_CYCLES);
}