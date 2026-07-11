/**
 * @file    pic8_harness_target.c
 * @brief   Real-target implementation of the test harness (see
 *          core/pic8_harness.h). Shared by every 8-bit PIC family.
 *
 * @details
 *   Linked by each family's XC8 Makefile. The host implementation is the
 *   family's `pic8_harness_sim.c`; the build picks one, so neither this
 *   file nor the examples need `#ifdef`.
 *
 *   On a real target the CPU starts itself and hardware time advances on
 *   its own, so init/tick are no-ops; the run loop never exits, so
 *   harness_running always returns 1; and there is no stdout, so log is
 *   a no-op. pic8_harness_report is a shared inline in the header. None
 *   of this references any register, bank, or vector, so the same object
 *   file links against PIC16F87XA, PIC18F2455, and any later family.
 */

#include "core/pic8_harness.h"

void pic8_harness_init(uint32_t cycles)
{
    (void)cycles;   /* The target starts itself; cycles are a host concept. */
}

void pic8_harness_tick(void)
{
    /* Real time advances on its own, nothing to pump. */
}

int pic8_harness_running(uint32_t iteration)
{
    (void)iteration;
    return 1;       /* Firmware runs forever. */
}

void pic8_harness_log(const char *fmt, ...)
{
    (void)fmt;      /* No stdout on the target. */
}