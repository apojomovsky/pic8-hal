/**
 * @file    peripherals/hal_gpio.h
 * @brief   Family-neutral GPIO contract (`HAL_GPIO_*` + RB-change hook).
 *
 * @details
 *   Family-agnostic consumers include this neutral name instead of the
 *   family-specific `pic18fxx5x_gpio.h`. Each family provides its own
 *   `peripherals/hal_gpio.h` that pulls in its family-specific GPIO header
 *   (same `GPIO_TypeDef` / `HAL_GPIO_*` API, family-shaped bodies). The
 *   build's include path selects which family's copy resolves.
 *
 *   The surface this neutral header exists for, beyond the always-portable
 *   `HAL_GPIO_Init/Read/Write/...`, is the RB<7:4> change-interrupt hook
 *   (@ref HAL_GPIO_RegisterChangeCallback / @ref RB_IRQHandler) that
 *   `pic8-encoder` builds on: the same names/signatures on both families,
 *   different register-level bodies.
 */

#ifndef HAL_GPIO_H
#define HAL_GPIO_H
#include "pic18fxx5x_gpio.h"
#endif /* HAL_GPIO_H */