/**
 * @file    pic16f87xa_harness_target.c
 * @brief   Real-target implementation of the test harness (see
 *          core/pic16f87xa_harness.h).
 *
 * @details
 *   Linked by the XC8 Makefile. The host implementation is
 *   pic16f87xa_harness_sim.c; the build picks one, so neither this file
 *   nor the examples need `#ifdef`.
 *
 *   On a real target the CPU starts itself and hardware time advances on
 *   its own, so init/tick are no-ops; the run loop never exits, so
 *   harness_running always returns 1; and there is no stdout, so log is
 *   a no-op. pic16f87xa_harness_report is a shared inline in the header.
 */

#include "core/pic16f87xa_harness.h"

void pic16f87xa_harness_init(uint32_t cycles)
{
    (void)cycles;   /* The target starts itself; cycles are a host concept. */
}

void pic16f87xa_harness_tick(void)
{
    /* Real time advances on its own, nothing to pump. */
}

int pic16f87xa_harness_running(uint32_t iteration)
{
    (void)iteration;
    return 1;       /* Firmware runs forever. */
}

void pic16f87xa_harness_log(const char *fmt, ...)
{
    (void)fmt;      /* No stdout on the target. */
}