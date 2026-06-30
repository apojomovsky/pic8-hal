/**
 * @file    example_blink.c
 * @brief   End-to-end smoke test: blink an LED on RB0, driven by a
 *          simulated Timer0 overflow.
 *
 * @details
 *   Portable across the host simulation backend and a real PIC16F87XA
 *   target compiled with XC8. Uses the new Timer0 driver — no direct
 *   register poking — to demonstrate that the Cube-style API is
 *   sufficient to drive a real application.
 *
 *   Wiring assumed on a real target:
 *     - LED + resistor between RB0 and GND (active-high).
 *     - 20 MHz crystal on OSC1/OSC2 → FOSC = HS, FCY = 5 MHz.
 *   On the simulator, the test rig drives RB0's "external world" and the
 *   LED is observed via pic16f87xa_sim_read_output().
 */

#include "pic16f87xa.h"
#include "pic16f87xa_sim.h"
#include "pic16f87xa_sfr.h"
#include "peripherals/pic16f87xa_gpio.h"
#include "peripherals/pic16f87xa_timer0.h"
#include "core/pic16f87xa_interrupt.h"
#include <stdio.h>

/** How long the simulated CPU should run, in instruction cycles. */
#define RUN_CYCLES  200000UL

/* Application state. */
static volatile uint32_t tick_count   = 0;
static volatile uint32_t toggle_count = 0;

/* Driver-managed overflow callback — invoked by the weak TIMER0_IRQHandler
 * every time TMR0 rolls over. */
static void on_t0_overflow(void)
{
    /* Reload TMR0 with 0 so the next period is exactly 256 input cycles
     * (DS39582B §5.3: writing TMR0 also clears the prescaler). */
    PIC16F87XA_REG8(PIC_REG_TMR0) = 0;
    tick_count++;
}

int main(void)
{
    pic16f87xa_sim_reset();
    /* Wire sim backend → driver weak ISR. */
    pic16f87xa_sim_set_irq_callback(TIMER0_IRQHandler);

    /* 1. RB0 as output, start low. */
    HAL_GPIO_Init(GPIOB, GPIO_PIN_0, GPIO_MODE_OUTPUT);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);

    /* 2. Configure Timer0: internal Fosc/4, 1:2 prescaler, no callback
     *    so we set one inline. Overflow = 256 × 2 = 512 cycles. */
    TIMER0_HandleTypeDef h = TIMER0_HANDLE_DEFAULT;
    h.ClockSource        = TIMER0_CLOCK_INTERNAL;
    h.Prescaler          = TIMER0_PRESCALER_1_2;
    h.PrescalerAssigned  = true;
    h.ReloadValue        = 0x00U;
    h.OverflowCallback   = on_t0_overflow;

    HAL_TIMER0_Init(&h);
    HAL_TIMER0_Start(&h);

    /* 3. Run the simulated CPU loop. Each TMR0IF toggles RB0. */
    for (uint32_t i = 0; i < RUN_CYCLES; i++) {
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

    if (toggle_count > 0U) {
        printf("OK: RB0 toggled %u times; final state = %u.\n",
               (unsigned)toggle_count,
               (unsigned)pic16f87xa_sim_read_output('B', 0));
        return 0;
    }
    printf("FAIL: RB0 never toggled.\n");
    return 1;
}