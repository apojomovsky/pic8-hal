/**
 * @file    peripherals/pic18fxx5x_usart.h
 * @brief   EUSART driver, async + sync master/slave.
 *
 * @details
 *   Source: DS39632E §20.0 (EUSART), Registers 20-1 (TXSTA), 20-2 (RCSTA),
 *   20-3 (BAUDCON), §20.1 (BRG, Table 20-1 baud-rate formulas).
 *
 *   The API mirrors pic16f87xa_usart.h — the same `USART_HandleTypeDef`,
 *   `USART_*TypeDef`, `HAL_USART_*` functions and weak
 *   `USART_RX/TX_IRQHandler` — so consumer code is portable. The PIC18
 *   EUSART adds what the PIC16 plain USART lacks, all surfaced through
 *   BAUDCON and the SPBRGH high byte:
 *     - 16-bit baud generator (BAUDCON<BRG16>) with SPBRG:SPBRGH;
 *     - auto-baud detection (BAUDCON<ABDEN>, status ABDOVF);
 *     - wake-up enable (BAUDCON<WUE>) and sync clock/data polarity
 *       (BAUDCON<TXCKP>/<RXDTP>);
 *     - 9-bit address-detect mode (RCSTA<ADDEN>).
 *
 *   Wiring on the part:
 *     - Asynchronous full-duplex (TX on RC6, RX on RC7).
 *     - Synchronous master (clock on RC6, data on RC7).
 *     - Synchronous slave (clock on RC6, data on RC7, both input).
 *     - 8-bit or 9-bit data.
 *
 *   Reset state (DS39632E Table 5-1):
 *     - TXSTA = 0000 0010 (TRMT=1, others 0)
 *     - RCSTA = 0000 0000
 *     - BAUDCON = 0000 0000
 *     - SPBRG / SPBRGH = 0000 0000
 *
 *   Simpler than the PIC16 driver: the PIC18 EUSART registers are all in the
 *   Access Bank (no bank switching), and the BRG is configured through
 *   BAUDCON + SPBRGH instead of the PIC16's single BRGH bit + SPBRG. RMW on
 *   the SFRs uses split read+write (pic8_sfr_read8/write8) because XC8 cannot
 *   lower a compound assignment on a volatile cast-lvalue at a runtime SFR
 *   address (the Phase 2 codegen lesson). The handle is copied into owned
 *   storage (the Phase 3 lesson).
 *
 *   Only one EUSART instance exists on the PIC18F2455 family.
 */

#ifndef PIC18FXX5X_USART_H
#define PIC18FXX5X_USART_H

#include "pic18fxx5x.h"
#include "pic18fxx5x_sfr.h"

/**
 * @brief USART mode (TXSTA<SYNC>, DS39632E Register 20-1).
 */
typedef enum {
    USART_MODE_ASYNCHRONOUS  = 0x0U,   /**< SYNC = 0. */
    USART_MODE_SYNCHRONOUS   = 0x1U,   /**< SYNC = 1. */
} USART_ModeTypeDef;

/**
 * @brief Synchronous clock source (TXSTA<CSRC>, Register 20-1).
 *        Only meaningful in synchronous mode; ignored otherwise.
 */
typedef enum {
    USART_CLOCK_SLAVE        = 0x0U,   /**< CSRC = 0, clock from external. */
    USART_CLOCK_MASTER       = 0x1U,   /**< CSRC = 1, clock from BRG. */
} USART_ClockSourceTypeDef;

/**
 * @brief High/low baud-rate divisor (TXSTA<BRGH>, DS39632E §20.1).
 *        Combined with @ref USART_BaudGenTypeDef per Table 20-1.
 */
typedef enum {
    USART_BRGH_LOW           = 0x0U,   /**< BRGH = 0. */
    USART_BRGH_HIGH           = 0x1U,   /**< BRGH = 1. */
} USART_BaudRateHighTypeDef;

/**
 * @brief Baud-generator width (BAUDCON<BRG16>, DS39632E Register 20-3).
 *        BRG16=0 uses the 8-bit SPBRG; BRG16=1 uses the 16-bit
 *        SPBRG:SPBRGH pair, extending the divisor range to 0..65535.
 */
typedef enum {
    USART_BAUDGEN_8BIT       = 0x0U,   /**< BRG16 = 0, 8-bit BRG (SPBRG).   */
    USART_BAUDGEN_16BIT      = 0x1U,   /**< BRG16 = 1, 16-bit BRG (SPBRGH:SPBRG). */
} USART_BaudGenTypeDef;

/**
 * @brief Receive / transmit data width (RCSTA<RX9>, TXSTA<TX9>).
 */
typedef enum {
    USART_DATA_8BITS         = 0x0U,
    USART_DATA_9BITS         = 0x1U,
} USART_DataWidthTypeDef;

/**
 * @brief Compute the BRG divisor for a desired baud rate.
 *
 *   Async (Table 20-1):
 *     BRG16=0, BRGH=0: rate = FOSC / (64 × (X+1))
 *     BRG16=0, BRGH=1: rate = FOSC / (16 × (X+1))
 *     BRG16=1, BRGH=0: rate = FOSC / (16 × (X+1))
 *     BRG16=1, BRGH=1: rate = FOSC / (4  × (X+1))
 *   Sync:        rate = FOSC / (4  × (X+1))
 *
 * Returns 0..255 (BRG16=0) or 0..65535 (BRG16=1), or 0xFFFF if the
 * requested baud rate is unattainable (X would exceed the BRG range).
 * The caller splits the result into SPBRG (low byte) and SPBRGH (high
 * byte, BRG16=1 only).
 */
uint16_t USART_ComputeSPBRG(uint32_t fosc_hz, uint32_t baud,
                            USART_ModeTypeDef mode,
                            USART_BaudRateHighTypeDef brgh,
                            USART_BaudGenTypeDef brg16);

/** Driver handle (Cube-style). */
typedef struct {
    USART_ModeTypeDef          Mode;
    USART_ClockSourceTypeDef   ClockSource;     /**< Sync only. */
    USART_BaudRateHighTypeDef  BaudHigh;
    USART_BaudGenTypeDef       BaudGen;          /**< BRG16: 8- or 16-bit BRG. */
    USART_DataWidthTypeDef     DataWidth;
    uint8_t                    SPBRG;            /**< 0..255, BRG low byte. */
    uint8_t                    SPBRGH;           /**< 0..255, BRG high byte (BRG16=1). */
    uint8_t                    AddressDetect;    /**< ADDEN: 9-bit address detect. */
    uint8_t                    AutoBaud;         /**< ABDEN: auto-baud detect on init. */
    /** @brief  Optional TX-complete callback (fires on TXIF). */
    void (*TxCpltCallback)(void);
    /** @brief  Optional RX-complete callback (fires on RCIF). */
    void (*RxCpltCallback)(uint8_t data);
} USART_HandleTypeDef;

#define USART_HANDLE_DEFAULT {                                          \
    .Mode            = USART_MODE_ASYNCHRONOUS,                         \
    .ClockSource     = USART_CLOCK_MASTER,                              \
    .BaudHigh        = USART_BRGH_HIGH,                                 \
    .BaudGen         = USART_BAUDGEN_8BIT,                              \
    .DataWidth       = USART_DATA_8BITS,                                \
    .SPBRG           = 0,                                               \
    .SPBRGH          = 0,                                               \
    .AddressDetect   = 0,                                               \
    .AutoBaud        = 0,                                               \
    .TxCpltCallback  = NULL,                                            \
    .RxCpltCallback  = NULL,                                            \
}

/* ───────────────────────── init / deinit ────────────────────────── */

HAL_StatusTypeDef HAL_USART_Init(const USART_HandleTypeDef *h);
HAL_StatusTypeDef HAL_USART_DeInit(void);

/* ───────────────────────── transmit ──────────────────────────────── */

/**
 * @brief  Write one byte to TXREG. The write:
 *    - loads the byte into the TSR if it's empty (back-to-back transfer),
 *    - else parks it in TXREG until TSR drains,
 *    - clears TXIF (TXIF is read-only, cleared on TXREG write).
 *
 * @note   TXIF is NOT cleared by reading, only by writing TXREG.
 *         DS39632E §20.2.1.
 */
void HAL_USART_Transmit(uint8_t data);

/** Read the 9th bit (TX9D) just transmitted. */
uint8_t HAL_USART_GetTX9D(void);

/** Set the 9th bit to send NEXT. Must be set BEFORE writing TXREG. */
void HAL_USART_SetTX9D(uint8_t bit9);

/** Returns 1 if the TSR is empty (TRMT = 1). */
uint8_t HAL_USART_IsTxShiftRegisterEmpty(void);

/* ───────────────────────── receive ──────────────────────────────── */

/**
 * @brief  Read the latest byte from RCREG. Reading clears RCIF and
 *         advances the 2-deep FIFO.
 */
uint8_t HAL_USART_Receive(void);

/** Read RX9D, the 9th bit of the most recently received byte. */
uint8_t HAL_USART_GetRX9D(void);

/** Returns 1 if an overrun was detected (RCSTA<OERR>). Clear it with
 *  @ref HAL_USART_ClearOverrun (which cycles CREN). */
uint8_t HAL_USART_HasOverrun(void);

/** Clear an overrun: clear CREN, then re-set it (DS39632E §20.2.2). */
void HAL_USART_ClearOverrun(void);

/* ───────────────────────── auto-baud (BAUDCON) ────────────────────── */

/**
 * @brief  Start auto-baud detection (sets BAUDCON<ABDEN>). The hardware
 *         measures the next incoming byte and loads SPBRG:SPBRGH; ABDEN
 *         self-clears when done. Poll @ref HAL_USART_IsAutoBaudBusy.
 */
void HAL_USART_StartAutoBaud(void);

/** Returns 1 while auto-baud detection is in progress (ABDEN = 1). */
uint8_t HAL_USART_IsAutoBaudBusy(void);

/** Returns 1 if auto-baud overflowed (ABDOVF set — measurement exceeded range). */
uint8_t HAL_USART_HasAutoBaudOverflow(void);

/** Clear the auto-baud overflow flag (ABDOVF). */
void HAL_USART_ClearAutoBaudOverflow(void);

/* ───────────────────────── interrupts ───────────────────────────── */

/** Weak USART RX ISR, override in user code. */
void USART_RX_IRQHandler(void) PIC8_WEAK;
/** Weak USART TX ISR, override in user code. */
void USART_TX_IRQHandler(void) PIC8_WEAK;

#endif /* PIC18FXX5X_USART_H */