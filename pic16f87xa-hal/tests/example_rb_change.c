/**
 * @file    example_rb_change.c
 * @brief   Smoke test for the RB<7:4> change-interrupt hook
 *          (HAL_GPIO_RegisterChangeCallback / RB_IRQHandler) on the sim.
 *
 * @details
 *   Covers the Phase 0a cases from docs/pic8-encoder-plan.md, run in the
 *   HAL's own example suite (the repo's HAL "tests" are example_*.c
 *   executables that return 0 on pass / non-zero on fail):
 *     1. RB_IRQHandler is a no-op when RBIF is not pending (callback not
 *        invoked, flag untouched).
 *     2. With RBIF pending, the registered callback fires exactly once,
 *        receives the freshly-read PORTB byte, and RBIF is clear after.
 *     3. A NULL (or unregistered) callback does not crash.
 *     4. pic8_dispatch_all_irqs reaches RB_IRQHandler (a full dispatch pass
 *        with RBIF pending still drives the callback, proving the fan-out).
 *
 *   RBIF modeling: the host sim does not assert RBIF on a PORTB mismatch
 *   (modeling the datasheet mismatch-comparator "snapshot on every PORTB
 *   read" behavior faithfully would require intercepting every CPU read of
 *   PORTB through the PIC8_REG8 macro, which is invasive for the one feature
 *   that needs it). The documented fallback (plan §"host sim does not model
 *   RBIF yet") is used here: the test asserts RBIF directly in INTCON, then
 *   calls the handler and checks the callback's observed argument and the
 *   post-call flag state. This proves the handler's own read/clear/callback
 *   ordering, which is the part that actually matters.
 *
 * Build (host):
 *   cc -std=c99 -DPIC16F877A -Iinclude/host -Iinclude \
 *      tests/example_rb_change.c -L... -lpic16f87xa_hal -o example_rb_change
 *   (or via the CMake build, which links the HAL library for this example.)
 */

#include "pic16f87xa.h"
#include "pic16f87xa_sim.h"
#include "pic16f87xa_sfr.h"
#include "peripherals/pic16f87xa_gpio.h"
#include "core/pic16_irq.h"
#include "core/pic8_harness.h"

#include <stdio.h>

static int g_pass = 0, g_fail = 0;
#define CHECK(c, m) do { if (c) { g_pass++; } else { printf("FAIL: %s\n", m); g_fail++; } } while (0)

/* Observed callback state. */
static volatile int    g_cb_calls;
static volatile uint8_t g_cb_last;

static void on_rb_change(uint8_t portb_value)
{
    g_cb_calls++;
    g_cb_last = portb_value;
}

static void reset_observed(void)
{
    g_cb_calls = 0;
    g_cb_last  = 0xFFU;   /* sentinel, no real PORTB read yields 0xFF */
}

/* Assert RBIF directly (the documented test-only fallback). */
static void assert_rbif(void)
{
    PIC8_REG8(PIC_REG_INTCON) |= PIC_INTCON_RBIF;
}

static uint8_t rbif_pending(void)
{
    return (PIC8_REG8(PIC_REG_INTCON) & PIC_INTCON_RBIF) ? 1U : 0U;
}

static void test_noop_when_not_pending(void)
{
    reset_observed();
    HAL_GPIO_RegisterChangeCallback(on_rb_change);

    /* Ensure RBIF is clear, PORTB is some known value. */
    HAL_IRQ_ClearFlag(PIC16_IRQ_RB);
    PIC8_REG8(PIC_REG_PORTB) = 0xA5U;

    RB_IRQHandler();   /* RBIF not pending: must do nothing. */

    CHECK(g_cb_calls == 0, "noop: callback not invoked when RBIF clear");
    CHECK(rbif_pending() == 0, "noop: flag still clear");
    CHECK(g_cb_last == 0xFFU, "noop: callback arg untouched");
}

static void test_fires_when_pending(void)
{
    reset_observed();
    HAL_GPIO_RegisterChangeCallback(on_rb_change);

    HAL_IRQ_ClearFlag(PIC16_IRQ_RB);
    PIC8_REG8(PIC_REG_PORTB) = 0x3CU;   /* the byte the handler must read */
    assert_rbif();
    CHECK(rbif_pending() == 1, "fires: RBIF set before handler");

    RB_IRQHandler();

    CHECK(g_cb_calls == 1, "fires: callback invoked exactly once");
    CHECK(g_cb_last == 0x3CU, "fires: callback received the PORTB byte");
    CHECK(rbif_pending() == 0, "fires: RBIF cleared by handler");
}

static void test_null_callback_safe(void)
{
    reset_observed();
    HAL_GPIO_RegisterChangeCallback(NULL);

    HAL_IRQ_ClearFlag(PIC16_IRQ_RB);
    PIC8_REG8(PIC_REG_PORTB) = 0x00U;
    assert_rbif();

    RB_IRQHandler();   /* must not crash, must still clear the flag */

    CHECK(g_cb_calls == 0, "null: no callback invocation");
    CHECK(rbif_pending() == 0, "null: flag still cleared");
}

static void test_dispatch_reaches_handler(void)
{
    reset_observed();
    HAL_GPIO_RegisterChangeCallback(on_rb_change);

    HAL_IRQ_ClearFlag(PIC16_IRQ_RB);
    PIC8_REG8(PIC_REG_PORTB) = 0x96U;
    assert_rbif();

    /* A full dispatch pass must route to RB_IRQHandler. */
    pic8_dispatch_all_irqs();

    CHECK(g_cb_calls == 1, "dispatch: fan-out reached RB_IRQHandler");
    CHECK(g_cb_last == 0x96U, "dispatch: correct byte delivered");
    CHECK(rbif_pending() == 0, "dispatch: RBIF cleared");
}

int main(void)
{
    pic16f87xa_sim_reset();

    test_noop_when_not_pending();
    test_fires_when_pending();
    test_null_callback_safe();
    test_dispatch_reaches_handler();

    printf("example_rb_change: %d passed, %d failed\n", g_pass, g_fail);
    return (g_fail == 0) ? 0 : 1;
}
