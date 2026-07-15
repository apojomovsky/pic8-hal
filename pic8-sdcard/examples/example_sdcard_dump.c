/**
 * @file    example_sdcard_dump.c
 * @brief   Read block 0 and print its first 16 bytes over pic8-serial --
 *          the Phase 3 real-hardware smoke test (see
 *          pic8-sdcard/docs/pic8-sdcard-plan.md). Real-target only: unlike
 *          pic8-usb, this module's host tests exercise mmc.c/crc.c
 *          directly against a mock (see tests/test_pic8_sdcard.c), not
 *          this wrapper -- pic8_sdcard.c itself depends on pic8_hal.h/
 *          pic8-tick, which need a real (or host-simulated, but
 *          meaninglessly so without a real SPI slave behind it) HAL
 *          family, so there's no useful host build of this example.
 */

#include "pic8_sdcard.h"
#include "pic8_serial.h"

#include <stdio.h>

int main(void)
{
    pic8_serial_init(48000000UL, 9600UL);

    pic8_sdcard_pins_t pins = { .cs_port = GPIOC, .cs_pin = 6 };
    if (!pic8_sdcard_init(&pins, 48000000UL)) {
        printf("pic8-sdcard: init failed\r\n");
        for (;;) {
        }
    }

    printf("pic8-sdcard: %lu blocks\r\n", (unsigned long)pic8_sdcard_num_blocks());

    static uint8_t block[512];
    if (!pic8_sdcard_read_block(0, block)) {
        printf("pic8-sdcard: read_block(0) failed\r\n");
        for (;;) {
        }
    }

    printf("block 0, first 16 bytes:");
    for (int i = 0; i < 16; i++) {
        printf(" %02x", block[i]);
    }
    printf("\r\n");

    for (;;) {
    }
    return 0;
}
