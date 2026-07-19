/**
 * @file    pic8_modbus.h
 * @brief   Family-agnostic Modbus RTU slave, built on `pic8-serial` (UART),
 *          `pic8-tick` (T3.5 silence timing), and the HAL's GPIO (optional
 *          RS-485 driver-enable).
 *
 * @details
 *   RTU only (binary framing, CRC-16), slave role only, function codes
 *   01/02/03/04/05/06/15/16 (read/write coils, discrete inputs, holding and
 *   input registers). No ASCII, no TCP, no master role, see
 *   `docs/ARCHITECTURE.md` for what's deliberately out of scope and why.
 *
 *   Register access is a plain data map (`pic8_modbus_slave_map_t`): the
 *   arrays *are* the data on both the host sim and real silicon, so unlike
 *   `pic8-bus`'s injectable ops seam (which exists to mock hardware the host
 *   sim can't model), no callback indirection is needed here.
 *
 *   `pic8_modbus_slave_poll()` is silence-delimited, not event-delimited:
 *   call it every main-loop iteration (or wire it as a `pic8-taskmgr` task).
 *   It drains newly received bytes into an internal frame buffer and, once
 *   `pic8-tick` shows the RTU T3.5 inter-frame gap has elapsed since the
 *   last received byte, validates and dispatches the frame in one call.
 *
 *   This header stays family-neutral (only <stdint.h>): the optional RS-485
 *   direction pin is identified by plain `port`/`pin` integers (mirroring
 *   `pic8_bus_spi_init`'s `cs_port`/`cs_pin` convention), not a HAL
 *   `GPIO_TypeDef`, so no HAL header needs to appear here. Only
 *   `src/pic8_modbus.c` includes `pic8_hal.h`, `pic8_serial.h`,
 *   `pic8_tick.h`.
 */

#ifndef PIC8_MODBUS_H
#define PIC8_MODBUS_H

#include <stdint.h>

/** Max RTU ADU (address + PDU + CRC) this module buffers. Override before
 *  including this header, e.g. -DPIC8_MODBUS_MAX_ADU=128, to fit a larger
 *  register map's read/write-multiple responses. */
#ifndef PIC8_MODBUS_MAX_ADU
#define PIC8_MODBUS_MAX_ADU 64u
#endif

/**
 * @brief  The slave's register map. Every array is caller-owned storage;
 *         this module only ever reads/writes through these pointers, it
 *         never allocates.
 *
 *   Coils and discrete inputs are bit-packed, LSB of `array[0]` is address
 *   0, matching Modbus's own bit-addressing order within a byte. Holding
 *   and input registers are one `uint16_t` per address. Any array may be
 *   NULL with its count 0 if the slave doesn't expose that table, requests
 *   against it get an ILLEGAL DATA ADDRESS exception.
 */
typedef struct {
    uint8_t        *coils;              /**< bit-packed, read/write        */
    uint16_t        num_coils;
    const uint8_t  *discrete_inputs;    /**< bit-packed, read-only         */
    uint16_t        num_discrete_inputs;
    uint16_t       *holding_regs;       /**< read/write                    */
    uint16_t        num_holding_regs;
    const uint16_t *input_regs;         /**< read-only                     */
    uint16_t        num_input_regs;
} pic8_modbus_slave_map_t;

/**
 * @brief  Initialize the Modbus RTU slave: configures the UART (via
 *         `pic8_serial_init`) at @p baud, precomputes the RTU T3.5
 *         inter-frame timeout for that baud, and stores @p slave_addr and
 *         @p map. Call once at startup, after `pic8_tick_init`.
 * @param  fosc_hz     system oscillator frequency in Hz.
 * @param  baud        RTU baud rate, e.g. 9600. Also drives the T3.5 timing,
 *                     see docs/ARCHITECTURE.md for the >19200 baud caveat.
 * @param  slave_addr  this slave's Modbus address, 1..247. 0 is reserved
 *                     for broadcast requests, do not pass it here.
 * @param  map         the register map (see @ref pic8_modbus_slave_map_t).
 *                     The pointer is stored, not copied, keep it alive for
 *                     the lifetime of the program.
 */
void pic8_modbus_slave_init(uint32_t fosc_hz, uint32_t baud,
                             uint8_t slave_addr,
                             const pic8_modbus_slave_map_t *map);

/**
 * @brief  Configure an optional RS-485 driver-enable pin, asserted for the
 *         duration of each response transmission. If never called, TX goes
 *         out through `pic8_serial_write` untouched (fine for point-to-point
 *         wiring or an auto-sensing transceiver).
 * @param  port  HAL GPIO port index (cast internally to the family's
 *               `GPIO_TypeDef`, e.g. `GPIOB` is port index 1).
 * @param  pin   bit index 0..7 within that port.
 */
void pic8_modbus_slave_set_rs485_dir_pin(uint8_t port, uint8_t pin);

/**
 * @brief  Poll the slave once. Call every main-loop iteration (or wire as a
 *         `pic8-taskmgr` task). Drains newly received UART bytes into the
 *         frame buffer; once the RTU T3.5 silence has elapsed since the
 *         last received byte, validates the frame (length, CRC-16, address
 *         match or broadcast) and, if valid, dispatches it and transmits
 *         the response (broadcast requests never get a response). The
 *         frame buffer is reset after every dispatch attempt, valid or not,
 *         so the next byte always starts a fresh frame.
 */
void pic8_modbus_slave_poll(void);

#endif /* PIC8_MODBUS_H */
