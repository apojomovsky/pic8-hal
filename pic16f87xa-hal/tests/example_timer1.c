/**
 * @file    example_timer1.c
 * @brief   End-to-end smoke test for the Timer1 driver on the sim backend.
 *
 *   Timer1 is a 16-bit counter clocked from Fosc/4 with a 1:1 prescaler
 *   in this test. Expected behaviour: TMR1IF fires every 65 536 instruction
 *   cycles; the test counts the number of overflows.
 *
 * Build:
 *   cc -std=c99 -DPIC16F877A -Iinclude/host -Iinclude \
 *      tests/example_timer1.c \
 *      src/peripherals/pic16f87xa_timer1.c \
 *      src/core/pic16f87xa_interrupt.c \
 *      src/sim/pic16f87xa_sim.c \
 *      -o example_timer1
 */

#include "pic16f87xa.h"
#include "pic16f87xa_sim.h"
#include "pic16f87xa_sfr.h"
#include "peripherals/pic16f87xa_timer1.h"
#include "core/pic16f87xa_interrupt.h"
#include <stdio.h>

/** Total overflows we expect. */
#define EXPECTED_OVERFLOWS  3U
/** Cycles between overflows at 1:1 prescaler = 0x10000 = 65536. */
#define OVERFLOW_CYCLES     65536UL
/** Safety margin for the simulator loop. */
#define SIM_BUDGET          ((OVERFLOW_CYCLES * EXPECTED_OVERFLOWS) + 1024UL)

static volatile uint32_t overflows = 0;

static void on_t1_overflow(void)
{
    overflows++;
}

int main(void)
{
    pic16f87xa_sim_reset();
    pic16f87xa_sim_set_irq_callback(TIMER1_IRQHandler);

    TIMER1_HandleTypeDef h = TIMER1_HANDLE_DEFAULT;
    h.Prescaler        = TIMER1_PRESCALER_1_1;
    h.ClockSource      = TIMER1_CLOCK_INTERNAL;
    h.ReloadValue      = 0x0000U;
    h.OverflowCallback = on_t1_overflow;

    PIC16F87XA_StatusTypeDef st = HAL_TIMER1_Init(&h);
    if (st != PIC16F87XA_OK) { printf("FAIL: Init returned %u\n", (unsigned)st); return 1; }
    HAL_TIMER1_Start(&h);

    for (uint32_t i = 0; i < SIM_BUDGET; i++) {
        pic16f87xa_sim_step(1);
        if (overflows >= EXPECTED_OVERFLOWS) break;
    }

    if (overflows >= EXPECTED_OVERFLOWS) {
        printf("OK: Timer1 produced %u overflows (expected >= %u)\n",
               (unsigned)overflows, (unsigned)EXPECTED_OVERFLOWS);
        return 0;
    }
    printf("FAIL: Timer1 produced only %u overflows (expected %u)\n",
           (unsigned)overflows, (unsigned)EXPECTED_OVERFLOWS);
    return 1;
}