/**
 * @file    example_blink.c
 * @brief   End-to-end smoke test: blink an LED on RB0, driven by a
 *          simulated Timer0 overflow.
 *
 * @details
 *   This program is portable across:
 *     - the simulation backend (compiled with -DPIC16F87XA_USE_SIMULATOR),
 *     - a real PIC16F87XA target compiled with XC8.
 *
 *   Wiring assumed on a real target:
 *     - LED + resistor between RB0 and GND (active-high).
 *     - 20 MHz crystal on OSC1/OSC2 → FOSC = HS, FCY = 5 MHz.
 *   On the simulator, the test rig drives RB0's "external world" and the
 *   LED is observed via pic16f87xa_sim_read_output().
 *
 * Build (host):
 *   cc -std=c99 -Wall -DPIC16F87XA_USE_SIMULATOR \
 *      -I include \
 *      tests/example_blink.c \
 *      src/peripherals/pic16f87xa_gpio.c \
 *      src/core/pic16f87xa_interrupt.c \
 *      src/sim/pic16f87xa_sim.c \
 *      -o example_blink
 *
 * Run:
 *   ./example_blink   # prints "RB0=0 ... RB0=1 ... RB0=0 ..."
 */

#include "pic16f87xa.h"
#include "pic16f87xa_sim.h"
#include "pic16f87xa_sfr.h"
#include "peripherals/pic16f87xa_gpio.h"
#include "core/pic16f87xa_interrupt.h"
#include <stdio.h>

/** Half-period measured in simulated instruction cycles. */
#define BLINK_HALF_PERIOD_CYCLES  200000UL

/* Application state. */
static volatile uint32_t tick_count = 0;
static volatile uint32_t toggle_count = 0;

/**
 * @brief  IRQ handler — called whenever a simulated interrupt fires.
 *         Implementation matches what an XC8 ISR() function would do.
 */
void pic16f87xa_on_irq(void)
{
    if (PIC16F87XA_IRQ_GetFlag(PIC16F87XA_IRQ_TMR0)) {
        PIC16F87XA_IRQ_ClearFlag(PIC16F87XA_IRQ_TMR0);
        PIC16F87XA_REG8(PIC_REG_TMR0) = 0;   /* reload. */
        tick_count++;
    }
}

int main(void)
{
    pic16f87xa_sim_reset();
    pic16f87xa_sim_set_irq_callback(pic16f87xa_on_irq);

    /* 1. Configure RB0 as output. */
    HAL_GPIO_Init(GPIOB, GPIO_PIN_0, GPIO_MODE_OUTPUT);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);

    /* 2. Configure Timer0: internal clock, no prescaler → 1:1 (simplest).
     *    OPTION_REG<PSA> = 0 (prescaler to Timer0), <PS2:PS0> = 000 (1:2).
     *    Real applications would set PS2:PS0 to something non-zero for
     *    longer periods; we keep it tiny so the test finishes quickly. */
    uint8_t opt = PIC16F87XA_REG8(PIC_REG_OPTION);
    opt &= (uint8_t)~0x07U;    /* PS2:PS0 = 000 → prescaler 1:2 */
    opt &= (uint8_t)~0x08U;    /* PSA  = 0   → prescaler to Timer0 */
    PIC16F87XA_REG8(PIC_REG_OPTION) = opt;

    /* 3. Enable Timer0 overflow interrupt. */
    PIC16F87XA_IRQ_Enable(PIC16F87XA_IRQ_TMR0);
    PIC16F87XA_BIT_SET(PIC16F87XA_REG8(PIC_REG_INTCON), PIC_INTCON_GIE);

    /* 4. Run the simulated CPU loop. Each TMR0IF toggle flips RB0. */
    for (uint32_t i = 0; i < BLINK_HALF_PERIOD_CYCLES; i++) {
        pic16f87xa_sim_step(1);
        if (tick_count >= 1U) {
            HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
            tick_count = 0;
            toggle_count++;
            printf("RB0 = %u after %u cycles\n",
                   (unsigned)pic16f87xa_sim_read_output('B', 0),
                   (unsigned)(i + 1));
        }
    }

    /* 5. Confirm the LED was driven high at some point during the run. */
    if (toggle_count > 0U) {
        printf("OK: RB0 toggled %u times; final state = %u.\n",
               (unsigned)toggle_count,
               (unsigned)pic16f87xa_sim_read_output('B', 0));
        return 0;
    }
    printf("FAIL: RB0 never toggled.\n");
    return 1;
}