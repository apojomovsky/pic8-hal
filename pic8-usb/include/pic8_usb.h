/**
 * @file    pic8_usb.h
 * @brief   USB CDC-ACM (virtual serial) device for PIC18F2455/2550/4455/4550,
 *          wrapping the vendored M-Stack USB device stack
 *          (third_party/m-stack, see pic8-usb/docs/pic8-usb-plan.md).
 *
 * @details
 *   Deliberately mirrors pic8_serial.h's shape: same non-blocking
 *   read/available contract, same "write blocks until enqueued" contract.
 *   Anyone who already knows pic8_serial_* needs near-zero new mental model.
 *
 *   Unlike pic8_serial, this is PIC18F2455/2550/4455/4550-only by hardware
 *   necessity (M-Stack's usb_hal.h talks directly to the SIE/BDT; there is
 *   no per-family split, no PIC16 backend, and never will be -- PIC16F87XA
 *   parts have no USB peripheral). See the plan doc for why.
 *
 *   pic8_usb_service() must be called frequently (every main-loop
 *   iteration, or from a short-period pic8-taskmgr task) -- it pumps
 *   M-Stack's enumeration/control-transfer state machine and drains/fills
 *   the ring buffers below. Nothing else in this API does anything useful
 *   before pic8_usb_connected() is true.
 */

#ifndef PIC8_USB_H
#define PIC8_USB_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/** Ring-buffer size (power of two) for both TX and RX. Override by defining
 *  PIC8_USB_RING_SZ before including this header. Default is one full-speed
 *  bulk packet (64 bytes) -- the natural quantum for this endpoint size. */
#ifndef PIC8_USB_RING_SZ
#define PIC8_USB_RING_SZ 64u
#endif

/**
 * @brief  Initialize the USB peripheral and start enumeration. Does not
 *         block for enumeration to complete -- call pic8_usb_service()
 *         afterwards to drive it, and check pic8_usb_connected() before
 *         relying on the host being present.
 */
void pic8_usb_init(void);

/**
 * @brief  Pump the USB stack: services control transfers, drains the TX
 *         ring into the IN endpoint when the host is ready, and fills the
 *         RX ring from the OUT endpoint. Call this often (low
 *         single-digit milliseconds) -- see "Servicing cadence" in the
 *         plan doc for the two supported ways to drive it.
 */
void pic8_usb_service(void);

/**
 * @brief  Enqueue @p len bytes for transmission. Non-blocking with respect
 *         to the USB transfer itself, but blocks (calling
 *         pic8_usb_service() internally) if the TX ring is full, so the
 *         whole buffer is always enqueued before this returns.
 * @return the number of bytes enqueued (always @p len unless @p len is 0).
 */
size_t pic8_usb_write(const uint8_t *data, size_t len);

/**
 * @brief  Pull up to @p max received bytes from the RX ring. Non-blocking.
 * @return the number of bytes actually read (0 if nothing received).
 */
size_t pic8_usb_read(uint8_t *buf, size_t max);

/**
 * @brief  Number of bytes available to read from the RX ring.
 */
size_t pic8_usb_available(void);

/**
 * @brief  Block (servicing internally) until every enqueued TX byte has
 *         left the ring and the IN endpoint is no longer busy.
 */
void pic8_usb_flush(void);

/**
 * @brief  True once the host has the CDC port open (DTR asserted via the
 *         CDC SET_CONTROL_LINE_STATE request) -- not merely once USB
 *         enumeration finished. A host can enumerate the device and never
 *         open a terminal; gate output on this, not on "did init return."
 */
bool pic8_usb_connected(void);

#endif /* PIC8_USB_H */
