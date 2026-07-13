/**
 * @file    pic18fxx5x_wdt_sleep_sim.c
 * @brief   Host-simulation implementation of HAL_WDT_Refresh /
 *          HAL_Sleep_Enter.
 *
 * @details
 *   Linked by the CMake host build. The companion target implementation is
 *   pic18fxx5x_wdt_sleep_target.c; the build picks one, so neither this
 *   file nor the examples need `#ifdef`. On the host there is no PIC18 CPU
 *   to stop and no WDT to refresh, so both are no-ops, exactly as the PIC16
 *   host build does. The shared BOR/POR status helpers live in
 *   pic18fxx5x_wdt_sleep.c.
 */

#include "core/pic18fxx5x_wdt_sleep.h"

void HAL_WDT_Refresh(void)
{
    /* No-op on the host sim: no WDT to refresh. */
}

void HAL_Sleep_Enter(void)
{
    /* No-op on the host sim: no CPU to halt. */
}