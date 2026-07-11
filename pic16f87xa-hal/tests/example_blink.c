/**
 * @file    example_blink.c
 * @brief   Blink an LED on RB0, driven by Timer0.
 *
 * @details
 *   Portable across the host simulation backend and a real PIC16F87XA
 *   target compiled with XC8 — the same source file builds for both:
 *
 *     - Host simulation (CMake, PIC16F87XA_USE_SIMULATOR defined):
 *       Timer0 overflows are simulated, the LED state is observed via
 *       pic16f87xa_sim_read_output(), and main() returns 0/1 as a smoke
 *       test. Driven through the weak TIMER0_IRQHandler.
 *
 *     - Real target (XC8, PIC16F87XA_USE_SIMULATOR *not* defined):
 *       a self-contained firmware. Timer0 is configured and its overflow
 *       flag polled in the main loop; each overflow toggles RB0. No
 *       interrupt vector is used, so this example does not depend on
 *       the HAL's (still host-only) ISR dispatch.
 *
 *   Wiring assumed on a real target:
 *     - LED + resistor between RB0 and GND (active-high).
 *     - 20 MHz crystal on OSC1/OSC2 → FOSC = HS, FCY = 5 MHz.
 *   On the simulator the LED is observed via
 *   pic16f87xa_sim_read_output() (no external device drives RB0 in
 *   this example).
 */

#include "pic16f87xa.h"
#include "pic16f87xa_sfr.h"
#include "peripherals/pic16f87xa_gpio.h"
#include "peripherals/pic16f87xa_timer0.h"
#include "core/pic16f87xa_interrupt.h"

#if defined(PIC16F87XA_USE_SIMULATOR)
#include "pic16f87xa_sim.h"
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

#else  /* Real PIC target — XC8 firmware. */

#include "core/pic16f87xa_wdt_sleep.h"

int main(void)
{
    /* 1. RB0 as output, start low. */
    HAL_GPIO_Init(GPIOB, GPIO_PIN_0, GPIO_MODE_OUTPUT);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);

    /* 2. Configure Timer0: internal Fosc/4, 1:256 prescaler assigned to
     *    TMR0, reload 0. With FOSC = 20 MHz (FCY = 5 MHz) the input tick is
     *    Fosc/4/256 ≈ 19.5 kHz, so TMR0 overflows every 256 of those ≈
     *    13.1 ms. RB0 toggles on each overflow → ~38 Hz blink. */
    TIMER0_HandleTypeDef h = TIMER0_HANDLE_DEFAULT;
    h.ClockSource       = TIMER0_CLOCK_INTERNAL;
    h.Prescaler         = TIMER0_PRESCALER_1_256;
    h.PrescalerAssigned = true;
    h.ReloadValue       = 0x00U;
    h.OverflowCallback  = NULL;   /* polled — no ISR. */

    HAL_TIMER0_Init(&h);
    HAL_TIMER0_Start(&h);

    /* 3. Poll the Timer0 overflow flag and toggle RB0 each time it sets.
     *    GIE stays clear (reset default), so no interrupt is taken; the
     *    flag is polled directly through the IRQ helper. WDT is enabled
     *    in the config word, so refresh it every pass. */
    for (;;) {
        if (PIC16F87XA_IRQ_GetFlag(PIC16F87XA_IRQ_TMR0)) {
            PIC16F87XA_IRQ_ClearFlag(PIC16F87XA_IRQ_TMR0);
            HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
        }
        HAL_WDT_Refresh();
    }
}

#endif /* PIC16F87XA_USE_SIMULATOR */