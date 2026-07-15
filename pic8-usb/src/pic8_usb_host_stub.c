/**
 * @file    pic8_usb_host_stub.c
 * @brief   Host-only test double for pic8_usb.h. Deliberately does NOT
 *          share implementation with pic8_usb.c.
 *
 * @details
 *   M-Stack's raw endpoint API (usb_get_in_buffer, usb_send_in_buffer, ...)
 *   has no meaningful host simulation -- there is no faithful stand-in for
 *   a real USB SIE enumerating against a real host driver stack. See
 *   "Host build story" in pic8-usb/docs/pic8-usb-plan.md.
 *
 *   What this file DOES prove, honestly: the public API's behavioral
 *   contract -- ring fill/drain ordering, overflow-drop policy,
 *   connected()'s DTR-gated state transitions, write() draining once
 *   connected. It proves nothing about real enumeration or the real
 *   endpoint-buffer code path in pic8_usb.c; that only gets proven on real
 *   silicon (Phase 3 of the plan).
 *
 *   The pic8_usb_test_* driver hooks (pic8_usb_test_support.h) simulate a
 *   host: pic8_usb_test_set_dtr() stands in for the CDC
 *   SET_CONTROL_LINE_STATE request, pic8_usb_test_inject_rx() stands in
 *   for bytes arriving on the OUT endpoint, and the sent-log stands in for
 *   bytes actually transmitted on the IN endpoint (only while "connected",
 *   matching the real module servicing EP2 only once usb_is_configured()
 *   -- here simplified to just the DTR gate, since enumeration itself
 *   isn't being modeled).
 */

#include "pic8_usb.h"
#include "pic8_usb_test_support.h"

#define MASK              (PIC8_USB_RING_SZ - 1u)
#define SENT_LOG_SZ       256u

static uint8_t g_tx_buf[PIC8_USB_RING_SZ];
static uint8_t g_tx_head, g_tx_tail, g_tx_count;
static uint8_t g_rx_buf[PIC8_USB_RING_SZ];
static uint8_t g_rx_head, g_rx_tail, g_rx_count;
static bool    g_dtr;

static uint8_t g_sent_log[SENT_LOG_SZ];
static size_t  g_sent_len;

void pic8_usb_init(void)
{
    g_tx_head = g_tx_tail = g_tx_count = 0u;
    g_rx_head = g_rx_tail = g_rx_count = 0u;
    g_dtr = false;
    g_sent_len = 0u;
}

void pic8_usb_service(void)
{
    if (!g_dtr) {
        return;                     /* real module gates draining on usb_is_configured() */
    }
    while (g_tx_count > 0u && g_sent_len < SENT_LOG_SZ) {
        g_sent_log[g_sent_len++] = g_tx_buf[g_tx_tail];
        g_tx_tail = (uint8_t)((g_tx_tail + 1u) & MASK);
        g_tx_count--;
    }
}

size_t pic8_usb_write(const uint8_t *data, size_t len)
{
    size_t n = 0;
    while (n < len && g_tx_count < PIC8_USB_RING_SZ) {
        g_tx_buf[g_tx_head] = data[n++];
        g_tx_head = (uint8_t)((g_tx_head + 1u) & MASK);
        g_tx_count++;
    }
    pic8_usb_service();
    return n;
}

size_t pic8_usb_read(uint8_t *buf, size_t max)
{
    size_t n = 0;
    while (n < max && g_rx_count > 0u) {
        buf[n++] = g_rx_buf[g_rx_tail];
        g_rx_tail = (uint8_t)((g_rx_tail + 1u) & MASK);
        g_rx_count--;
    }
    return n;
}

size_t pic8_usb_available(void)
{
    return (size_t)g_rx_count;
}

void pic8_usb_flush(void)
{
    pic8_usb_service();
}

bool pic8_usb_connected(void)
{
    return g_dtr;
}

/* ---- test-only driver hooks (pic8_usb_test_support.h) ---- */

void pic8_usb_test_set_dtr(bool on)
{
    g_dtr = on;
    if (on) {
        pic8_usb_service();
    }
}

size_t pic8_usb_test_inject_rx(const uint8_t *data, size_t len)
{
    size_t n = 0;
    while (n < len && g_rx_count < PIC8_USB_RING_SZ) {
        g_rx_buf[g_rx_head] = data[n++];
        g_rx_head = (uint8_t)((g_rx_head + 1u) & MASK);
        g_rx_count++;
    }
    return n;
}

size_t pic8_usb_test_sent_len(void)
{
    return g_sent_len;
}

const uint8_t *pic8_usb_test_sent_data(void)
{
    return g_sent_log;
}

void pic8_usb_test_reset_sent(void)
{
    g_sent_len = 0u;
}
