/**
 * @file    pic8_hal.h
 * @brief   Family-neutral top-level entry point to the PIC18F2455 HAL.
 *
 * @details
 *   A consumer that should build unchanged against any 8-bit PIC family
 *   (the cooperative task manager is the canonical example) includes this
 *   single neutral header instead of the family-specific `pic18fxx5x.h`.
 *   Each family provides its own `pic8_hal.h` under the same neutral name,
 *   so the build's include path (which family's HAL tree is added) decides
 *   which family's headers are pulled in. This is the family-agnostic
 *   contract entry point introduced for the Phase 3 litmus test.
 *
 *   Phase 2 peripheral coverage: GPIO and Timer0. Phase 4 adds the rest
 *   (Timer1/2/3, ECCP, MSSP, ADC, USART, EEPROM); this header grows then.
 */

#ifndef PIC8_HAL_H
#define PIC8_HAL_H

#include "pic18fxx5x.h"       /* standard types, status codes, platform   */
#include "pic18fxx5x_sfr.h"   /* SFR address map + bit definitions       */

/* Core. */
#include "core/pic18_irq.h"
#include "core/pic18fxx5x_wdt_sleep.h"

/* Peripherals (Phase 2 MVP + Phase 4 Timer1/Timer2). */
#include "peripherals/pic18fxx5x_gpio.h"
#include "peripherals/pic18fxx5x_timer0.h"
#include "peripherals/pic18fxx5x_timer1.h"
#include "peripherals/pic18fxx5x_timer2.h"
#include "peripherals/pic18fxx5x_timer3.h"
#include "peripherals/pic18fxx5x_ccp.h"
#include "peripherals/pic18fxx5x_ssp.h"
#include "peripherals/pic18fxx5x_usart.h"
#include "peripherals/pic18fxx5x_comp.h"
#include "peripherals/pic18fxx5x_eeprom.h"
#include "peripherals/pic18fxx5x_adc.h"

#endif /* PIC8_HAL_H */