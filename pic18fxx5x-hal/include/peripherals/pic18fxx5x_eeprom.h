/**
 * @file    peripherals/pic18fxx5x_eeprom.h
 * @brief   Data EEPROM driver.
 *
 * @details
 *   Source: DS39632E §7.0 (Data EEPROM Memory), Register 7-1 (EECON1),
 *   §7.2 (writing), §7.1 (reading).
 *
 *   The API mirrors pic16f87xa_eeprom.h — the same `HAL_EEPROM_*`
 *   functions and weak `EEPROM_IRQHandler` — so consumer code is portable.
 *   The PIC18 moves the EEPROM registers into the Access Bank (no bank
 *   switching) and adds the EEPGD/CFGS select bits in EECON1; for data
 *   EEPROM access the driver keeps EEPGD = 0 and CFGS = 0.
 *
 *   Wiring on the part:
 *     - 256 bytes of data EEPROM (DS39632E §1.0, Table 1-1).
 *     - EEDATA (0xFA8), 8-bit data register.
 *     - EEADR  (0xFA9), 8-bit address register (0..255; no EEADRH on
 *       this family).
 *     - EECON1 (0xFA6): RD, WR, WREN, WRERR, FREE, CFGS, EEPGD.
 *     - EECON2 (0xFA7), magic unlock: write 0x55 then 0xAA (§7.2).
 *
 *   The driver hides the unlock sequence from the caller. Writes are
 *   non-blocking: the driver returns as soon as the WR bit is set; the
 *   caller detects completion by polling EEIF (PIR2<4>), which fires when
 *   the write cycle ends.
 *
 *   Reset state (DS39632E Table 5-1): EECON1 = 0x00.
 */

#ifndef PIC18FXX5X_EEPROM_H
#define PIC18FXX5X_EEPROM_H

#include "pic18fxx5x.h"
#include "pic18fxx5x_sfr.h"

/**
 * @brief  Initialize the EEPROM driver. Programs PIE2<EEIE> if
 *         `callback` is non-NULL. */
HAL_StatusTypeDef HAL_EEPROM_Init(void (*callback)(void));

/** Disable the EEPROM module and clear EEIF. */
HAL_StatusTypeDef HAL_EEPROM_DeInit(void);

/**
 * @brief  Read one byte from data EEPROM. Loads EEADR with `addr`,
 *         sets EECON1<RD> (with EEPGD=0), and returns the byte from
 *         EEDATA. Per §7.1: EEADR must be loaded first, then RD.
 */
uint8_t HAL_EEPROM_ReadByte(uint8_t addr);

/**
 * @brief  Write one byte to data EEPROM at `addr`. Performs the mandatory
 *         unlock sequence (0x55 -> 0xAA -> WR), with EEPGD=0.
 * @return HAL_OK on success, HAL_ERROR if a previous write was aborted
 *         (WRERR set).
 */
HAL_StatusTypeDef HAL_EEPROM_WriteByte(uint8_t addr, uint8_t data);

/** Read a contiguous block. */
void HAL_EEPROM_ReadBuffer(uint8_t start, uint8_t *buf, uint8_t len);

/** Write a contiguous block. */
HAL_StatusTypeDef HAL_EEPROM_WriteBuffer(uint8_t start,
                                              const uint8_t *buf,
                                              uint8_t len);

/** Returns 1 if EEIF is set (write cycle complete). */
uint8_t HAL_EEPROM_IsWriteComplete(void);

/** Clear EEIF (must be cleared in the user's IRQ). */
void HAL_EEPROM_ClearITFlag(void);

void EEPROM_IRQHandler(void) PIC8_WEAK;

#endif /* PIC18FXX5X_EEPROM_H */