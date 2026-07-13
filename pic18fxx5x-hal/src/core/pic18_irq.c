/**
 * @file    pic18_irq.c
 * @brief   Implementation of @ref pic18_irq.h.
 *
 * @details
 *   Each IRQ source maps to a flag bit, an enable bit, and (except INT0)
 *   a priority bit, spread across INTCON / INTCON2 / INTCON3 / PIR1 /
 *   PIE1 / IPR1 (DS39632E §9.0, Register 9-1/9-2/9-3/9-5/9-6/9-8). The
 *   translation table at the top of this file holds those locations; every
 *   public function is a thin wrapper over it.
 *
 *   The master enable is GIEH (INTCON<7>) and, in priority mode, GIEL
 *   (INTCON<6>). HAL_IRQ_Disable/Restore treat them as a single "interrupts
 *   on/off" switch so the API is the drop-in equivalent of PIC16's GIE.
 *   Restoring to "on" also sets IPEN (RCON<7>) to activate the two-vector
 *   priority scheme.
 */

#include "core/pic18_irq.h"

/**
 * @brief Per-IRQ descriptor: where the flag, enable, and priority bits live.
 *        `prio_mask` = 0 means the source has no priority bit (INT0, which
 *        is always high-priority).
 */
typedef struct {
    uint16_t flag_addr;  /**< Address of the register holding the flag bit.  */
    uint8_t  flag_mask;  /**< Flag bit mask.                                 */
    uint16_t en_addr;    /**< Address of the register holding the enable bit.*/
    uint8_t  en_mask;    /**< Enable bit mask.                               */
    uint16_t prio_addr;  /**< Address of the priority register (0 if none).  */
    uint8_t  prio_mask;  /**< Priority bit mask (0 if none).                 */
} pic18_irq_desc_t;

static const pic18_irq_desc_t irq_table[] = {
    [PIC18_IRQ_INT0]     = { PIC_REG_INTCON,  PIC_INTCON_INT0IF,  PIC_REG_INTCON,  PIC_INTCON_INT0IE,  0,              0 },
    [PIC18_IRQ_INT1]     = { PIC_REG_INTCON3, PIC_INTCON3_INT1IF, PIC_REG_INTCON3, PIC_INTCON3_INT1IE, PIC_REG_INTCON3, PIC_INTCON3_INT1IP },
    [PIC18_IRQ_INT2]     = { PIC_REG_INTCON3, PIC_INTCON3_INT2IF, PIC_REG_INTCON3, PIC_INTCON3_INT2IE, PIC_REG_INTCON3, PIC_INTCON3_INT2IP },
    [PIC18_IRQ_RB]       = { PIC_REG_INTCON,  PIC_INTCON_RBIF,    PIC_REG_INTCON,  PIC_INTCON_RBIE,    PIC_REG_INTCON2, PIC_INTCON2_RBIP },
    [PIC18_IRQ_TMR0]     = { PIC_REG_INTCON,  PIC_INTCON_TMR0IF,  PIC_REG_INTCON,  PIC_INTCON_TMR0IE,  PIC_REG_INTCON2, PIC_INTCON2_TMR0IP },
    [PIC18_IRQ_TMR1]     = { PIC_REG_PIR1,    PIC_PIR1_TMR1IF,    PIC_REG_PIE1,    PIC_PIE1_TMR1IE,    PIC_REG_IPR1,    PIC_IPR1_TMR1IP },
    [PIC18_IRQ_TMR2]     = { PIC_REG_PIR1,    PIC_PIR1_TMR2IF,    PIC_REG_PIE1,    PIC_PIE1_TMR2IE,    PIC_REG_IPR1,    PIC_IPR1_TMR2IP },
    [PIC18_IRQ_TMR3]     = { PIC_REG_PIR2,    PIC_PIR2_TMR3IF,    PIC_REG_PIE2,    PIC_PIE2_TMR3IE,    PIC_REG_IPR2,    PIC_IPR2_TMR3IP },
    [PIC18_IRQ_CCP1]     = { PIC_REG_PIR1,    PIC_PIR1_CCP1IF,    PIC_REG_PIE1,    PIC_PIE1_CCP1IE,    PIC_REG_IPR1,    PIC_IPR1_CCP1IP },
    [PIC18_IRQ_SSP]      = { PIC_REG_PIR1,    PIC_PIR1_SSPIF,     PIC_REG_PIE1,    PIC_PIE1_SSPIE,     PIC_REG_IPR1,    PIC_IPR1_SSPIP },
    [PIC18_IRQ_USART_TX] = { PIC_REG_PIR1,    PIC_PIR1_TXIF,      PIC_REG_PIE1,    PIC_PIE1_TXIE,      PIC_REG_IPR1,    PIC_IPR1_TXIP },
    [PIC18_IRQ_USART_RX] = { PIC_REG_PIR1,    PIC_PIR1_RCIF,      PIC_REG_PIE1,    PIC_PIE1_RCIE,      PIC_REG_IPR1,    PIC_IPR1_RCIP },
    [PIC18_IRQ_ADC]      = { PIC_REG_PIR1,    PIC_PIR1_ADIF,      PIC_REG_PIE1,    PIC_PIE1_ADIE,      PIC_REG_IPR1,    PIC_IPR1_ADIP },
    [PIC18_IRQ_CCP2]     = { PIC_REG_PIR2,    PIC_PIR2_CCP2IF,    PIC_REG_PIE2,    PIC_PIE2_CCP2IE,    PIC_REG_IPR2,    PIC_IPR2_CCP2IP },
    [PIC18_IRQ_CMP]      = { PIC_REG_PIR2,    PIC_PIR2_CMIF,      PIC_REG_PIE2,    PIC_PIE2_CMIE,      PIC_REG_IPR2,    PIC_IPR2_CMIP },
    [PIC18_IRQ_EEPROM]   = { PIC_REG_PIR2,    PIC_PIR2_EEIF,      PIC_REG_PIE2,    PIC_PIE2_EEIE,      PIC_REG_IPR2,    PIC_IPR2_EEIP },
#if PIC18FXX5X_FAMILY_HAS_SPP
    [PIC18_IRQ_SPP]      = { PIC_REG_PIR1,    PIC_PIR1_SPPIF,     PIC_REG_PIE1,    PIC_PIE1_SPPIE,     PIC_REG_IPR1,    PIC_IPR1_SPPIP },
#endif
};

#define IRQ_TABLE_SIZE  (sizeof irq_table / sizeof irq_table[0])

/* RMW helpers. The PIC8_BIT_SET/CLR(PIC8_REG8(addr), mask) macros expand to
 * a compound assignment on a volatile cast-lvalue; XC8 cannot always lower
 * that for a runtime 16-bit SFR address (error 712). Splitting into an
 * explicit read + write via the platform's pic8_sfr_read8/write8 (a plain
 * load and store through a pointer) lowers cleanly on XC8 and stays
 * platform-agnostic (array indexing on the host). */
static void sfr_set(uint16_t addr, uint8_t mask)
{
    uint8_t v = pic8_sfr_read8(addr);
    pic8_sfr_write8(addr, (uint8_t)(v | mask));
}

static void sfr_clr(uint16_t addr, uint8_t mask)
{
    uint8_t v = pic8_sfr_read8(addr);
    pic8_sfr_write8(addr, (uint8_t)(v & (uint8_t)~mask));
}

/* ───────────────────────── public API ───────────────────────────── */

uint8_t HAL_IRQ_Disable(void)
{
    uint8_t intcon = pic8_sfr_read8(PIC_REG_INTCON);
    uint8_t prev = (intcon & (PIC_INTCON_GIEH | PIC_INTCON_GIEL)) ? 1U : 0U;
    sfr_clr(PIC_REG_INTCON, PIC_INTCON_GIEH);
    sfr_clr(PIC_REG_INTCON, PIC_INTCON_GIEL);
    return prev;
}

void HAL_IRQ_Restore(uint8_t prev_state)
{
    if (prev_state) {
        /* Activate the two-vector priority scheme (DS39632E §9.0, RCON<7>)
         * before enabling the masters, so high/low routing is in effect. */
        sfr_set(PIC_REG_RCON, PIC_RCON_IPEN);
        sfr_set(PIC_REG_INTCON, PIC_INTCON_GIEH);
        sfr_set(PIC_REG_INTCON, PIC_INTCON_GIEL);
    } else {
        sfr_clr(PIC_REG_INTCON, PIC_INTCON_GIEH);
        sfr_clr(PIC_REG_INTCON, PIC_INTCON_GIEL);
    }
}

void HAL_IRQ_Enable(PIC18_IRQn irq)
{
    if ((unsigned)irq >= IRQ_TABLE_SIZE) return;
    const pic18_irq_desc_t *d = &irq_table[irq];
    sfr_set(d->en_addr, d->en_mask);
}

void HAL_IRQ_DisableSrc(PIC18_IRQn irq)
{
    if ((unsigned)irq >= IRQ_TABLE_SIZE) return;
    const pic18_irq_desc_t *d = &irq_table[irq];
    sfr_clr(d->en_addr, d->en_mask);
}

void HAL_IRQ_ClearFlag(PIC18_IRQn irq)
{
    if ((unsigned)irq >= IRQ_TABLE_SIZE) return;
    const pic18_irq_desc_t *d = &irq_table[irq];
    sfr_clr(d->flag_addr, d->flag_mask);
}

uint8_t HAL_IRQ_GetFlag(PIC18_IRQn irq)
{
    if ((unsigned)irq >= IRQ_TABLE_SIZE) return 0U;
    const pic18_irq_desc_t *d = &irq_table[irq];
    return (pic8_sfr_read8(d->flag_addr) & d->flag_mask) ? 1U : 0U;
}

void HAL_IRQ_SetPriority(PIC18_IRQn irq, HAL_IRQ_Priority prio)
{
    if ((unsigned)irq >= IRQ_TABLE_SIZE) return;
    const pic18_irq_desc_t *d = &irq_table[irq];
    if (d->prio_mask == 0U) return;     /* INT0: always high, no bit to set. */
    if (prio == HAL_IRQ_PRIORITY_HIGH) {
        sfr_set(d->prio_addr, d->prio_mask);
    } else {
        sfr_clr(d->prio_addr, d->prio_mask);
    }
}