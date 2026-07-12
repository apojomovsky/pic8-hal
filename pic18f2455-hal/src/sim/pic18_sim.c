/**
 * @file    pic18_sim.c
 * @brief   PIC18F2455 family host simulation backend (Phase 1 minimal).
 *
 * @details
 *   Linked by the CMake host build only; the XC8 Makefile does not compile
 *   this file. Defines the memory-backed register file referenced by
 *   include/host/pic18_platform.h and the three Phase 1 sim hooks declared
 *   in pic18f2455_sim.h.
 *
 *   Phase 1: `reset` zeroes the register file, `step` is a no-op, and the
 *   IRQ callback is stored but never fired (no peripherals modeled yet).
 *   Phase 2 grows this into a real Timer0/GPIO stepping model mirroring
 *   pic16f87xa_sim.c; the storage array and the three hooks here are the
 *   seams Phase 2 fills in.
 */

#include "pic18f2455_sim.h"
#include "pic18_platform.h"

#include <string.h>

/* Memory-backed register file, referenced by include/host/pic18_platform.h.
 * Provisionally the full 12-bit data-memory footprint; see the platform
 * header for the Phase 2 sizing note. */
uint8_t pic18_sim_sfr[0x1000];

/* The IRQ callback registered by the host harness; the family dispatcher
 * in pic18_irq_dispatch.c. Phase 1: stored, never fired. */
static pic18_sim_irq_cb_t g_irq_cb;

void pic18_sim_reset(void)
{
    memset(pic18_sim_sfr, 0, sizeof pic18_sim_sfr);
    g_irq_cb = (pic18_sim_irq_cb_t)0;
}

void pic18_sim_step(uint32_t ticks)
{
    /* Phase 1: no peripherals to advance. The argument is intentionally
     * unused; Phase 2 consumes it to step Timer0 etc. */
    (void)ticks;
}

void pic18_sim_set_irq_callback(pic18_sim_irq_cb_t cb)
{
    g_irq_cb = cb;
}