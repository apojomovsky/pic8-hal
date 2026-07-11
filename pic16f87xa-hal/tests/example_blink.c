/**
 * @file    example_blink.c
 * @brief   Blink an LED on RB0 from a Timer0 overflow — the canonical
 *          "the HAL drives a real application" smoke test.
 *
 * @details
 *   One source builds for both the host simulation backend and a real
 *   XC8 target, with one main() and one algorithm: poll the Timer0
 *   overflow flag and toggle RB0 each time it sets. No software delay
 *   loop — the timebase is the hardware timer.
 *
 *   The only mode-specific bits are inline guards (see the file header
 *   of example_idle_blink.c for the full rationale): on the host, time
 *   advances only when pic16f87xa_sim_step() is pumped and the test has
 *   to terminate with a pass/fail report; on the target, hardware
 *   advances on its own and main() never returns.
 *
 *   Wiring (real target): LED + resistor between RB0 and GND
 *   (active-high); 20 MHz crystal on OSC1/OSC2 → FOSC = HS, FCY = 5 MHz.
 *   Timer0 internal Fosc/4, 1:256 prescaler, reload 0 → overflow every
 *   256 × 256 × 0.2 µs ≈ 13 ms, so RB0 toggles ~76×/s (a fast blink).
 *   The WDT (enabled in the config word) is refreshed in the poll loop.
 */

#include "pic16f87xa.h"
#include "pic16f87xa_sfr.h"
#include "peripherals/pic16f87xa_gpio.h"
#include "peripherals/pic16f87xa_timer0.h"
#include "core/pic16f87xa_interrupt.h"
#include "core/pic16f87xa_wdt_sleep.h"

#if defined(PIC16F87XA_USE_SIMULATOR)
#include "pic16f87xa_sim.h"
#include <stdio.h>
/** Simulated run length in instruction cycles. Enough for several
 *  1:256-prescaler overflows (256 × 256 = 65536 cycles each). */
#define SIM_RUN_CYCLES  600000UL
#endif

/* Toggle count — the poll loop is the only writer. */
static volatile uint32_t g_toggle_count = 0;

int main(void)
{
#if defined(PIC16F87XA_USE_SIMULATOR)
    uint32_t cycles = 0;
    pic16f87xa_sim_reset();
#endif

    /* 1. RB0 as output, start low. */
    HAL_GPIO_Init(GPIOB, GPIO_PIN_0, GPIO_MODE_OUTPUT);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);

    /* 2. Timer0: internal Fosc/4, 1:256 prescaler, reload 0. Polled —
     *    no overflow callback, no interrupt. */
    TIMER0_HandleTypeDef h = TIMER0_HANDLE_DEFAULT;
    h.ClockSource       = TIMER0_CLOCK_INTERNAL;
    h.Prescaler         = TIMER0_PRESCALER_1_256;
    h.PrescalerAssigned = true;
    h.ReloadValue       = 0x00U;
    h.OverflowCallback  = NULL;
    HAL_TIMER0_Init(&h);
    HAL_TIMER0_Start(&h);

    /* 3. Poll the Timer0 overflow flag and toggle RB0 each time it sets.
     *    On the target this loop never returns (firmware); on the host the
     *    sim-only block steps the simulated CPU and exits after a bounded
     *    run with a pass/fail report. HAL_WDT_Refresh is a no-op on the
     *    host, so it is called unconditionally. */
    for (;;) {
#if defined(PIC16F87XA_USE_SIMULATOR)
        if (++cycles >= SIM_RUN_CYCLES) {
            if (g_toggle_count >= 2U) {
                printf("OK: RB0 toggled %u times.\n",
                       (unsigned)g_toggle_count);
                return 0;
            }
            printf("FAIL: RB0 only toggled %u times.\n",
                   (unsigned)g_toggle_count);
            return 1;
        }
        pic16f87xa_sim_step(1);
#endif
        if (PIC16F87XA_IRQ_GetFlag(PIC16F87XA_IRQ_TMR0)) {
            PIC16F87XA_IRQ_ClearFlag(PIC16F87XA_IRQ_TMR0);
            HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
            g_toggle_count++;
        }
        HAL_WDT_Refresh();
    }
}