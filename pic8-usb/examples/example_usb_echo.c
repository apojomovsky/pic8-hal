/**
 * @file    example_usb_echo.c
 * @brief   Minimal echo: read a byte from the host, write it back.
 *
 * @details
 *   Only calls pic8_usb.h's public API, so this same source builds against
 *   either backend: pic8_usb_hoststub (this CMakeLists.txt, a bounded-loop
 *   host smoke run) or the real target's pic8_usb.c (Phase 3's
 *   mcu/pic18fxx5x-usb-mplabx/Makefile, not written yet -- see
 *   pic8-usb/docs/pic8-usb-plan.md).
 *
 *   On host, PIC8_USB_EXAMPLE_HOST_ITERS bounds the loop (there's no real
 *   host to plug in, so pic8_usb_connected() would otherwise never go
 *   true); on target the loop runs forever, as firmware does.
 */

#include "pic8_usb.h"

#ifdef PIC8_USB_EXAMPLE_HOST_ITERS
#include "pic8_usb_test_support.h"
#include <stdio.h>
#endif

int main(void)
{
    pic8_usb_init();

#ifdef PIC8_USB_EXAMPLE_HOST_ITERS
    /* Host smoke run: simulate a host connecting and sending a few bytes. */
    pic8_usb_test_set_dtr(true);
    const uint8_t sent[] = "ping";
    pic8_usb_test_inject_rx(sent, sizeof(sent) - 1u);

    for (int i = 0; i < PIC8_USB_EXAMPLE_HOST_ITERS; i++) {
        pic8_usb_service();
        uint8_t b;
        while (pic8_usb_connected() && pic8_usb_read(&b, 1) == 1) {
            pic8_usb_write(&b, 1);
        }
    }
    pic8_usb_flush();

    printf("example_usb_echo (host smoke run): echoed %u bytes\n",
           (unsigned)pic8_usb_test_sent_len());
    return (pic8_usb_test_sent_len() == sizeof(sent) - 1u) ? 0 : 1;
#else
    for (;;) {
        pic8_usb_service();
        uint8_t b;
        if (pic8_usb_connected() && pic8_usb_read(&b, 1) == 1) {
            pic8_usb_write(&b, 1);
        }
    }
    return 0;
#endif
}
