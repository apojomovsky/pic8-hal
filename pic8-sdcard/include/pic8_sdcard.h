/**
 * @file    pic8_sdcard.h
 * @brief   SD/MMC-over-SPI block storage for PIC18F2455/2550/4455/4550,
 *          wrapping the vendored M-Stack storage driver
 *          (third_party/m-stack-storage, see pic8-sdcard/docs/pic8-sdcard-plan.md).
 *
 * @details
 *   Thin on purpose: unlike pic8-usb, M-Stack's mmc_read_block/write_block
 *   are already the right shape -- this module's real job is binding
 *   MMC_SPI_TRANSFER/MMC_SPI_SET_CS/MMC_SPI_SET_SPEED to this repo's HAL
 *   (HAL_SSP_* and HAL_GPIO_WritePin) and the timer macros to pic8-tick,
 *   not inventing new buffering.
 *
 *   PIC18F2455/2550/4455/4550-only, same as pic8-usb, but for a RAM
 *   reason, not a peripheral-absence reason: MMC_BLOCK_SIZE is a fixed
 *   512 bytes, and every PIC16F87XA family member's total RAM (192-368 B)
 *   is smaller than one block. See the plan doc, "Chip scope."
 *
 *   Only one card is in scope (owns one mmc_card instance internally) --
 *   M-Stack's mmc.h supports an array of cards, not wrapped here since
 *   there's no concrete multi-card caller yet.
 */

#ifndef PIC8_SDCARD_H
#define PIC8_SDCARD_H

#include <stdint.h>
#include <stdbool.h>
#include "pic8_hal.h"   /* GPIO_TypeDef, needed by pic8_sdcard_pins_t below */

/** Card CS pin, board-specific -- SCK/SDI/SDO are fixed to the SSP
 *  peripheral's pins, but CS is ordinary GPIO wired however the board
 *  wires it. */
typedef struct {
    GPIO_TypeDef cs_port;
    uint16_t     cs_pin;
} pic8_sdcard_pins_t;

/**
 * @brief  Configure the SSP peripheral for SPI mode 0,0, assert CS idle,
 *         and run the SD/MMC card bring-up sequence (CMD0/CMD8/ACMD41/
 *         CMD58/CMD9 -- see the plan doc's "Confirmed API surface" for
 *         the full sequence). Blocks until the card responds or the
 *         M-Stack-internal retry/timeout bounds are hit.
 * @param  pins     which GPIO pin drives the card's CS line.
 * @param  fosc_hz  system oscillator frequency in Hz, needed to compute
 *                  the SPI clock divisor for both the slow bring-up speed
 *                  and the card's negotiated max speed.
 * @return true if the card initialized and pic8_sdcard_num_blocks()/
 *         read_block()/write_block() are now usable, false otherwise.
 */
bool pic8_sdcard_init(const pic8_sdcard_pins_t *pins, uint32_t fosc_hz);

/**
 * @brief  Re-query the card's status (SEND_STATUS-adjacent, see mmc_ready()
 *         in the vendored mmc.h). Has real SPI traffic cost -- don't call
 *         in a tight loop.
 */
bool pic8_sdcard_ready(void);

/**
 * @brief  Number of 512-byte blocks on the card, cached from init. 0 if
 *         not initialized.
 */
uint32_t pic8_sdcard_num_blocks(void);

/**
 * @brief  Read one 512-byte block.
 * @param  data  destination buffer, must be at least 512 bytes.
 * @return true on success (including CRC16 check passing).
 */
bool pic8_sdcard_read_block(uint32_t block_addr, uint8_t *data);

/**
 * @brief  Write one 512-byte block.
 * @param  data  source buffer, must be exactly 512 bytes.
 * @return true on success (card accepted the data and reported no write
 *         error via the follow-up SEND_STATUS check).
 */
bool pic8_sdcard_write_block(uint32_t block_addr, const uint8_t *data);

#endif /* PIC8_SDCARD_H */
