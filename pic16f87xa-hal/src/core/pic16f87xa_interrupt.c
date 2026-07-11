/**
 * @file    pic16f87xa_interrupt.c
 * @brief   Implementation of @ref pic16f87xa_interrupt.h.
 *
 * @details
 *   Each IRQ source maps to a specific bit in INTCON / PIE1 / PIE2
 *   (DS39582B §14.11 + Figure 14-10). The translation table lives at the
 *   top of this file, every other function is a thin wrapper.
 */

#include "core/pic16f87xa_interrupt.h"

/**
 * @brief Per-IRQ descriptor: which bank the enable / flag bit lives in,
 *        and at which position.
 *
 * PIC16F87XA interrupt layout:
 *   - RBIF, INTF, TMR0IF, plus their enable bits + GIE/PEIE → INTCON.
 *   - Peripheral flags → PIR1 (Bank 0) / PIR2 (Bank 0).
 *   - Peripheral enables → PIE1 (Bank 1) / PIE2 (Bank 1).
 *   - Bank 1 mirrors of PIR1/PIR2 are at 0x8C / 0x8D (DS39582B Figure 2-3).
 */
typedef struct {
    uint8_t flag_mask;     /**< PIR/INTCON bit to test/clear. */
    uint8_t enable_mask;   /**< PIE/INTCON bit to set/clear. */
    uint8_t in_intcon;     /**< 1 = INTCON, 0 = PIR1/PIR2. */
    uint8_t pir_is_pir2;   /**< 1 = PIR2, 0 = PIR1. (Ignored if in_intcon.) */
} irq_desc_t;

static const irq_desc_t irq_table[] = {
    [PIC16F87XA_IRQ_RB]       = { PIC_INTCON_RBIF,   PIC_INTCON_RBIE,   1, 0 },
    [PIC16F87XA_IRQ_INT]      = { PIC_INTCON_INTF,   PIC_INTCON_INTE,   1, 0 },
    [PIC16F87XA_IRQ_TMR0]     = { PIC_INTCON_TMR0IF, PIC_INTCON_TMR0IE, 1, 0 },
    [PIC16F87XA_IRQ_TMR1]     = { PIC_PIR1_TMR1IF,   PIC_PIE1_TMR1IE,   0, 0 },
    [PIC16F87XA_IRQ_TMR2]     = { PIC_PIR1_TMR2IF,   PIC_PIE1_TMR2IE,   0, 0 },
    [PIC16F87XA_IRQ_CCP1]     = { PIC_PIR1_CCP1IF,   PIC_PIE1_CCP1IE,   0, 0 },
    [PIC16F87XA_IRQ_CCP2]     = { PIC_PIR2_CCP2IF,   PIC_PIE2_CCP2IE,   0, 1 },
    [PIC16F87XA_IRQ_SSP]      = { PIC_PIR1_SSPIF,    PIC_PIE1_SSPIE,    0, 0 },
    [PIC16F87XA_IRQ_BCL]      = { PIC_PIR2_BCLIF,    PIC_PIE2_BCLIE,    0, 1 },
    [PIC16F87XA_IRQ_USART_TX] = { PIC_PIR1_TXIF,     PIC_PIE1_TXIE,     0, 0 },
    [PIC16F87XA_IRQ_USART_RX] = { PIC_PIR1_RCIF,     PIC_PIE1_RCIE,     0, 0 },
    [PIC16F87XA_IRQ_ADC]      = { PIC_PIR1_ADIF,     PIC_PIE1_ADIE,     0, 0 },
    [PIC16F87XA_IRQ_EEPROM]   = { PIC_PIR2_EEIF,     PIC_PIE2_EEIE,     0, 1 },
    [PIC16F87XA_IRQ_CMP]      = { PIC_PIR2_CMIF,     PIC_PIE2_CMIE,     0, 1 },
#if PIC16F87XA_FAMILY_HAS_PSP
    [PIC16F87XA_IRQ_PSP]      = { PIC_PIR1_PSPIF,    PIC_PIE1_PSPIE,    0, 0 },
#endif
};

#define IRQ_TABLE_SIZE  (sizeof irq_table / sizeof irq_table[0])

static uint8_t pir_reg_addr(const irq_desc_t *d)
{
    /* PIR1 = 0x0C, PIR2 = 0x0D. */
    return d->pir_is_pir2 ? PIC_REG_PIR2 : PIC_REG_PIR1;
}

static uint8_t pie_reg_addr(const irq_desc_t *d)
{
    /* PIE1 = 0x8C, PIE2 = 0x8D, Bank 1 mirrors of PIR1/PIR2 (Fig. 2-3). */
    return d->pir_is_pir2 ? 0x8DU : 0x8CU;
}

/* ───────────────────────── public API ───────────────────────────── */

uint8_t PIC16F87XA_IRQ_Disable(void)
{
    uint8_t s = PIC16F87XA_REG8(PIC_REG_INTCON);
    uint8_t prev = (s & PIC_INTCON_GIE) ? 1U : 0U;
    PIC16F87XA_REG8(PIC_REG_INTCON) = s & (uint8_t)~PIC_INTCON_GIE;
    return prev;
}

void PIC16F87XA_IRQ_Restore(uint8_t prev_state)
{
    if (prev_state) PIC16F87XA_BIT_SET(PIC16F87XA_REG8(PIC_REG_INTCON), PIC_INTCON_GIE);
    else            PIC16F87XA_BIT_CLR(PIC16F87XA_REG8(PIC_REG_INTCON), PIC_INTCON_GIE);
}

void PIC16F87XA_IRQ_Enable(PIC16F87XA_IrqTypeDef irq)
{
    if ((unsigned)irq >= IRQ_TABLE_SIZE) return;
    const irq_desc_t *d = &irq_table[irq];
    if (d->in_intcon) {
        PIC16F87XA_BIT_SET(PIC16F87XA_REG8(PIC_REG_INTCON), d->enable_mask);
    } else {
        /* Bank 1. */
        uint8_t prev_bank = (PIC16F87XA_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
        pic_select_bank(1);
        PIC16F87XA_BIT_SET(PIC16F87XA_REG8(pie_reg_addr(d)), d->enable_mask);
        /* Peripheral IRQs also need PEIE; auto-set it as a courtesy. */
        PIC16F87XA_BIT_SET(PIC16F87XA_REG8(PIC_REG_INTCON), PIC_INTCON_PEIE);
        pic_select_bank(prev_bank);
    }
}

void PIC16F87XA_IRQ_DisableSrc(PIC16F87XA_IrqTypeDef irq)
{
    if ((unsigned)irq >= IRQ_TABLE_SIZE) return;
    const irq_desc_t *d = &irq_table[irq];
    if (d->in_intcon) {
        PIC16F87XA_BIT_CLR(PIC16F87XA_REG8(PIC_REG_INTCON), d->enable_mask);
    } else {
        uint8_t prev_bank = (PIC16F87XA_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
        pic_select_bank(1);
        PIC16F87XA_BIT_CLR(PIC16F87XA_REG8(pie_reg_addr(d)), d->enable_mask);
        pic_select_bank(prev_bank);
    }
}

void PIC16F87XA_IRQ_ClearFlag(PIC16F87XA_IrqTypeDef irq)
{
    if ((unsigned)irq >= IRQ_TABLE_SIZE) return;
    const irq_desc_t *d = &irq_table[irq];
    if (d->in_intcon) {
        PIC16F87XA_BIT_CLR(PIC16F87XA_REG8(PIC_REG_INTCON), d->flag_mask);
    } else {
        PIC16F87XA_BIT_CLR(PIC16F87XA_REG8(pir_reg_addr(d)), d->flag_mask);
    }
}

uint8_t PIC16F87XA_IRQ_GetFlag(PIC16F87XA_IrqTypeDef irq)
{
    if ((unsigned)irq >= IRQ_TABLE_SIZE) return 0U;
    const irq_desc_t *d = &irq_table[irq];
    uint8_t reg = d->in_intcon
                ? PIC16F87XA_REG8(PIC_REG_INTCON)
                : PIC16F87XA_REG8(pir_reg_addr(d));
    return (reg & d->flag_mask) ? 1U : 0U;
}