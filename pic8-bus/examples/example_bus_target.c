/**
 * @file    example_bus_target.c
 * @brief   pic8-bus on-target link smoke: initialize the I2C and SPI buses.
 *
 * @details
 *   The XC8 Makefiles build this (the host test examples/example_bus.c uses
 *   injected mock ops and needs no hardware). This demo configures the MSSP
 *   as an I2C master (100 kHz) and an SPI master with a GPIO chip-select,
 *   proving the pic8-bus module compiles and links against the real HAL on
 *   target. It deliberately does NOT issue a MEM transaction -- the default
 *   ops' SSPIF wait would block without a device attached. A real firmware
 *   image calls `pic8_bus_i2c_mem_read`/`write` against an attached sensor
 *   after this init.
 */

#include "pic8_bus.h"
#include "core/pic8_harness.h"

#ifndef FOSC_HZ
#define FOSC_HZ 20000000UL
#endif

int main(void)
{
    pic8_harness_init(0UL);
    pic8_bus_i2c_init(FOSC_HZ, 100000UL);        /* I2C master, 100 kHz */
    pic8_bus_spi_init(FOSC_HZ, 0UL, 1u, 0u);     /* SPI master, CS = GPIOB pin 0 */

    for (uint32_t i = 0; pic8_harness_running(i); i++) {
        pic8_harness_tick();
    }
    return 0;
}