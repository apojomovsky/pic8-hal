/**
 * @file    pic8_sdcard_mock_spi.h
 * @brief   Test-only mock SD-over-SPI card: plays the card's side of the
 *          protocol well enough that the REAL vendored mmc.c logic (not a
 *          hand-written stand-in for it) can be host-tested directly.
 *
 * @details
 *   Implements the exact command sequence mmc_init_card/mmc_read_block/
 *   mmc_write_block/mmc_ready issue against a real card: CMD0, CMD8,
 *   CMD55+ACMD41, CMD58 (twice: pre- and post-bring-up, to report CCS),
 *   CMD9 (SEND_CSD, crafted to report an SDHC-style card with 1024
 *   reported 512-byte blocks), CMD17/CMD24 (single-block read/write,
 *   backed by a small in-memory block store), CMD13 (post-write status).
 *   Multi-block write (CMD25) is NOT implemented -- pic8_sdcard.h doesn't
 *   wrap it either (see pic8-sdcard-plan.md, "Public API design").
 *
 *   Bound into mmc.c via tests/mock/mmc_config.h's MMC_SPI_TRANSFER/
 *   MMC_SPI_SET_CS/MMC_SPI_SET_SPEED macros -- never linked into the real
 *   target build.
 */

#ifndef PIC8_SDCARD_MOCK_SPI_H
#define PIC8_SDCARD_MOCK_SPI_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/** Number of blocks actually backed by real storage (small, host RAM is
 *  cheap but there's no reason to allocate more than tests touch). The
 *  card still *reports* PIC8_SDCARD_MOCK_REPORTED_BLOCKS via CMD9/SEND_CSD,
 *  same as a real 512 KiB-ish card would -- only reads/writes within
 *  PIC8_SDCARD_MOCK_BACKING_BLOCKS are meaningful. */
#define PIC8_SDCARD_MOCK_BACKING_BLOCKS  4u
#define PIC8_SDCARD_MOCK_REPORTED_BLOCKS 1024u
#define PIC8_SDCARD_MOCK_BLOCK_SIZE      512u

/** Reset the mock to a freshly-inserted, uninitialized card: clears all
 *  protocol state (idle/ACMD41 retry count/pending write phase) AND
 *  zeroes the backing store. Call before every test. */
void pic8_sdcard_mock_reset(void);

/** Direct access to a backing block's 512 bytes, for pre-seeding data
 *  before a read test or inspecting it after a write test. Returns NULL
 *  if block_addr >= PIC8_SDCARD_MOCK_BACKING_BLOCKS. */
uint8_t *pic8_sdcard_mock_block(uint32_t block_addr);

#endif /* PIC8_SDCARD_MOCK_SPI_H */
