/**
 * @file    peripherals/pic16f87xa_psp.h
 * @brief   Parallel Slave Port driver (40/44-pin only).
 *
 * @details
 *   Source: DS39582B §4.5 (PORTE / TRISE — Register 4-9).
 *
 *   The PSP exposes an 8-bit parallel bus on PORTD (PSP0..PSP7) plus
 *   three control lines on PORTE (RE0/RD, RE1/WR, RE2/CS) when the
 *   PSPMODE bit (TRISE<4>) is set.  An external master can then
 *   read/write the part's data through these pins.
 *
 *   ⚠ This driver is **40/44-pin only** — PIC16F873A and PIC16F876A
 *   do not have a PSP.  The compile-time check
 *   `PIC16F87XA_FAMILY_HAS_PSP` (defined in pic16f87xa.h) lets the
 *   rest of the application avoid building this driver on 28-pin
 *   parts.
 *
 *   The driver manages:
 *     - PSPMODE enable.
 *     - Input/output buffer flag helpers (IBF, OBF, IBOV).
 *     - PSP interrupt enable (PSPIE in TRISE<0>; the IRQ itself is
 *       PIR1<PSPIF> and PIE1<PSPIE>, both only present on 40/44-pin).
 *
 *   The driver only programs the configuration register.  On real
 *   silicon an external master drives CS/RD/WR; on the sim backend
 *   the host application drives state through pic16f87xa_sim.h
 *   helpers.
 */

#ifndef PIC16F87XA_PSP_H
#define PIC16F87XA_PSP_H

#include "pic16f87xa.h"
#include "pic16f87xa_sfr.h"

#ifdef __cplusplus
extern "C" {
#endif

#if !PIC16F87XA_FAMILY_HAS_PSP
#error "pic16f87xa_psp.h is for 40/44-pin PIC16F87XA parts only"
#endif

PIC16F87XA_StatusTypeDef HAL_PSP_Init(void (*callback)(void));
PIC16F87XA_StatusTypeDef HAL_PSP_DeInit(void);

/** Enable Parallel Slave Port mode (TRISE<PSPMODE> = 1). */
void HAL_PSP_Enable(void);

/** Disable Parallel Slave Port mode. */
void HAL_PSP_Disable(void);

/** Returns 1 if the PSP input buffer is full (TRISE<IBF>). */
uint8_t HAL_PSP_IsInputBufferFull(void);

/** Returns 1 if the PSP output buffer is full (TRISE<OBF>). */
uint8_t HAL_PSP_IsOutputBufferFull(void);

/** Returns 1 if an input buffer overflow occurred (TRISE<IBOV>). */
uint8_t HAL_PSP_HasInputOverflow(void);

/** Clear the IBOV flag. Must be done in software. */
void HAL_PSP_ClearInputOverflow(void);

void PSP_IRQHandler(void) PIC16F87XA_WEAK;

#ifdef __cplusplus
}
#endif

#endif /* PIC16F87XA_PSP_H */
