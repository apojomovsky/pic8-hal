/**
 * @file    example_serial_target.c
 * @brief   pic8-serial on-target demo: print a banner, then echo RX -> TX.
 *
 * @details
 *   The XC8 Makefiles build this (the host smoke test examples/example_serial.c
 *   uses the host-sim RX-injection API, which is not compiled on target).
 *   Initializes the UART, sends a banner through the ring-buffered IT TX path,
 *   then forever routes received bytes back to TX (a loopback echo). On a real
 *   target `pic8_harness_running` always returns 1, so the echo loop runs
 *   forever; connect a serial terminal at the configured baud to see the
 *   banner and have your typed characters echoed back.
 */

#include "pic8_serial.h"
#include "core/pic8_harness.h"

#ifndef FOSC_HZ
#define FOSC_HZ 20000000UL
#endif

int main(void)
{
    pic8_harness_init(0UL);
    pic8_serial_init(FOSC_HZ, 9600u);

    static const uint8_t banner[] = "pic8-serial ready\r\n";
    pic8_serial_write(banner, (int)sizeof(banner) - 1);
    pic8_serial_flush();

    uint8_t buf[8];
    for (uint32_t i = 0; pic8_harness_running(i); i++) {
        int n = pic8_serial_read(buf, (int)sizeof(buf));
        if (n > 0) {
            pic8_serial_write(buf, n);
        }
        pic8_harness_tick();
    }
    return 0;
}