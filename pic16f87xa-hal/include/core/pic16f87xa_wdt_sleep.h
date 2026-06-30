/**
 * @file    core/pic16f87xa_wdt_sleep.h
 * @brief   CPU-level helpers: Watchdog Timer, Brown-out Reset, Sleep.
 *
 * @details
 *   Source: DS39582B §14.10 (PCON — Register 14-2), §14.13 (WDT),
 *   §14.14 (Power-down Mode / Sleep).
 *
 *   The Watchdog Timer is enabled by the WDT configuration bit
 *   (Register 14-1) which can only be programmed at flash time — the
 *   runtime API only provides a refresh helper. Once the WDT is
 *   enabled (WDTEN=1 in CONFIG), the user MUST call
 *   @ref HAL_WDT_Refresh periodically or the chip will reset.
 *
 *   The SLEEP instruction is invoked via inline asm. On the host
 *   simulation, HAL_Sleep_Enter is a no-op because there is no
 *   PIC16F87XA CPU to stop.
 *
 *   BOR (Brown-out Reset) state is observable through PCON<BOR>
 *   (bit 0, DS39582B §14.10). The POR bit (bit 1) is set on power-on
 *   reset and unaffected by other resets.
 *
 *   The config-bit control is left to the user's MPLAB X / XC8 setup;
 *   this file does not emit `#pragma config` directives because the
 *   bit definitions live in user code (and vary by IDE).
 */

#ifndef PIC16F87XA_WDT_SLEEP_H
#define PIC16F87XA_WDT_SLEEP_H

#include "pic16f87xa.h"
#include "pic16f87xa_sfr.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Refresh the Watchdog Timer by executing the `clrwdt` asm
 *         instruction (or the equivalent macro in the simulator).
 *
 *         On a real target this MUST be called more often than the
 *         WDT period (typ. 18 ms × prescaler, §17.0 #31). On the
 *         host simulator it's a no-op.
 */
void HAL_WDT_Refresh(void);

/**
 * @brief  Enter Power-down (Sleep) mode. On a real target this is
 *         implemented with the `sleep` asm instruction; the CPU
 *         halts until any enabled interrupt wakes it (§14.14).
 *
 *         On the host simulator this is a no-op; callers should
 *         continue to drive pic16f87xa_sim_step().
 */
void HAL_Sleep_Enter(void);

/**
 * @brief  Returns 1 if the last reset was a Brown-out Reset
 *         (PCON<BOR>).  Clear after reading via @ref HAL_BOR_ClearFlag.
 */
uint8_t HAL_BOR_GetStatus(void);

/** Clear PCON<BOR>. The POR bit is write-1-to-clear. */
void HAL_BOR_ClearFlag(void);

/**
 * @brief  Returns 1 if the device just powered on (PCON<POR>).
 *         Set only on Power-on Reset.
 */
uint8_t HAL_POR_GetStatus(void);

/** Clear PCON<POR>. */
void HAL_POR_ClearFlag(void);

#ifdef __cplusplus
}
#endif

#endif /* PIC16F87XA_WDT_SLEEP_H */
