/**
 * @file    pic16f87xa_harness_sim.c
 * @brief   Host-simulation implementation of the test harness (see
 *          core/pic16f87xa_harness.h).
 *
 * @details
 *   Linked by the CMake host build. The companion target implementation
 *   is pic16f87xa_harness_target.c; the build picks one, so neither this
 *   file nor the examples need `#ifdef`.
 */

#include "core/pic16f87xa_harness.h"
#include "core/pic16f87xa_interrupt.h"   /* pic16f87xa_dispatch_all_irqs */
#include "pic16f87xa_sim.h"

#include <stdio.h>
#include <stdarg.h>

/** Bounded run length set by the last harness_init() call. */
static uint32_t g_cycles = 0U;

void pic16f87xa_harness_init(uint32_t cycles)
{
    g_cycles = cycles;
    pic16f87xa_sim_reset();
    /* One sim callback that fans out to every peripheral handler — the
     * host analogue of the real target's single interrupt vector. */
    pic16f87xa_sim_set_irq_callback(pic16f87xa_dispatch_all_irqs);
}

void pic16f87xa_harness_tick(void)
{
    pic16f87xa_sim_step(1);
}

int pic16f87xa_harness_running(uint32_t iteration)
{
    return (iteration < g_cycles) ? 1 : 0;
}

void pic16f87xa_harness_log(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    (void)vprintf(fmt, ap);
    va_end(ap);
}