/**
 * @file    example_idle_blink.c
 * @brief   Blink an LED on RB0 with the CPU asleep — fully peripheral
 *          driven, so the CPU is mostly idle.
 *
 * @details
 *   The timebase is a hardware timer, not a software delay loop. The
 *   CPU spends almost all of its time in Power-down (Sleep); a Timer1
 *   overflow raises an interrupt, the ISR toggles the LED, and the CPU
 *   goes back to sleep. The CPU is awake for only a handful of cycles
 *   per overflow.
 *
 *   One source builds for both the host simulation backend and a real
 *   XC8 target. The two differ in exactly two ways, both inline below:
 *
 *     - Timebase. On a real target: Timer1 on the 32.768 kHz watch
 *       crystal (T1OSC) on T1OSO/T1OSI, asynchronous — the only Timer1
 *       mode that keeps counting in Sleep (DS39582B §6.5) — 1:1, reload
 *       0x8000 (32768) → 1 s overflow. The host sim does not model the
 *       T1OSC crystal (external-clock Timer1 is skipped), so it drives
 *       Timer1 off the internal Fosc/4 clock with a 1:8 prescaler to
 *       exercise the same ISR → callback → GPIO path.
 *
 *     - Idle loop. On the target the CPU sleeps and Timer1 wakes it; the
 *       loop never exits. On the host Sleep is a no-op, so each loop
 *       iteration steps the simulated CPU and the loop exits after a
 *       bounded run with a pass/fail report.
 *
 *   Real-target wiring: LED + resistor on RB0; a 32.768 kHz crystal +
 *   ~22 pF caps on T1OSO/T1OSI (PORTC<0>/<1>); the 20 MHz HS crystal on
 *   OSC1/OSC2 is still required for the CPU clock. With the default
 *   config (WDTE=ON) the ISR refreshes the WDT each wake; set WDTE=OFF
 *   if you'd rather not manage it.
 */

#include "pic16f87xa.h"
#include "pic16f87xa_sfr.h"
#include "peripherals/pic16f87xa_gpio.h"
#include "peripherals/pic16f87xa_timer1.h"
#include "core/pic16f87xa_interrupt.h"
#include "core/pic16f87xa_wdt_sleep.h"

#if defined(PIC16F87XA_USE_SIMULATOR)
#include "pic16f87xa_sim.h"
#include <stdio.h>
#endif

/**
 * @brief  Timer1 reload value — 0x8000 (32768). On the T1OSC target
 *         timebase that is exactly 1 s of the 32.768 kHz crystal; the
 *         ISR writes it back after each overflow to re-arm the 1 s
 *         period. On the host the same reload halves the simulated
 *         work per overflow.
 */
#define T1_RELOAD  0x8000U

#if defined(PIC16F87XA_USE_SIMULATOR)
/** Simulated run length in instruction cycles. Enough for several
 *  1:8-prescaler overflows at the 0x8000 reload (8 × 32768 = 262144
 *  cycles each). */
#define SIM_RUN_CYCLES  2000000UL
#endif

/* Toggle count — the only writer is the ISR/callback. */
static volatile uint32_t g_toggle_count = 0;

/**
 * @brief  Timer1 overflow callback — runs in interrupt context on the
 *         target and in the sim callback on the host. Re-arms the
 *         period, toggles the LED, and (on target) keeps the WDT alive
 *         across Sleep.
 */
static void on_t1_overflow(void)
{
    HAL_TIMER1_WriteCounter(T1_RELOAD);   /* re-arm the 1 s period */
    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
#if !defined(PIC16F87XA_USE_SIMULATOR)
    HAL_WDT_Refresh();                    /* WDT runs in Sleep; refresh on wake */
#endif
    g_toggle_count++;
}

int main(void)
{
#if defined(PIC16F87XA_USE_SIMULATOR)
    uint32_t cycles = 0;
    pic16f87xa_sim_reset();
    /* The sim dispatches every IRQ through this single callback; the
     * driver handler checks the TMR1 flag and forwards to our callback. */
    pic16f87xa_sim_set_irq_callback(TIMER1_IRQHandler);
#endif

    /* 1. RB0 as output, start low. */
    HAL_GPIO_Init(GPIOB, GPIO_PIN_0, GPIO_MODE_OUTPUT);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);

    /* 2. Configure the Timer1 timebase. The handle is the same for both
     *    builds except for the clock source and prescaler (see the file
     *    header for why). Reload 0x8000 + a 1 s/half-period overflow. */
    TIMER1_HandleTypeDef h = TIMER1_HANDLE_DEFAULT;
    h.ReloadValue      = T1_RELOAD;
    h.OverflowCallback = on_t1_overflow;
#if defined(PIC16F87XA_USE_SIMULATOR)
    h.ClockSource      = TIMER1_CLOCK_INTERNAL;
    h.Prescaler        = TIMER1_PRESCALER_1_8;
#else
    h.ClockSource      = TIMER1_CLOCK_EXTERNAL;
    h.ClockSync        = TIMER1_ASYNC_EXTERNAL;
    h.Oscillator       = TIMER1_OSCILLATOR_ON;
    h.Prescaler        = TIMER1_PRESCALER_1_1;
#endif
    HAL_TIMER1_Init(&h);
    HAL_TIMER1_Start(&h);

#if !defined(PIC16F87XA_USE_SIMULATOR)
    /* 3. Wait for the T1OSC crystal to start ticking before sleeping (a
     *    32 kHz crystal can take a few hundred ms), refreshing the WDT
     *    meanwhile; the loop exits once the counter has advanced past the
     *    reload, proving the crystal is running. */
    while (HAL_TIMER1_ReadCounter() <= T1_RELOAD) {
        HAL_WDT_Refresh();
    }

    /* 4. Enable global interrupts (HAL_TIMER1_Init already set TMR1IE
     *    and PEIE). */
    PIC16F87XA_IRQ_Restore(1);   /* GIE = 1. */
#endif

    /* 5. Idle loop — the heart of "CPU mostly idle".
     *    On the target this is just an infinite sleep: each overflow
     *    wakes the CPU, the vector dispatcher calls TIMER1_IRQHandler,
     *    which clears the flag and runs on_t1_overflow (reload + toggle
     *    + WDT refresh), and the CPU sleeps again. The sim-only block
     *    is compiled out, so the loop never exits on real silicon.
     *
     *    On the host, Sleep is a no-op, so each iteration steps the
     *    simulated CPU; after SIM_RUN_CYCLES the block reports whether
     *    the LED toggled and returns. */
    for (;;) {
#if defined(PIC16F87XA_USE_SIMULATOR)
        if (++cycles >= SIM_RUN_CYCLES) {
            if (g_toggle_count >= 2U) {
                printf("OK: RB0 toggled %u times; CPU idle between overflows.\n",
                       (unsigned)g_toggle_count);
                return 0;
            }
            printf("FAIL: RB0 only toggled %u times.\n", (unsigned)g_toggle_count);
            return 1;
        }
        pic16f87xa_sim_step(1);
#endif
        HAL_Sleep_Enter();
    }
}