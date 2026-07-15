/**
 * @file    pic8_sdcard_mock_spi.c
 * @brief   Mock SD-over-SPI card -- see pic8_sdcard_mock_spi.h.
 *
 * @details
 *   Dispatch is driven entirely by call *shape* (which of out_buf/in_buf
 *   is non-NULL, and len), matched against exactly how mmc.c actually
 *   calls MMC_SPI_TRANSFER -- confirmed by reading it, not guessed:
 *     - out-only, len==6            -> a 6-byte command frame
 *     - out-only, len==1            -> a pending write's 0xFE start token
 *     - out-only, len==512          -> a pending write's block data
 *     - out-only, len==2            -> a pending write's trailing CRC16
 *     - in-only, any len            -> drain the queued reply
 *     - both NULL                   -> pure clock filler ("give it 8 extra
 *                                       clocks", blank_clock()), no-op
 *   No other shape occurs in the command set this mock implements
 *   (CMD0/8/55/41/58/9/16/17/24/13 -- CMD25 multi-block write is not
 *   implemented, matching pic8_sdcard.h not wrapping it either).
 */

#include "pic8_sdcard_mock_spi.h"
#include "crc.h"

#include <string.h>

static bool     g_idle;
static uint8_t  g_acmd41_count;

typedef enum { WRITE_PHASE_NONE, WRITE_PHASE_TOKEN, WRITE_PHASE_DATA, WRITE_PHASE_CRC } write_phase_t;
static write_phase_t g_write_phase;
static uint32_t       g_write_block_addr;
static uint8_t         g_write_staging[PIC8_SDCARD_MOCK_BLOCK_SIZE];

#define RESP_QUEUE_CAP 520u
static uint8_t  g_resp[RESP_QUEUE_CAP];
static uint16_t g_resp_len;
static uint16_t g_resp_pos;

static uint8_t g_blocks[PIC8_SDCARD_MOCK_BACKING_BLOCKS][PIC8_SDCARD_MOCK_BLOCK_SIZE];

/* SDHC-style CSD v2.0: byte 0 top bits '01' = version 2.0; byte 3 =
 * TRAN_SPEED (time_val_code=1, transfer_unit=1 -> calculate_speed() in
 * mmc.c returns 1,000,000, an arbitrary nonzero value); bytes 7-9 = 0
 * means C_SIZE=0, so mmc.c computes card_size_blocks = (0+1)*1024 =
 * PIC8_SDCARD_MOCK_REPORTED_BLOCKS. Everything else is unread by mmc.c
 * for a CCS=1 (SDHC) card, left zero. */
static const uint8_t g_csd[16] = {
    0x40, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

/* ---- response queue ---- */

static void q_reset(void)
{
    g_resp_len = 0u;
    g_resp_pos = 0u;
}

static void q_byte(uint8_t b)
{
    if (g_resp_len < RESP_QUEUE_CAP) {
        g_resp[g_resp_len++] = b;
    }
}

static void q_bytes(const uint8_t *data, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++) {
        q_byte(data[i]);
    }
}

/* Data-block reply: idle gap, start token (7.3.3.2), the bytes, then the
 * CRC16.
 *
 * Byte order note (corrected from an initial wrong guess -- verified with
 * a standalone probe against the real vendored crc.c before settling
 * this): __read_data_block's self-check (continue the CRC16 accumulator
 * with the trailing CRC bytes in receive order, expect 0) only holds for
 * [high_byte, low_byte] (MSB-first). mmc_write_block sends
 * [low_byte, high_byte] instead, but that send is never self-checked by
 * anything in mmc.c (writes are validated by the card's data-response
 * token, not a self-check) -- so it's simply a different, unverified
 * convention on the write side, not evidence for what reads need. Only
 * MSB-first makes __read_data_block's actual math work; that's what this
 * mock (playing the *card*, i.e. the read path) must send. */
static void q_data_block(const uint8_t *data, uint16_t len)
{
    q_byte(0xFF);
    q_byte(0xFE);
    q_bytes(data, len);
    uint16_t ck = 0;
    ck = add_crc16_array(ck, (uint8_t *)data, len);
    q_byte((ck >> 8) & 0xffu);
    q_byte(ck & 0xffu);
}

/* ---- command decode ---- */

static void handle_command(const uint8_t *cmd6)
{
    uint8_t  idx = cmd6[0] & 0x3fu;
    uint32_t arg = ((uint32_t)cmd6[1] << 24) | ((uint32_t)cmd6[2] << 16) |
                   ((uint32_t)cmd6[3] << 8) | cmd6[4];

    q_reset();

    switch (idx) {
    case 0:     /* CMD0: GO_IDLE_STATE */
        g_idle = true;
        g_acmd41_count = 0u;
        q_byte(0x01u);
        break;

    case 8:     /* CMD8: SEND_IF_COND -- always "supported", echoes the check pattern */
        q_byte(0x01u); q_byte(0x00u); q_byte(0x00u); q_byte(0x01u); q_byte(0xA0u);
        break;

    case 55:    /* CMD55: APP_CMD -- also mmc_ready()'s own poll (its comment says CMD13,
                 * its code actually sends CMD55; harmless upstream comment/code
                 * mismatch, not something this mock needs to "fix") */
        q_byte(g_idle ? 0x01u : 0x00u);
        break;

    case 41:    /* ACMD41: SD_SEND_OP_COND -- idle once, then ready, so the retry loop
                 * in mmc_init_card is genuinely exercised at least once */
        if (g_acmd41_count < 1u) {
            q_byte(0x01u);
            g_acmd41_count++;
        } else {
            q_byte(0x00u);
            g_idle = false;
        }
        break;

    case 58:    /* CMD58: READ_OCR -- called twice; g_idle distinguishes which */
        if (g_idle) {
            q_byte(0x01u); q_byte(0x00u); q_byte(0xFFu); q_byte(0x80u); q_byte(0x00u);
        } else {
            /* buf[1] bit 0x40 set = CCS = 1 (SDHC/SDXC) */
            q_byte(0x00u); q_byte(0xC0u); q_byte(0x00u); q_byte(0x00u); q_byte(0x00u);
        }
        break;

    case 9:     /* CMD9: SEND_CSD */
        q_byte(0x00u);
        q_data_block(g_csd, sizeof(g_csd));
        break;

    case 16:    /* CMD16: SET_BLOCKLEN -- never reached (CCS=1 skips it in mmc_init_card);
                 * implemented harmlessly in case that ever changes */
        q_byte(0x00u);
        break;

    case 17:    /* CMD17: READ_SINGLE_BLOCK -- arg is already a block address (CCS=1) */
        q_byte(0x00u);
        if (arg < PIC8_SDCARD_MOCK_BACKING_BLOCKS) {
            q_data_block(g_blocks[arg], PIC8_SDCARD_MOCK_BLOCK_SIZE);
        }
        /* else: only R1 queued, no data block -- __read_data_block will
         * time out waiting for a start token it's never going to get,
         * same as a real card asked for an address it can't serve. */
        break;

    case 24:    /* CMD24: WRITE_SINGLE_BLOCK -- data phase follows via handle_write_data() */
        q_byte(0x00u);
        g_write_block_addr = arg;
        g_write_phase = WRITE_PHASE_TOKEN;
        break;

    case 13:    /* CMD13: SEND_STATUS (R2, 2 bytes) -- mmc_write_block's post-write check */
        q_byte(0x00u); q_byte(0x00u);
        break;

    default:
        q_byte(0x04u);  /* RESP_ILLEGAL_COMMAND -- honest about anything unimplemented */
        break;
    }
}

static void handle_write_data(const uint8_t *out, uint16_t len)
{
    switch (g_write_phase) {
    case WRITE_PHASE_TOKEN:                    /* len == 1: the 0xFE start token, value unchecked */
        g_write_phase = WRITE_PHASE_DATA;
        break;

    case WRITE_PHASE_DATA:                     /* len == PIC8_SDCARD_MOCK_BLOCK_SIZE */
        if (len == PIC8_SDCARD_MOCK_BLOCK_SIZE) {
            memcpy(g_write_staging, out, PIC8_SDCARD_MOCK_BLOCK_SIZE);
        }
        g_write_phase = WRITE_PHASE_CRC;
        break;

    case WRITE_PHASE_CRC:                      /* len == 2: trailing CRC16, not independently
                                                 * re-verified here -- mmc.c never reads it back
                                                 * either, so there's no "bad CRC" leg to model */
        if (g_write_block_addr < PIC8_SDCARD_MOCK_BACKING_BLOCKS) {
            memcpy(g_blocks[g_write_block_addr], g_write_staging, PIC8_SDCARD_MOCK_BLOCK_SIZE);
        }
        g_write_phase = WRITE_PHASE_NONE;
        q_reset();
        q_byte(0x05u);   /* data response token, low 5 bits = accepted (7.3.3.1) */
        q_byte(0x00u);   /* one busy byte, for realism */
        q_byte(0xFFu);   /* then ready */
        break;

    default:
        break;
    }
}

/* ---- MMC_SPI_* bindings (named in tests/mock/mmc_config.h) ---- */

void pic8_sdcard_mock_spi_transfer(uint8_t instance, const uint8_t *out_buf,
                                   uint8_t *in_buf, uint16_t len)
{
    (void)instance;

    if (out_buf != NULL && in_buf == NULL) {
        if (len == 6u) {
            handle_command(out_buf);
        } else if (g_write_phase != WRITE_PHASE_NONE) {
            handle_write_data(out_buf, len);
        }
        return;
    }

    if (in_buf != NULL && out_buf == NULL) {
        uint16_t i = 0;
        for (; i < len && g_resp_pos < g_resp_len; i++, g_resp_pos++) {
            in_buf[i] = g_resp[g_resp_pos];
        }
        for (; i < len; i++) {
            in_buf[i] = 0xFFu;   /* queue exhausted: idle filler, as real hardware would send */
        }
        return;
    }

    /* both NULL: pure clock filler (blank_clock(), the "8 extra clocks"), no-op */
}

void pic8_sdcard_mock_spi_set_cs(uint8_t instance, uint8_t value)
{
    (void)instance;
    (void)value;
}

void pic8_sdcard_mock_spi_set_speed(uint8_t instance, uint32_t speed_hz)
{
    (void)instance;
    (void)speed_hz;
}

/* ---- test-only setup/inspection (pic8_sdcard_mock_spi.h) ---- */

void pic8_sdcard_mock_reset(void)
{
    g_idle = true;
    g_acmd41_count = 0u;
    g_write_phase = WRITE_PHASE_NONE;
    g_write_block_addr = 0u;
    q_reset();
    memset(g_blocks, 0, sizeof(g_blocks));
}

uint8_t *pic8_sdcard_mock_block(uint32_t block_addr)
{
    if (block_addr >= PIC8_SDCARD_MOCK_BACKING_BLOCKS) {
        return NULL;
    }
    return g_blocks[block_addr];
}
