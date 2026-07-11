/**
 * @file    pic16f87xa.h
 * @brief   PIC16F87XA family — top-level types, status codes, build-time device
 *          selection, and the simulated/real-target SFR mapping layer.
 *
 * @details
 *   This header is the single entry point for the PIC16F87XA HAL. It mirrors
 *   the role of `stm32fxxx_hal.h` in STMicroelectronics' HAL: pulling in
 *   standard integer types, common macros, status enums, and the device-
 *   specific include that defines every SFR.
 *
 *   The datasheet (DS39582B) is the authoritative source for every constant,
 *   bit name, reset value and behavior implemented in this HAL. Every
 *   peripheral header carries a citation of the datasheet section it maps to.
 *
 * Target family (DS39582B §1.0, Table 1-1):
 *   - PIC16F873A — 28-pin,  4 KW flash, 192 B RAM, 128 B EEPROM, 5 ADC ch.
 *   - PIC16F874A — 40-pin,  4 KW flash, 192 B RAM, 128 B EEPROM, 8 ADC ch.,
 *                   PORTD + PORTE (with PSP).
 *   - PIC16F876A — 28-pin,  8 KW flash, 368 B RAM, 256 B EEPROM, 5 ADC ch.
 *   - PIC16F877A — 40-pin,  8 KW flash, 368 B RAM, 256 B EEPROM, 8 ADC ch.,
 *                   PORTD + PORTE (with PSP).
 *
 * @copyright © 2003 Microchip Technology Inc. (datasheet DS39582B).
 */

#ifndef PIC16F87XA_H
#define PIC16F87XA_H

/* ───────────────────────── standard types ───────────────────────── */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* ───────────────────────── build-time device selection ──────────── */

/**
 * @defgroup  PIC16F87XA_Device Device Selection
 * @brief     Select exactly one target device before including any
 *            peripheral header. Defaults to PIC16F877A when none is set.
 * @{
 */
#if !defined(PIC16F873A) && !defined(PIC16F874A) && \
    !defined(PIC16F876A) && !defined(PIC16F877A)
#define PIC16F877A   1
#endif

#if defined(PIC16F873A) + defined(PIC16F874A) + \
    defined(PIC16F876A) + defined(PIC16F877A) > 1
#error "Define exactly one of PIC16F873A / PIC16F874A / PIC16F876A / PIC16F877A."
#endif

#if   defined(PIC16F873A)
  #define PIC16F87XA_FAMILY_FLASH_KW   4
  #define PIC16F87XA_FAMILY_RAM_BYTES  192
  #define PIC16F87XA_FAMILY_EEPROM_B   128
  #define PIC16F87XA_FAMILY_ADC_CH     5
  #define PIC16F87XA_FAMILY_HAS_PORTD  0
  #define PIC16F87XA_FAMILY_HAS_PORTE  0
  #define PIC16F87XA_FAMILY_HAS_PSP    0
  #define PIC16F87XA_DEVICE_NAME       "PIC16F873A"
#elif defined(PIC16F874A)
  #define PIC16F87XA_FAMILY_FLASH_KW   4
  #define PIC16F87XA_FAMILY_RAM_BYTES  192
  #define PIC16F87XA_FAMILY_EEPROM_B   128
  #define PIC16F87XA_FAMILY_ADC_CH     8
  #define PIC16F87XA_FAMILY_HAS_PORTD  1
  #define PIC16F87XA_FAMILY_HAS_PORTE  1
  #define PIC16F87XA_FAMILY_HAS_PSP    1
  #define PIC16F87XA_DEVICE_NAME       "PIC16F874A"
#elif defined(PIC16F876A)
  #define PIC16F87XA_FAMILY_FLASH_KW   8
  #define PIC16F87XA_FAMILY_RAM_BYTES  368
  #define PIC16F87XA_FAMILY_EEPROM_B   256
  #define PIC16F87XA_FAMILY_ADC_CH     5
  #define PIC16F87XA_FAMILY_HAS_PORTD  0
  #define PIC16F87XA_FAMILY_HAS_PORTE  0
  #define PIC16F87XA_FAMILY_HAS_PSP    0
  #define PIC16F87XA_DEVICE_NAME       "PIC16F876A"
#else  /* PIC16F877A */
  #define PIC16F87XA_FAMILY_FLASH_KW   8
  #define PIC16F87XA_FAMILY_RAM_BYTES  368
  #define PIC16F87XA_FAMILY_EEPROM_B   256
  #define PIC16F87XA_FAMILY_ADC_CH     8
  #define PIC16F87XA_FAMILY_HAS_PORTD  1
  #define PIC16F87XA_FAMILY_HAS_PORTE  1
  #define PIC16F87XA_FAMILY_HAS_PSP    1
  #define PIC16F87XA_DEVICE_NAME       "PIC16F877A"
#endif
/** @} */

/* ───────────────────────── HAL status / error codes ─────────────── */

/**
 * @brief   Standard HAL status codes. Mirrors `HAL_StatusTypeDef` from
 *          STM32Cube so users familiar with that HAL get the same flow.
 */
typedef enum {
    PIC16F87XA_OK       = 0x00U, /**< Operation completed successfully. */
    PIC16F87XA_ERROR    = 0x01U, /**< Generic error. */
    PIC16F87XA_BUSY     = 0x02U, /**< Resource busy with ongoing operation. */
    PIC16F87XA_TIMEOUT  = 0x03U, /**< Operation timed out. */
    PIC16F87XA_INVALID  = 0x04U  /**< Invalid parameter or state. */
} PIC16F87XA_StatusTypeDef;

/* ───────────────────────── bit / register helpers ───────────────── */

/**
 * @name    Bit manipulation
 * @brief   Standard set/clr/test helpers — preferred over hand-rolled masks.
 * @{
 */
#define PIC16F87XA_BIT(n)                       (1U << (n))
#define PIC16F87XA_BIT_SET(reg, mask)           ((reg) |=  (uint8_t)(mask))
#define PIC16F87XA_BIT_CLR(reg, mask)           ((reg) &= ~(uint8_t)(mask))
#define PIC16F87XA_BIT_TGL(reg, mask)           ((reg) ^=  (uint8_t)(mask))
#define PIC16F87XA_BIT_READ(reg, mask)          ((reg) &   (uint8_t)(mask))
/** @} */

/* ───────────── platform: SFR mapping + weak attribute ───────────── */
/**
 * @defgroup PIC16F87XA_SFR Special Function Register mapping
 * @brief   How every SFR is stored and how the weak attribute is spelled.
 *
 * The same source code reads `pic16f87xa_sfr_read8(0x05)` (or uses the
 * convenience macro `PIC16F87XA_REG8`) on both builds, but the
 * implementation is chosen by the build's include path, not by `#ifdef`:
 * the CMake host build resolves `pic16f87xa_platform.h` to
 * `include/host/...` (a 512-byte memory-backed register file), and the
 * XC8 Makefile resolves it to `include/target/...` (direct volatile
 * dereference of the literal SFR address). See those two headers for the
 * exact macros; @ref PIC16F87XA_WEAK is likewise defined there.
 * @{
 */
#include "pic16f87xa_platform.h"
/** @} */

#endif /* PIC16F87XA_H */
