/**
 * @file    core/pic18fxx5x_wdt_sleep.h
 * @brief   CPU-level helpers: Watchdog Timer, Brown-out Reset, Sleep.
 *
 * @details
 *   Source: DS39632E §4.0 (RCON, Register 4-1), §9.0 (RCON<IPEN>),
 *   §14.x (WDT), §3.0 (Power-down Mode / Sleep).
 *
 *   The API matches pic16f87xa_wdt_sleep.h (same function names/signatures)
 *   so consumer code is portable; the register-level difference is that
 *   PIC18 folds the reset-status bits (TO/PD/POR/BOR) into RCON rather
 *   than PIC16's separate PCON, and the bit positions differ:
 *     - RCON<TO>  (bit 3): Watchdog Time-out flag (1 = not timed out).
 *     - RCON<PD>  (bit 2): Power-down (Sleep) detection flag (1 = not in
 *       Sleep).
 *     - RCON<POR> (bit 1): Power-on Reset status.
 *     - RCON<BOR> (bit 0): Brown-out Reset status.
 *
 *   The Watchdog Timer is enabled by the WDTEN configuration bit (set at
 *   flash time via `#pragma config WDT = ON`); the runtime API only
 *   provides a refresh helper. Once enabled, the user MUST call
 *   @ref HAL_WDT_Refresh periodically or the chip will reset.
 *
 *   The SLEEP instruction is invoked via inline asm. On the host
 *   simulation, HAL_Sleep_Enter is a no-op because there is no PIC18 CPU
 *   to stop.
 *
 *   The build-mode-specific helpers HAL_WDT_Refresh and HAL_Sleep_Enter
 *   live in pic18fxx5x_wdt_sleep_sim.c (host) and
 *   pic18fxx5x_wdt_sleep_target.c (XC8), selected at link time; the BOR/POR
 *   status helpers are shared and stay in pic18fxx5x_wdt_sleep.c.
 */

#ifndef PIC18FXX5X_WDT_SLEEP_H
#define PIC18FXX5X_WDT_SLEEP_H

#include "pic18fxx5x.h"
#include "pic18fxx5x_sfr.h"

/**
 * @brief  Refresh the Watchdog Timer by executing the `clrwdt` asm
 *         instruction (or the equivalent no-op in the simulator).
 *
 *         On a real target this MUST be called more often than the WDT
 *         period (DS39632E §14.x). On the host simulator it's a no-op.
 */
void HAL_WDT_Refresh(void);

/**
 * @brief  Enter Power-down (Sleep) mode. On a real target this is the
 *         `sleep` asm instruction; the CPU halts until any enabled
 *         interrupt wakes it (DS39632E §3.0).
 *
 *         On the host simulator this is a no-op; callers should continue
 *         to drive pic18_sim_step().
 */
void HAL_Sleep_Enter(void);

/**
 * @brief  Returns 1 if the last reset was a Brown-out Reset
 *         (RCON<BOR>). Clear after reading via @ref HAL_BOR_ClearFlag.
 */
uint8_t HAL_BOR_GetStatus(void);

/** Clear RCON<BOR> (write 0). */
void HAL_BOR_ClearFlag(void);

/**
 * @brief  Returns 1 if the device just powered on (RCON<POR>).
 *         Set on Power-on Reset (DS39632E Register 4-1).
 */
uint8_t HAL_POR_GetStatus(void);

/** Clear RCON<POR> (write 0). */
void HAL_POR_ClearFlag(void);

#endif /* PIC18FXX5X_WDT_SLEEP_H */