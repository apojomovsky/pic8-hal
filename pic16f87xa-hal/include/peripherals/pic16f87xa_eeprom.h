/**
 * @file    peripherals/pic16f87xa_eeprom.h
 * @brief   Data EEPROM driver.
 *
 * @details
 *   Source: DS39582B §3.0 (Data EEPROM Memory), Table 3-1, Register 3-1
 *   (EECON1), §3.4 (writing sequence), §3.5 (reading sequence).
 *
 *   Wiring on the part:
 *     - 128 / 256 bytes of data EEPROM (Table 1-1).
 *     - EEDATA (Bank 2, 0x10C) — 8-bit data register.
 *     - EEADR  (Bank 2, 0x10D) — 8-bit low address register.
 *     - EEDATH (Bank 2, 0x10E) + EEADRH (Bank 2, 0x10F) — flash-only,
 *       ignored for EEPROM access.
 *     - EECON1 (Bank 3, 0x18C): RD, WR, WREN, WRERR, EEIF.
 *     - EECON2 (Bank 3, 0x18D) — magic unlock: must write 0x55 then 0xAA
 *       (DS39582B §3.4 / Example 3-1).
 *
 *   The driver hides the unlock sequence from the caller. Writes are
 *   non-blocking: the driver returns as soon as the WR bit is set;
 *   the caller detects completion by polling EEIF (PIR2<4>), which
 *   fires when the write cycle ends (DS39582B §3.4 step 6).
 *
 *   Reset state: EECON1 = 0x00 (RD/WR cleared).
 */

#ifndef PIC16F87XA_EEPROM_H
#define PIC16F87XA_EEPROM_H

#include "pic16f87xa.h"
#include "pic16f87xa_sfr.h"

/**
 * @brief  Initialize the EEPROM driver. Programs PIE2<EEIE> if
 *         `callback` is non-NULL. */
PIC16F87XA_StatusTypeDef HAL_EEPROM_Init(void (*callback)(void));

/** Disable the EEPROM module and clear EEIF. */
PIC16F87XA_StatusTypeDef HAL_EEPROM_DeInit(void);

/**
 * @brief  Read one byte from data EEPROM. Loads EEADR with `addr`,
 *         sets EECON1<RD>, and returns the byte from EEDATA.
 *
 *         Per §3.5: EEADR must be loaded first, then RD.
 */
uint8_t HAL_EEPROM_ReadByte(uint8_t addr);

/**
 * @brief  Write one byte to data EEPROM at `addr`. Performs the
 *         mandatory unlock sequence (0x55 → 0xAA → WR).
 *
 * @return PIC16F87XA_OK on success, PIC16F87XA_ERROR if a previous
 *         write was aborted (WRERR set).
 */
PIC16F87XA_StatusTypeDef HAL_EEPROM_WriteByte(uint8_t addr, uint8_t data);

/** Read a contiguous block. */
void HAL_EEPROM_ReadBuffer(uint8_t start, uint8_t *buf, uint8_t len);

/** Write a contiguous block. */
PIC16F87XA_StatusTypeDef HAL_EEPROM_WriteBuffer(uint8_t start,
                                                const uint8_t *buf,
                                                uint8_t len);

/** Returns 1 if EEIF is set. */
uint8_t HAL_EEPROM_IsWriteComplete(void);

/** Clear EEIF (must be cleared in the user's IRQ). */
void HAL_EEPROM_ClearITFlag(void);

void EEPROM_IRQHandler(void) PIC16F87XA_WEAK;

#endif /* PIC16F87XA_EEPROM_H */
