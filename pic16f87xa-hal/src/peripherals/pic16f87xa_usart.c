/**
 * @file    pic16f87xa_usart.c
 * @brief   USART driver — implementation (DS39582B §10.0).
 *
 *   Sim backend notes:
 *     - TXIF = 1 by default after init, becomes 0 on TXREG write, returns
 *       to 1 once TSR has "drained" (we approximate this with a
 *       configurable cycle count).
 *     - RCIF = 0 by default. Becomes 1 when a byte arrives in RCREG,
 *       cleared by reading RCREG.
 *     - The sim backend does NOT actually shift bits onto the TX pin —
 *       the test rig observes TXREG / RCREG / flags directly. A more
 *       elaborate sim would model the bit-time BRG; for now we just
 *       assert the driver programs the registers correctly.
 */

#include "peripherals/pic16f87xa_usart.h"
#include "core/pic16f87xa_interrupt.h"

/* ───────────────────────── SPBRG computation ────────────────────── */

uint16_t USART_ComputeSPBRG(uint32_t fosc_hz, uint32_t baud,
                            USART_ModeTypeDef mode,
                            USART_BaudRateHighTypeDef brgh)
{
    if (baud == 0) return 0xFFFFU;
    uint32_t divisor = (mode == USART_MODE_ASYNCHRONOUS)
                       ? (brgh == USART_BRGH_HIGH ? 16U : 64U)
                       : 4U;
    /* SPBRG = (Fosc / (divisor × baud)) - 1 */
    uint32_t x = (fosc_hz / (divisor * baud)) - 1U;
    if (x > 255U) return 0xFFFFU;
    return (uint16_t)x;
}

/* ───────────────────────── handle storage ───────────────────────── */

static const USART_HandleTypeDef *g_usart = NULL;

/* ───────────────────────── public API ───────────────────────────── */

PIC16F87XA_StatusTypeDef HAL_USART_Init(const USART_HandleTypeDef *h)
{
    if (!h) return PIC16F87XA_INVALID;
    g_usart = h;

    /* Program SPBRG (Bank 1, address 0x99 — DS39582B §10.1). */
    uint8_t prev = (PIC16F87XA_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
    pic_select_bank(1);
    PIC16F87XA_REG8(PIC_REG_SPBRG) = h->SPBRG;
    pic_select_bank(prev);

    /* Build TXSTA (Bank 1, address 0x98).
     *   CSRC  bit 7 — sync clock source
     *   TX9   bit 6 — 9-bit TX
     *   TXEN  bit 5 — TX enable
     *   SYNC  bit 4 — sync/async
     *   BRGH  bit 2 — high baud rate
     *   TX9D  bit 0 — 9th bit
     * Reset value of TXSTA: 0000 -010 (TRMT=1). */
    uint8_t txsta = 0x02U;
    if (h->Mode == USART_MODE_SYNCHRONOUS) txsta |= PIC_TXSTA_SYNC;
    if (h->Mode == USART_MODE_SYNCHRONOUS &&
        h->ClockSource == USART_CLOCK_MASTER) txsta |= PIC_TXSTA_CSRC;
    if (h->BaudHigh == USART_BRGH_HIGH) txsta |= PIC_TXSTA_BRGH;
    if (h->DataWidth == USART_DATA_9BITS) txsta |= PIC_TXSTA_TX9;
    if (h->TxCpltCallback) txsta |= PIC_TXSTA_TXEN;  /* TXEN implied if user has a callback. */
    PIC16F87XA_REG8(PIC_REG_TXSTA) = txsta;

    /* Build RCSTA (Bank 0, address 0x18).
     *   SPEN bit 7 — enable serial port
     *   RX9  bit 6 — 9-bit RX
     *   CREN bit 4 — continuous receive enable
     *   ADDEN bit 3 — address detect
     * Reset value: 0000 000x. */
    uint8_t rcsta = PIC_RCSTA_SPEN;
    if (h->DataWidth == USART_DATA_9BITS) rcsta |= PIC_RCSTA_RX9;
    if (h->RxCpltCallback) rcsta |= PIC_RCSTA_CREN;
    PIC16F87XA_REG8(PIC_REG_RCSTA) = rcsta;

    /* TXIF is initially 1 (TXREG empty after reset, §10.2.1).
     * RCIF is initially 0 (RCREG empty after reset). */
    PIC16F87XA_IRQ_ClearFlag(PIC16F87XA_IRQ_USART_RX);

    if (h->TxCpltCallback) PIC16F87XA_IRQ_Enable(PIC16F87XA_IRQ_USART_TX);
    else                   PIC16F87XA_IRQ_DisableSrc(PIC16F87XA_IRQ_USART_TX);
    if (h->RxCpltCallback) PIC16F87XA_IRQ_Enable(PIC16F87XA_IRQ_USART_RX);
    else                   PIC16F87XA_IRQ_DisableSrc(PIC16F87XA_IRQ_USART_RX);

    return PIC16F87XA_OK;
}

PIC16F87XA_StatusTypeDef HAL_USART_DeInit(void)
{
    PIC16F87XA_IRQ_DisableSrc(PIC16F87XA_IRQ_USART_TX);
    PIC16F87XA_IRQ_DisableSrc(PIC16F87XA_IRQ_USART_RX);
    PIC16F87XA_IRQ_ClearFlag(PIC16F87XA_IRQ_USART_TX);
    PIC16F87XA_IRQ_ClearFlag(PIC16F87XA_IRQ_USART_RX);
    PIC16F87XA_REG8(PIC_REG_RCSTA) = 0x00U;
    PIC16F87XA_REG8(PIC_REG_TXSTA) = 0x02U;     /* keep TRMT=1 reset state. */
    {
        uint8_t prev = (PIC16F87XA_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
        pic_select_bank(1);
        PIC16F87XA_REG8(PIC_REG_SPBRG) = 0x00U;
        pic_select_bank(prev);
    }
    g_usart = NULL;
    return PIC16F87XA_OK;
}

void HAL_USART_Transmit(uint8_t data)
{
    /* Writing TXREG:
     *   - clears TXIF immediately (DS39582B §10.2.1).
     *   - if TSR is empty, transfers immediately (back-to-back).
     *   - else parks in TXREG until TSR drains.
     * In the sim we model the transfer as instantaneous: clear TXIF
     * synchronously with the write. A cycle-accurate sim would defer. */
    PIC16F87XA_REG8(PIC_REG_TXREG) = data;
    /* Sim: TXIF goes low on the write, back high when the next byte
     * loads TSR. We model "back high" as the next sim_step iteration. */
    PIC16F87XA_IRQ_ClearFlag(PIC16F87XA_IRQ_USART_TX);
}

uint8_t HAL_USART_GetTX9D(void)
{
    return (PIC16F87XA_REG8(PIC_REG_TXSTA) & PIC_TXSTA_TX9D) ? 1U : 0U;
}

void HAL_USART_SetTX9D(uint8_t bit9)
{
    if (bit9) PIC16F87XA_BIT_SET(PIC16F87XA_REG8(PIC_REG_TXSTA), PIC_TXSTA_TX9D);
    else      PIC16F87XA_BIT_CLR(PIC16F87XA_REG8(PIC_REG_TXSTA), PIC_TXSTA_TX9D);
}

uint8_t HAL_USART_IsTxShiftRegisterEmpty(void)
{
    return (PIC16F87XA_REG8(PIC_REG_TXSTA) & PIC_TXSTA_TRMT) ? 1U : 0U;
}

uint8_t HAL_USART_Receive(void)
{
    /* Reading RCREG clears RCIF (DS39582B §10.2.2). */
    uint8_t data = PIC16F87XA_REG8(PIC_REG_RCREG);
    PIC16F87XA_IRQ_ClearFlag(PIC16F87XA_IRQ_USART_RX);
    return data;
}

uint8_t HAL_USART_GetRX9D(void)
{
    return (PIC16F87XA_REG8(PIC_REG_RCSTA) & PIC_RCSTA_RX9D) ? 1U : 0U;
}

/* ───────────────────────── ISRs ─────────────────────────────────── */

void USART_TX_IRQHandler(void)
{
    if (!PIC16F87XA_IRQ_GetFlag(PIC16F87XA_IRQ_USART_TX)) return;
    /* TXIF is read-only and cleared by writing TXREG — there is nothing
     * to clear here. We just call the user callback. */
    if (g_usart && g_usart->TxCpltCallback) g_usart->TxCpltCallback();
}

void USART_RX_IRQHandler(void)
{
    if (!PIC16F87XA_IRQ_GetFlag(PIC16F87XA_IRQ_USART_RX)) return;
    uint8_t data = PIC16F87XA_REG8(PIC_REG_RCREG);
    PIC16F87XA_IRQ_ClearFlag(PIC16F87XA_IRQ_USART_RX);
    if (g_usart && g_usart->RxCpltCallback) g_usart->RxCpltCallback(data);
}