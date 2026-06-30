/**
 * @file    pic16f87xa_psp.c
 * @brief   Parallel Slave Port driver — implementation (DS39582B §4.5).
 */

#include "peripherals/pic16f87xa_psp.h"
#include "core/pic16f87xa_interrupt.h"

static void (*g_psp_cb)(void) = NULL;

static uint8_t b1_trise(void)
{
    uint8_t prev = (PIC16F87XA_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
    pic_select_bank(1);
    uint8_t v = PIC16F87XA_REG8(0x89U);
    pic_select_bank(prev);
    return v;
}

static void b1_trise_write(uint8_t v)
{
    uint8_t prev = (PIC16F87XA_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
    pic_select_bank(1);
    PIC16F87XA_REG8(0x89U) = v;
    pic_select_bank(prev);
}

PIC16F87XA_StatusTypeDef HAL_PSP_Init(void (*callback)(void))
{
    g_psp_cb = callback;
    /* Clear the read-only status flags (IBF, OBF, IBOV) by writing
     * TRISE with them clear. The lower 4 bits of TRISE are
     * read-only — we leave PSPIE/PSPMODE managed by the user. */
    b1_trise_write(b1_trise() &
                   (uint8_t)~(PIC_TRISE_IBF | PIC_TRISE_OBF | PIC_TRISE_IBOV));
    PIC16F87XA_IRQ_ClearFlag(PIC16F87XA_IRQ_PSP);
    if (callback) PIC16F87XA_IRQ_Enable(PIC16F87XA_IRQ_PSP);
    else          PIC16F87XA_IRQ_DisableSrc(PIC16F87XA_IRQ_PSP);
    return PIC16F87XA_OK;
}

PIC16F87XA_StatusTypeDef HAL_PSP_DeInit(void)
{
    PIC16F87XA_IRQ_DisableSrc(PIC16F87XA_IRQ_PSP);
    PIC16F87XA_IRQ_ClearFlag(PIC16F87XA_IRQ_PSP);
    b1_trise_write(0x07U);    /* POR default: I/O mode, no PSP. */
    g_psp_cb = NULL;
    return PIC16F87XA_OK;
}

void HAL_PSP_Enable(void)
{
    b1_trise_write(b1_trise() | PIC_TRISE_PSPMODE);
}

void HAL_PSP_Disable(void)
{
    b1_trise_write(b1_trise() & (uint8_t)~PIC_TRISE_PSPMODE);
}

uint8_t HAL_PSP_IsInputBufferFull(void)
{
    return (b1_trise() & PIC_TRISE_IBF) ? 1U : 0U;
}

uint8_t HAL_PSP_IsOutputBufferFull(void)
{
    return (b1_trise() & PIC_TRISE_OBF) ? 1U : 0U;
}

uint8_t HAL_PSP_HasInputOverflow(void)
{
    return (b1_trise() & PIC_TRISE_IBOV) ? 1U : 0U;
}

void HAL_PSP_ClearInputOverflow(void)
{
    b1_trise_write(b1_trise() & (uint8_t)~PIC_TRISE_IBOV);
}

void PSP_IRQHandler(void)
{
    if (!PIC16F87XA_IRQ_GetFlag(PIC16F87XA_IRQ_PSP)) return;
    PIC16F87XA_IRQ_ClearFlag(PIC16F87XA_IRQ_PSP);
    if (g_psp_cb) g_psp_cb();
}