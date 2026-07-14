/**
 * @file    pic8_bus.h
 * @brief   Family-agnostic I2C/SPI "MEM" register-access idiom -- the
 *          `HAL_I2C_Mem_Read`/`Mem_Write` / SPI register-transaction layer
 *          Cube sensor code uses, for 8-bit PICs.
 *
 * @details
 *   Sits on the HAL's MSSP/SSP driver. The two operations every I2C/SPI
 *   sensor driver reimplements -- "write a register address, then write N
 *   bytes" and "write a register address, then read N bytes" -- are one call
 *   here. Works on every PIC this repo supports (PIC16F87XA and
 *   PIC18F2455/2550/4455/4550) and is host-testable.
 *
 *   The MEM transaction logic (start, addr+W, reg, restart, addr+R, read N
 *   with NACK-last, stop -- and the SPI select/exchange/deselect equivalent)
 *   is family-neutral and lives in src/pic8_bus.c. It calls a small "bus
 *   ops" interface (start/repeated_start/stop/write_byte/read_byte for I2C;
 *   select/exchange/deselect for SPI). The default ops wrap the HAL's SSP
 *   primitives plus the two pieces the HAL doesn't expose -- the ACKDT bit
 *   (NACK the last read byte) and the wait-for-SSPIF idle poll -- with a
 *   family branch for the SSPCON2/IRQn differences. `pic8_bus_set_i2c_ops`
 *   / `pic8_bus_set_spi_ops` inject an alternate ops table, which is how the
 *   host test wires in a mock I2C/SPI MEM device (the host sim has no SSP
 *   slave model, so end-to-end bus transactions can't run there).
 *
 *   See docs/ARCHITECTURE.md for the transaction shapes and the ops seam,
 *   and docs/API.md for the per-function reference.
 */

#ifndef PIC8_BUS_H
#define PIC8_BUS_H

#include <stdint.h>

/* ---- I2C ---- */

/** I2C bus operations (the MEM logic calls these). The default implementation
 *  wraps the HAL SSP driver; inject your own (e.g. a mock) for testing. */
typedef struct {
    void    (*start)(void);
    void    (*repeated_start)(void);
    void    (*stop)(void);
    int     (*write_byte)(uint8_t b);   /**< returns 1 if slave ACKed, 0 if NACK */
    uint8_t (*read_byte)(int ack);      /**< reads one byte; ack=1 sends ACK, 0 sends NACK */
} pic8_bus_i2c_ops_t;

/** Configure the MSSP as an I2C master at @p fscl_hz (from @p fosc_hz) and
 *  use the default (HAL) I2C ops. Call once before the mem_read/write. */
void pic8_bus_i2c_init(uint32_t fosc_hz, uint32_t fscl_hz);

/** Use a custom I2C ops table (NULL restores the default HAL ops). */
void pic8_bus_set_i2c_ops(const pic8_bus_i2c_ops_t *ops);

/** Write @p n bytes to register @p reg on I2C device @p dev (7-bit address).
 *  Transaction: START, (dev<<1)|W, reg, data..., STOP. Returns n on success
 *  (all ACKed), or -1 if the device NACKed the address or register. */
int pic8_bus_i2c_mem_write(uint8_t dev, uint8_t reg, const uint8_t *data, int n);

/** Read @p n bytes from register @p reg on I2C device @p dev (7-bit address).
 *  Transaction: START, (dev<<1)|W, reg, REPEATED-START, (dev<<1)|R,
 *  read n-1 with ACK, read last with NACK, STOP. Returns n on success, -1 on
 *  address/register NACK. */
int pic8_bus_i2c_mem_read(uint8_t dev, uint8_t reg, uint8_t *buf, int n);

/* ---- SPI ---- */

/** SPI bus operations. The default wraps the HAL SSP driver + a GPIO CS. */
typedef struct {
    void    (*select)(void);
    void    (*deselect)(void);
    uint8_t (*exchange)(uint8_t b);     /**< write MOSI byte, return MISO byte shifted in */
} pic8_bus_spi_ops_t;

/** Configure the MSSP as an SPI master and use @p cs_port/@p cs_pin (GPIO) as
 *  the chip-select (asserted low). Uses the default (HAL) SPI ops. */
void pic8_bus_spi_init(uint32_t fosc_hz, uint32_t f_sclk_hz, uint8_t cs_port, uint8_t cs_pin);

/** Use a custom SPI ops table (NULL restores the default HAL ops). */
void pic8_bus_set_spi_ops(const pic8_bus_spi_ops_t *ops);

/** Write @p n bytes to register @p reg over SPI: CS low, exchange(reg),
 *  exchange(data[0..n-1]), CS high. Returns n. */
int pic8_bus_spi_mem_write(uint8_t reg, const uint8_t *data, int n);

/** Read @p n bytes from register @p reg over SPI: CS low, exchange(reg),
 *  exchange(0) x n (capturing MISO), CS high. Returns n. */
int pic8_bus_spi_mem_read(uint8_t reg, uint8_t *buf, int n);

#endif /* PIC8_BUS_H */