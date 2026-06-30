/**
 * @file    pic16f87xa_wdt_sleep.c
 * @brief   WDT refresh, Sleep, BOR/POR status — implementation.
 *
 *   On a real XC8 target, HAL_WDT_Refresh and HAL_Sleep_Enter are
 *   inline asm. On the host simulation backend they are no-ops.
 *
 *   We branch on PIC16F87XA_USE_SIMULATOR (set by the sim build) to
 *   pick the implementation; the sim build never emits SLEEP/CLRWDT
 *   asm instructions.
 */

#include "core/pic16f87xa_wdt_sleep.h"

void HAL_WDT_Refresh(void)
{
#if defined(PIC16F87XA_USE_SIMULATOR)
    /* No-op: the sim does not model a watchdog timer. */
#else
    asm("clrwdt");
#endif
}

void HAL_Sleep_Enter(void)
{
#if defined(PIC16F87XA_USE_SIMULATOR)
    /* No-op: the sim does not stop execution. Callers should keep
     * driving pic16f87xa_sim_step() to advance time. */
#else
    asm("sleep");
#endif
}

/* PCON bits (DS39582B §14.10, Register 14-2). */
#define PIC_PCON_BOR   PIC16F87XA_BIT(0)
#define PIC_PCON_POR   PIC16F87XA_BIT(1)

uint8_t HAL_BOR_GetStatus(void)
{
    return (PIC16F87XA_REG8(PIC_REG_PCON) & PIC_PCON_BOR) ? 1U : 0U;
}

void HAL_BOR_ClearFlag(void)
{
    PIC16F87XA_BIT_CLR(PIC16F87XA_REG8(PIC_REG_PCON), PIC_PCON_BOR);
}

uint8_t HAL_POR_GetStatus(void)
{
    return (PIC16F87XA_REG8(PIC_REG_PCON) & PIC_PCON_POR) ? 1U : 0U;
}

void HAL_POR_ClearFlag(void)
{
    PIC16F87XA_BIT_CLR(PIC16F87XA_REG8(PIC_REG_PCON), PIC_PCON_POR);
}