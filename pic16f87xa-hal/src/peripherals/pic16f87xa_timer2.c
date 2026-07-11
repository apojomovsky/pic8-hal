/**
 * @file    pic16f87xa_timer2.c
 * @brief   Timer2 driver, implementation (DS39582B §7.0).
 */

#include "peripherals/pic16f87xa_timer2.h"
#include "core/pic16f87xa_interrupt.h"

/* T2CON prescaler, DS39582B Register 7-1:
 *   00 → 1:1, 01 → 1:4, 1x → 1:16 */
static const uint8_t pre_ratio[4] = { 1, 4, 16, 16 };

/* T2CON postscaler, DS39582B Register 7-1: 1:(N+1). */
static const uint8_t post_ratio[16] = {
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16
};

static const TIMER2_HandleTypeDef *g_t2_handle = NULL;

uint8_t HAL_TIMER2_ReadCounter(void)
{
    return PIC16F87XA_REG8(PIC_REG_TMR2);
}

void HAL_TIMER2_WriteCounter(uint8_t value)
{
    PIC16F87XA_REG8(PIC_REG_TMR2) = value;
}

uint8_t HAL_TIMER2_ReadPeriod(void)
{
    /* PR2 lives in Bank 1 (DS39582B Register 7-2, address 0x92). */
    uint8_t prev = (PIC16F87XA_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
    pic_select_bank(1);
    uint8_t pr2 = PIC16F87XA_REG8(PIC_REG_PR2);
    pic_select_bank(prev);
    return pr2;
}

void HAL_TIMER2_WritePeriod(uint8_t period)
{
    uint8_t prev = (PIC16F87XA_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
    pic_select_bank(1);
    PIC16F87XA_REG8(PIC_REG_PR2) = period;
    pic_select_bank(prev);
}

uint16_t HAL_TIMER2_PrescalerToRatio(TIMER2_PrescalerTypeDef p)
{
    if ((unsigned)p > 3U) return 1U;
    return pre_ratio[p];
}

uint16_t HAL_TIMER2_PostscalerToRatio(TIMER2_PostscalerTypeDef p)
{
    if ((unsigned)p > 15U) return 1U;
    return post_ratio[p];
}

PIC16F87XA_StatusTypeDef HAL_TIMER2_Init(const TIMER2_HandleTypeDef *h)
{
    if (!h) return PIC16F87XA_INVALID;

    PIC16F87XA_BIT_CLR(PIC16F87XA_REG8(PIC_REG_T2CON), PIC_T2CON_TMR2ON);

    PIC16F87XA_IRQ_ClearFlag(PIC16F87XA_IRQ_TMR2);
    if (h->OverflowCallback) {
        PIC16F87XA_IRQ_Enable(PIC16F87XA_IRQ_TMR2);
    } else {
        PIC16F87XA_IRQ_DisableSrc(PIC16F87XA_IRQ_TMR2);
    }

    g_t2_handle = h;
    return PIC16F87XA_OK;
}

PIC16F87XA_StatusTypeDef HAL_TIMER2_DeInit(void)
{
    PIC16F87XA_IRQ_DisableSrc(PIC16F87XA_IRQ_TMR2);
    PIC16F87XA_IRQ_ClearFlag(PIC16F87XA_IRQ_TMR2);
    PIC16F87XA_REG8(PIC_REG_T2CON) = PIC_T2CON_POR_VALUE;
    HAL_TIMER2_WritePeriod(0xFFU);
    g_t2_handle = NULL;
    return PIC16F87XA_OK;
}

PIC16F87XA_StatusTypeDef HAL_TIMER2_Start(const TIMER2_HandleTypeDef *h)
{
    if (!h) return PIC16F87XA_INVALID;

    /* Period register first, DS39582B §7.0 recommends setting PR2
     * before enabling TMR2ON to avoid spurious matches. */
    HAL_TIMER2_WritePeriod(h->Period);

    /* Build T2CON:
     *   TOUTPS3:TOUTPS0 → bits 6:3
     *   TMR2ON          → bit 2
     *   T2CKPS1:T2CKPS0 → bits 1:0
     */
    uint8_t v = 0U;
    v |= (uint8_t)((h->Postscaler & 0xFU) << 3);
    v |= PIC_T2CON_TMR2ON;
    v |= (uint8_t)(h->Prescaler & 0x3U);
    PIC16F87XA_REG8(PIC_REG_T2CON) = v;

    return PIC16F87XA_OK;
}

PIC16F87XA_StatusTypeDef HAL_TIMER2_Stop(void)
{
    PIC16F87XA_BIT_CLR(PIC16F87XA_REG8(PIC_REG_T2CON), PIC_T2CON_TMR2ON);
    return PIC16F87XA_OK;
}

void TIMER2_IRQHandler(void)
{
    if (!PIC16F87XA_IRQ_GetFlag(PIC16F87XA_IRQ_TMR2)) return;
    PIC16F87XA_IRQ_ClearFlag(PIC16F87XA_IRQ_TMR2);
    if (g_t2_handle && g_t2_handle->OverflowCallback) {
        g_t2_handle->OverflowCallback();
    }
}