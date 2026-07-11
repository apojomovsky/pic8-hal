/**
 * @file    core/hal_status.h
 * @brief   Status codes and bit macros shared by every 8-bit PIC HAL
 *          family. Architecture-blind: no register, no bank, no vector.
 *
 * @details
 *   This is the one header in the shared layer (`pic8-common/`) that every
 *   per-family HAL tree (`pic16f87xa-hal/`, `pic18f2455-hal/`, ...) includes
 *   unmodified. It mirrors the role of `HAL_StatusTypeDef` and the bit
 *   helpers in STM32Cube's `stm32fxxx_hal_def.h`: the names and values are
 *   identical on every family, so consumer code (the task manager, the
 *   examples) never sees a family-specific status enum or bit macro.
 *
 *   What does NOT live here: SFR access, bank/BSR addressing, the weak-
 *   attribute spelling, the IRQ enum, anything that differs between
 *   PIC16F87XA and PIC18F2455. Those stay in each family's `platform.h`
 *   and IRQ backend, which all implement the same contract spelled out
 *   by this shared layer's headers.
 */

#ifndef HAL_STATUS_H
#define HAL_STATUS_H

#include <stdint.h>

/* ───────────────────────── HAL status / error codes ─────────────── */

/**
 * @brief   Standard HAL status codes. Mirrors `HAL_StatusTypeDef` from
 *          STM32Cube so users familiar with that HAL get the same flow.
 *          Identical on every 8-bit PIC family.
 */
typedef enum {
    HAL_OK      = 0x00U, /**< Operation completed successfully. */
    HAL_ERROR   = 0x01U, /**< Generic error. */
    HAL_BUSY    = 0x02U, /**< Resource busy with ongoing operation. */
    HAL_TIMEOUT = 0x03U, /**< Operation timed out. */
    HAL_INVALID = 0x04U  /**< Invalid parameter or state. */
} HAL_StatusTypeDef;

/* ───────────────────────── bit / register helpers ───────────────── */

/**
 * @name    Bit manipulation
 * @brief   Standard set/clr/test helpers, preferred over hand-rolled masks.
 *          The `PIC8_` prefix marks them as shared across every family.
 * @{
 */
#define PIC8_BIT(n)                  (1U << (n))
#define PIC8_BIT_SET(reg, mask)     ((reg) |=  (uint8_t)(mask))
#define PIC8_BIT_CLR(reg, mask)     ((reg) &= ~(uint8_t)(mask))
#define PIC8_BIT_TGL(reg, mask)      ((reg) ^=  (uint8_t)(mask))
#define PIC8_BIT_READ(reg, mask)     ((reg) &   (uint8_t)(mask))
/** @} */

#endif /* HAL_STATUS_H */