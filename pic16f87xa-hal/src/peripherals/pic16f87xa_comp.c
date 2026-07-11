/**
 * @file    pic16f87xa_comp.c
 * @brief   Comparator driver, implementation (DS39582B §12.0).
 */

#include "peripherals/pic16f87xa_comp.h"
#include "core/pic16f87xa_interrupt.h"

static const COMP_HandleTypeDef *g_comp = NULL;

PIC16F87XA_StatusTypeDef HAL_COMP_Init(const COMP_HandleTypeDef *h)
{
    if (!h) return PIC16F87XA_INVALID;
    g_comp = h;

    /* Build CMCON (Bank 1, address 0x9C). */
    uint8_t v = (uint8_t)(h->Mode & PIC_CMCON_CM_MASK);
    if (h->CIS)        v |= PIC_CMCON_CIS;
    if (h->C1Inverted) v |= PIC_CMCON_C1INV;
    if (h->C2Inverted) v |= PIC_CMCON_C2INV;
    {
        uint8_t prev = (PIC16F87XA_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
        pic_select_bank(1);
        PIC16F87XA_REG8(0x9CU) = v;
        pic_select_bank(prev);
    }

    /* Interrupt enable. */
    PIC16F87XA_IRQ_ClearFlag(PIC16F87XA_IRQ_CMP);
    if (h->ChangeCallback) PIC16F87XA_IRQ_Enable(PIC16F87XA_IRQ_CMP);
    else                   PIC16F87XA_IRQ_DisableSrc(PIC16F87XA_IRQ_CMP);

    return PIC16F87XA_OK;
}

PIC16F87XA_StatusTypeDef HAL_COMP_DeInit(void)
{
    PIC16F87XA_IRQ_DisableSrc(PIC16F87XA_IRQ_CMP);
    PIC16F87XA_IRQ_ClearFlag(PIC16F87XA_IRQ_CMP);
    {
        uint8_t prev = (PIC16F87XA_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
        pic_select_bank(1);
        PIC16F87XA_REG8(0x9CU) = 0x07U;     /* POR default: comparators off. */
        pic_select_bank(prev);
    }
    g_comp = NULL;
    return PIC16F87XA_OK;
}

uint8_t HAL_COMP_C1Out(void)
{
    uint8_t prev = (PIC16F87XA_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
    pic_select_bank(1);
    uint8_t v = PIC16F87XA_REG8(0x9CU);
    pic_select_bank(prev);
    return (v & PIC_CMCON_C1OUT) ? 1U : 0U;
}

uint8_t HAL_COMP_C2Out(void)
{
    uint8_t prev = (PIC16F87XA_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
    pic_select_bank(1);
    uint8_t v = PIC16F87XA_REG8(0x9CU);
    pic_select_bank(prev);
    return (v & PIC_CMCON_C2OUT) ? 1U : 0U;
}

uint8_t HAL_COMP_IsChangeFlag(void)
{
    /* CMIF lives in PIR2<6>. */
    return (PIC16F87XA_REG8(0x0DU) & 0x40U) ? 1U : 0U;
}

void HAL_COMP_ClearChangeFlag(void)
{
    PIC16F87XA_IRQ_ClearFlag(PIC16F87XA_IRQ_CMP);
}

void COMP_IRQHandler(void)
{
    if (!PIC16F87XA_IRQ_GetFlag(PIC16F87XA_IRQ_CMP)) return;
    PIC16F87XA_IRQ_ClearFlag(PIC16F87XA_IRQ_CMP);
    if (g_comp && g_comp->ChangeCallback) g_comp->ChangeCallback();
}