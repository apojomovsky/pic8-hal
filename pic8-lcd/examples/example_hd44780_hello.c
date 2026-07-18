/**
 * @file    example_lcd_hello.c
 * @brief   "Hello, World!" on both lines of a 16x2 LCD, using 4-bit GPIO.
 *          Real-target only (depends on HAL). Adapt pins to your board.
 */

#include "pic8_lcd.h"
#include "pic8_lcd_transport.h"
#include "pic8_hal.h"
#include "pic8_tick.h"

int main(void)
{
    pic8_tick_init();

    pic8_lcd_ops_t ops;
    void *ops_ctx;

    pic8_lcd_gpio4_pins_t pins = {
        .rs_port = GPIOA, .rs_pin  = GPIO_PIN_0,
        .e_port  = GPIOA, .e_pin   = GPIO_PIN_1,
        .db4_port = GPIOA, .db4_pin = GPIO_PIN_4,
        .db5_port = GPIOA, .db5_pin = GPIO_PIN_5,
        .db6_port = GPIOA, .db6_pin = GPIO_PIN_6,
        .db7_port = GPIOA, .db7_pin = GPIO_PIN_7,
    };
    pic8_lcd_gpio4_init(&ops, &ops_ctx, &pins);

    pic8_lcd_t lcd;
    pic8_lcd_config_t cfg = { .cols = 16, .rows = 2, .row_addr = {0} };
    pic8_lcd_init(&lcd, &ops, ops_ctx, &cfg);

    pic8_lcd_set_cursor(&lcd, 0, 0);
    pic8_lcd_print(&lcd, "Hello, World!");

    pic8_lcd_set_cursor(&lcd, 0, 1);
    pic8_lcd_print(&lcd, "pic8-lcd ready");

    pic8_lcd_cursor_blink(&lcd, true);

    for (;;) {
    }
    return 0;
}
