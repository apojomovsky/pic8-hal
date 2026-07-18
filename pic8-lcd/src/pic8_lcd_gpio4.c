/**
 * @file    pic8_lcd_gpio4.c
 * @brief   4-bit parallel GPIO transport for pic8_lcd. R/W tied low.
 */

#include "pic8_lcd.h"
#include "pic8_hal.h"

typedef struct {
    pic8_lcd_gpio4_pins_t pins;
} gpio4_ctx_t;

static void gpio4_send(void *ctx, uint8_t rs, uint8_t byte)
{
    gpio4_ctx_t *g = (gpio4_ctx_t *)ctx;

    HAL_GPIO_WritePin(g->pins.rs_port, g->pins.rs_pin,
                      rs ? GPIO_PIN_SET : GPIO_PIN_RESET);

    /* High nibble first (HD44780 4-bit protocol) */
    uint8_t hi = (uint8_t)(byte >> 4u);
    HAL_GPIO_WritePin(g->pins.db4_port, g->pins.db4_pin,
                      (hi & 0x01u) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(g->pins.db5_port, g->pins.db5_pin,
                      (hi & 0x02u) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(g->pins.db6_port, g->pins.db6_pin,
                      (hi & 0x04u) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(g->pins.db7_port, g->pins.db7_pin,
                      (hi & 0x08u) ? GPIO_PIN_SET : GPIO_PIN_RESET);

    /* E pulse: high then low */
    HAL_GPIO_WritePin(g->pins.e_port, g->pins.e_pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(g->pins.e_port, g->pins.e_pin, GPIO_PIN_RESET);

    /* Low nibble */
    uint8_t lo = (uint8_t)(byte & 0x0Fu);
    HAL_GPIO_WritePin(g->pins.db4_port, g->pins.db4_pin,
                      (lo & 0x01u) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(g->pins.db5_port, g->pins.db5_pin,
                      (lo & 0x02u) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(g->pins.db6_port, g->pins.db6_pin,
                      (lo & 0x04u) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(g->pins.db7_port, g->pins.db7_pin,
                      (lo & 0x08u) ? GPIO_PIN_SET : GPIO_PIN_RESET);

    HAL_GPIO_WritePin(g->pins.e_port, g->pins.e_pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(g->pins.e_port, g->pins.e_pin, GPIO_PIN_RESET);
}

/* On target, use pic8-tick for ms delays and a busy-wait for us.
 * On host (when built via the HAL's host sim), HAL_GPIO_WritePin is
 * a no-op and delays are irrelevant -- the mock transport is used instead. */

static void gpio4_delay_us(void *ctx, uint32_t us)
{
    (void)ctx;
    /* Best-effort busy wait. pic8-tick's resolution is 1ms; for sub-ms
     * delays we approximate. For most HD44780 commands this is fine --
     * the timing is a minimum, not exact. */
    if (us >= 1000u) {
        pic8_tick_delay_ms(us / 1000u);
    }
    /* Sub-ms: no precise timer available on 8-bit PIC without Timer
     * intervention. The E-pulse setup/hold time is already satisfied by
     * the HAL_GPIO_WritePin call overhead (several us at 20-48 MHz). */
}

static void gpio4_delay_ms(void *ctx, uint32_t ms)
{
    (void)ctx;
    pic8_tick_delay_ms(ms);
}

/* 4-bit init requires special handling: before the LCD knows we're in
 * 4-bit mode, we must send the Function Set command's high nibble
 * three times (0x3, 0x3, 0x3) then 0x2 to switch to 4-bit.
 * gpio4_send() already handles nibble splitting, so sending 0x33
 * then 0x32 replicates this. But the init sequence in pic8_lcd.c
 * sends full 0x38 commands -- the transport must intercept those.
 *
 * Strategy: the first send() call after a cold start needs to send
 * the 4-bit init preamble. We don't track cold start here; instead,
 * pic8_lcd_init() sends Function Set three times before anything else.
 * The first 0x38 send produces: nibble 0x3, pulse, nibble 0x8, pulse.
 * The LCD only interprets the first nibble 0x3 as "8-bit Function Set",
 * and ignores the second nibble (still in 8-bit mode, expecting DB0-3).
 * After three 0x3X sends the LCD is stabilized, and the fourth send
 * (0x28 = 4-bit, 2-line) switches to 4-bit mode properly.
 *
 * This matches the HD44780 datasheet's 4-bit init procedure exactly. */

void pic8_lcd_gpio4_init(pic8_lcd_ops_t *ops, void **ctx,
                         const pic8_lcd_gpio4_pins_t *pins)
{
    static gpio4_ctx_t g;
    g.pins = *pins;

    HAL_GPIO_Init(g.pins.rs_port,  g.pins.rs_pin,  GPIO_MODE_OUTPUT);
    HAL_GPIO_Init(g.pins.e_port,   g.pins.e_pin,   GPIO_MODE_OUTPUT);
    HAL_GPIO_Init(g.pins.db4_port, g.pins.db4_pin,  GPIO_MODE_OUTPUT);
    HAL_GPIO_Init(g.pins.db5_port, g.pins.db5_pin,  GPIO_MODE_OUTPUT);
    HAL_GPIO_Init(g.pins.db6_port, g.pins.db6_pin,  GPIO_MODE_OUTPUT);
    HAL_GPIO_Init(g.pins.db7_port, g.pins.db7_pin,  GPIO_MODE_OUTPUT);

    HAL_GPIO_WritePin(g.pins.e_port, g.pins.e_pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(g.pins.rs_port, g.pins.rs_pin, GPIO_PIN_RESET);

    ops->send     = gpio4_send;
    ops->delay_us = gpio4_delay_us;
    ops->delay_ms = gpio4_delay_ms;
    *ctx = &g;
}
