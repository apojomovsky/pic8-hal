/**
 * @file    pic16f87xa_wdt_sleep.c
 * @brief   BOR / POR status helpers — shared by both builds.
 *
 * @details
 *   The build-mode-specific helpers HAL_WDT_Refresh and HAL_Sleep_Enter
 *   live in pic16f87xa_wdt_sleep_sim.c (host) and
 *   pic16f87xa_wdt_sleep_target.c (XC8), selected at link time. The BOR/POR
 *   status helpers below are identical on both builds — they just read and
 *   clear bits in PCON through the platform SFR macro — so they stay here
 *   as one shared translation unit.
 */

#include "core/pic16f87xa_wdt_sleep.h"

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