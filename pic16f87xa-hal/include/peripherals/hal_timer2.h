/**
 * @file    peripherals/hal_timer2.h
 * @brief   Family-neutral Timer2 driver contract (`HAL_TIMER2_*`).
 *
 * @details
 *   Family-agnostic consumers include this neutral name instead of the
 *   family-specific `pic16f87xa_timer2.h`. Each family provides its own
 *   `peripherals/hal_timer2.h` that pulls in its family-specific Timer2
 *   header (same `TIMER2_HandleTypeDef` / `HAL_TIMER2_*` API, family-shaped
 *   bodies). The build's include path selects which family's copy resolves.
 */

#ifndef HAL_TIMER2_H
#define HAL_TIMER2_H
#include "pic16f87xa_timer2.h"
#endif /* HAL_TIMER2_H */