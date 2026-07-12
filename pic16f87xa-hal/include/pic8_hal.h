/**
 * @file    pic8_hal.h
 * @brief   Family-neutral top-level entry point to the PIC16F87XA HAL.
 *
 * @details
 *   A consumer that should build unchanged against any 8-bit PIC family
 *   (the cooperative task manager is the canonical example) includes this
 *   single neutral header instead of the family-specific `pic16f87xa.h`.
 *   Each family provides its own `pic8_hal.h` under the same neutral name,
 *   so the build's include path (which family's HAL tree is added) decides
 *   which family's headers are pulled in. This is the family-agnostic
 *   contract entry point introduced for the Phase 3 litmus test.
 */

#ifndef PIC8_HAL_H
#define PIC8_HAL_H

#include "pic16f87xa.h"       /* standard types, status codes, platform   */
#include "pic16f87xa_sfr.h"   /* SFR address map + bit definitions       */

/* Core. */
#include "core/pic16_irq.h"
#include "core/pic16f87xa_wdt_sleep.h"

/* Peripherals. */
#include "peripherals/pic16f87xa_gpio.h"
#include "peripherals/pic16f87xa_timer0.h"
#include "peripherals/pic16f87xa_timer1.h"
#include "peripherals/pic16f87xa_timer2.h"
#include "peripherals/pic16f87xa_ccp.h"
#include "peripherals/pic16f87xa_usart.h"
#include "peripherals/pic16f87xa_ssp.h"
#include "peripherals/pic16f87xa_adc.h"
#include "peripherals/pic16f87xa_comp.h"
#include "peripherals/pic16f87xa_vref.h"
#include "peripherals/pic16f87xa_eeprom.h"
#if PIC16F87XA_FAMILY_HAS_PSP
#include "peripherals/pic16f87xa_psp.h"
#endif

#endif /* PIC8_HAL_H */