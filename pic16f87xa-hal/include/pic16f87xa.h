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

#ifdef __cplusplus
extern "C" {
#endif

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

/**
 * @brief   Compiler-agnostic weak attribute. Falls back to GCC/Clang
 *          `__attribute__((weak))` on host builds; on XC8 nothing is
 *          emitted (XC8 has no concept of weak symbols).
 */
#if defined(__XC8) || defined(_HITECH_)
  #define PIC16F87XA_WEAK
#else
  #define PIC16F87XA_WEAK   __attribute__((weak))
#endif

/* ───────────────────────── SFR mapping layer ────────────────────── */

/**
 * @defgroup PIC16F87XA_SFR Special Function Register mapping
 * @brief   Pre-processor layer that gives every SFR either a real
 *          volatile backing store (XC8 target) or a host-side memory cell
 *          (simulation backend).
 *
 * The same source code reads `pic16f87xa_sfr_read8(0x05)` (or uses the
 * convenience macro `PORTA`) on both targets, but the implementation is
 * chosen at compile time by `PIC16F87XA_USE_SIMULATOR`.
 *
 * On a real PIC, `PORTA` resolves to `*(volatile uint8_t *)&0x05`, which
 * the XC8 linker maps to the actual SFR. On the host, the same address
 * indexes a 256-byte RAM-backed register file that tests can poke.
 * @{
 */
#if defined(PIC16F87XA_USE_SIMULATOR)
  /* Host simulation backend — see src/sim/pic16f87xa_sim.c. */
  extern uint8_t pic16f87xa_sim_sfr[0x200];
  #define PIC16F87XA_SFR_PTR(addr)     (&pic16f87xa_sim_sfr[(uint16_t)(addr)])
  #define pic16f87xa_sfr_read8(addr)   (pic16f87xa_sim_sfr[(uint16_t)(addr)])
  #define pic16f87xa_sfr_write8(addr, v)                                      \
      do { pic16f87xa_sim_sfr[(uint16_t)(addr)] = (uint8_t)(v); } while (0)
#else
  /* Real target — direct volatile access. */
  #define PIC16F87XA_SFR_PTR(addr)     ((volatile uint8_t *)(addr))
  #define pic16f87xa_sfr_read8(addr)   (*(volatile uint8_t *)(addr))
  #define pic16f87xa_sfr_write8(addr, v)                                      \
      do { *(volatile uint8_t *)(addr) = (uint8_t)(v); } while (0)
#endif
/** @} */

/* Convenience: address of a register as a uint8_t lvalue. */
#if defined(PIC16F87XA_USE_SIMULATOR)
  #define PIC16F87XA_REG8(addr)  (pic16f87xa_sim_sfr[(uint16_t)(addr)])
#else
  #define PIC16F87XA_REG8(addr)  (*(volatile uint8_t *)(addr))
#endif

#ifdef __cplusplus
}
#endif

#endif /* PIC16F87XA_H */