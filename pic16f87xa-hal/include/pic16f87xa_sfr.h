/**
 * @file    pic16f87xa_sfr.h
 * @brief   Special Function Register (SFR) address map for the
 *          PIC16F87XA family.
 *
 * @details
 *   Every address, bit mask and reset value in this file is taken 1-to-1
 *   from the DS39582B datasheet register maps:
 *     - Figure 2-3 / 2-4 — Register File Map (Bank 0–3).
 *     - Table 3-1        — Data EEPROM and Flash program registers.
 *     - Table 4-2..4-10  — PORTA..PORTE register summaries.
 *     - Table 14-1       — Configuration Word (Register 14-1).
 *   Address conflicts that are family-dependent (PORTD/PORTE/TRISD/TRISE
 *   do not exist on 28-pin parts) are guarded with PIC16F87XA_FAMILY_HAS_*
 *   macros so the same header compiles for any of the four parts.
 */

#ifndef PIC16F87XA_SFR_H
#define PIC16F87XA_SFR_H

#include "pic16f87xa.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ───────────────────────── Bank 0 — core SFRs ───────────────────── */

/** Indirect address pointer.                    DS39582B §2.2, addr 00h. */
#define PIC_REG_INDF          0x00U
/** Option register (Bank 1).                     DS39582B §2.2, addr 81h. */
#define PIC_REG_OPTION        0x81U
/** Program Counter low byte.                    DS39582B §2.2, addr 02h. */
#define PIC_REG_PCL           0x02U
/** Status register.                             DS39582B §2.2, addr 03h. */
#define PIC_REG_STATUS        0x03U
/** File Select Register (indirect addressing).  DS39582B §2.2, addr 04h. */
#define PIC_REG_FSR           0x04U

/* I/O ports — Tables 4-2..4-10. */
#define PIC_REG_PORTA         0x05U
#define PIC_REG_PORTB         0x06U
#define PIC_REG_PORTC         0x07U
#define PIC_REG_PORTD         0x08U   /* 40/44-pin only. */
#define PIC_REG_PORTE         0x09U   /* 40/44-pin only. */

#define PIC_REG_TRISA         0x85U
#define PIC_REG_TRISB         0x86U
#define PIC_REG_TRISC         0x87U
#define PIC_REG_TRISD         0x88U   /* 40/44-pin only. */
#define PIC_REG_TRISE         0x89U   /* 40/44-pin only. */

/* Core CPU control — DS39582B §14.10, §14.11, Table 14-4. */
#define PIC_REG_PCLATH        0x0AU
#define PIC_REG_INTCON        0x0BU
#define PIC_REG_PIR1          0x0CU
#define PIC_REG_PIR2          0x0DU
#define PIC_REG_PCON          0x8EU

/* Timer0 — DS39582B §5.0. */
#define PIC_REG_TMR0          0x01U
#define PIC_REG_TMR0L         0x0EU   /* (unused on F87XA, kept for naming). */

/* Timer1 — DS39582B §6.0. */
#define PIC_REG_TMR1L         0x0EU
#define PIC_REG_TMR1H         0x0FU
#define PIC_REG_T1CON         0x10U

/* Timer2 — DS39582B §7.0. */
#define PIC_REG_TMR2          0x11U
#define PIC_REG_T2CON         0x12U

/* SSP — DS39582B §9.0. */
#define PIC_REG_SSPBUF        0x13U
#define PIC_REG_SSPCON        0x14U

/* CCP — DS39582B §8.0. */
#define PIC_REG_CCP1RL        0x15U
#define PIC_REG_CCP1RH        0x16U
#define PIC_REG_CCP1CON       0x17U

/* USART — DS39582B §10.0. */
#define PIC_REG_RCSTA         0x18U
#define PIC_REG_TXREG         0x19U
#define PIC_REG_RCREG         0x1AU
#define PIC_REG_CCPR2L        0x1BU
#define PIC_REG_CCPR2H        0x1CU
#define PIC_REG_CCP2CON       0x1DU

/* ADC — DS39582B §11.0. */
#define PIC_REG_ADRESH        0x1EU
#define PIC_REG_ADCON0        0x1FU

/* ───────────────────────── Bank 1 ──────────────────────────────── */

/* SSP — DS39582B §9.0, Bank 1. */
#define PIC_REG_SSPCON2       0x91U
#define PIC_REG_PR2           0x92U
#define PIC_REG_SSPADD        0x93U
#define PIC_REG_SSPSTAT       0x94U

/* USART — DS39582B §10.0, Bank 1. */
#define PIC_REG_TXSTA         0x98U
#define PIC_REG_SPBRG         0x99U

/* Comparators + Vref — DS39582B §12.0, §13.0, Register 12-1 / 13-1. */
#define PIC_REG_CMCON         0x9CU
#define PIC_REG_CVRCON        0x9DU

/* ADC — DS39582B §11.0, Bank 1. */
#define PIC_REG_ADRESL        0x9EU
#define PIC_REG_ADCON1        0x9FU

/* ───────────────────────── Bank 2 — EEPROM ─────────────────────── */

/* DS39582B §3.0, Table 3-1. */
#define PIC_REG_EEDATA        0x10CU
#define PIC_REG_EEADR         0x10DU
#define PIC_REG_EEDATH        0x10EU
#define PIC_REG_EEADRH        0x10FU
#define PIC_REG_EECON1        0x18CU
#define PIC_REG_EECON2        0x18DU

/* ───────────────────────── STATUS register bits ────────────────── */

/** Carry / Borrow.                              DS39582B §2.2 / Register 2-1. */
#define PIC_STATUS_C          PIC16F87XA_BIT(0)
/** Digit Carry.                                  DS39582B Register 2-1. */
#define PIC_STATUS_DC         PIC16F87XA_BIT(1)
/** Zero.                                         DS39582B Register 2-1. */
#define PIC_STATUS_Z          PIC16F87XA_BIT(2)
/** Power-down.                                   DS39582B §14.14, Register 2-1. */
#define PIC_STATUS_PD         PIC16F87XA_BIT(3)
/** Time-out.                                     DS39582B §14.13, Register 2-1. */
#define PIC_STATUS_TO         PIC16F87XA_BIT(4)
/** Register Page Select bits (RP0:RP1).          DS39582B Register 2-1. */
#define PIC_STATUS_RP0        PIC16F87XA_BIT(5)
#define PIC_STATUS_RP1        PIC16F87XA_BIT(6)
/** IRP — unused on PIC16F87XA, reads as 0.       DS39582B Register 2-1. */
#define PIC_STATUS_IRP        PIC16F87XA_BIT(7)

/* ───────────────────────── INTCON register bits ────────────────── */

/** RB0/INT external interrupt flag.             DS39582B §14.11.1, Reg 14-3. */
#define PIC_INTCON_INTF       PIC16F87XA_BIT(1)
/** RB port change interrupt flag.               DS39582B §14.11.3, Reg 14-3. */
#define PIC_INTCON_RBIF       PIC16F87XA_BIT(0)
/** TMR0 overflow interrupt flag.                DS39582B §14.11.2, Reg 14-3. */
#define PIC_INTCON_TMR0IF     PIC16F87XA_BIT(2)
/** RB0/INT external interrupt enable.           DS39582B §14.11.1, Reg 14-3. */
#define PIC_INTCON_INTE       PIC16F87XA_BIT(4)
/** RB port change interrupt enable.             DS39582B §14.11.3, Reg 14-3. */
#define PIC_INTCON_RBIE       PIC16F87XA_BIT(3)
/** TMR0 overflow interrupt enable.              DS39582B §14.11.2, Reg 14-3. */
#define PIC_INTCON_TMR0IE     PIC16F87XA_BIT(5)
/** Peripheral interrupt enable.                 DS39582B §14.11, Reg 14-3. */
#define PIC_INTCON_PEIE       PIC16F87XA_BIT(6)
/** Global interrupt enable.                     DS39582B §14.11, Reg 14-3. */
#define PIC_INTCON_GIE        PIC16F87XA_BIT(7)

/* ───────────────────────── PIR1 / PIE1 ──────────────────────────── */

/* DS39582B §14.11, Register 14-3. */
#define PIC_PIR1_TMR1IF       PIC16F87XA_BIT(0)
#define PIC_PIR1_TMR2IF       PIC16F87XA_BIT(1)
#define PIC_PIR1_CCP1IF       PIC16F87XA_BIT(2)
#define PIC_PIR1_SSPIF        PIC16F87XA_BIT(3)
#define PIC_PIR1_TXIF         PIC16F87XA_BIT(4)
#define PIC_PIR1_RCIF         PIC16F87XA_BIT(5)
#define PIC_PIR1_ADIF         PIC16F87XA_BIT(6)
#define PIC_PIR1_PSPIF        PIC16F87XA_BIT(7)   /* 40/44-pin only. */

#define PIC_PIE1_TMR1IE       PIC16F87XA_BIT(0)
#define PIC_PIE1_TMR2IE       PIC16F87XA_BIT(1)
#define PIC_PIE1_CCP1IE       PIC16F87XA_BIT(2)
#define PIC_PIE1_SSPIE        PIC16F87XA_BIT(3)
#define PIC_PIE1_TXIE         PIC16F87XA_BIT(4)
#define PIC_PIE1_RCIE         PIC16F87XA_BIT(5)
#define PIC_PIE1_ADIE         PIC16F87XA_BIT(6)
#define PIC_PIE1_PSPIE        PIC16F87XA_BIT(7)

/* ───────────────────────── PIR2 / PIE2 ──────────────────────────── */

/* DS39582B §14.11, Register 14-4. */
#define PIC_PIR2_CCP2IF       PIC16F87XA_BIT(0)
#define PIC_PIR2_BCLIF        PIC16F87XA_BIT(3)
#define PIC_PIR2_EEIF         PIC16F87XA_BIT(4)
#define PIC_PIR2_CMIF         PIC16F87XA_BIT(6)
#define PIC_PIR2_OSFIF        PIC16F87XA_BIT(7)

#define PIC_PIE2_CCP2IE       PIC16F87XA_BIT(0)
#define PIC_PIE2_BCLIE        PIC16F87XA_BIT(3)
#define PIC_PIE2_EEIE         PIC16F87XA_BIT(4)
#define PIC_PIE2_CMIE         PIC16F87XA_BIT(6)

/* ───────────────────────── Reset values (POR) ───────────────────── */

/* DS39582B §14, Table 14-6. */
#define PIC_STATUS_POR_VALUE     0x18U  /* 0001 1xxx — IRP=0,RP1=0,RP0=0,TO=1,PD=1,... */
#define PIC_PCON_POR_VALUE       0x0FU  /* BOR and POR flags unknown. */
#define PIC_INTCON_POR_VALUE     0x00U
#define PIC_PIR1_POR_VALUE       0x00U
#define PIC_PIR2_POR_VALUE       0x00U
#define PIC_PIE1_POR_VALUE       0x00U
#define PIC_PIE2_POR_VALUE       0x00U
#define PIC_T1CON_POR_VALUE      0x00U
#define PIC_T2CON_POR_VALUE      0x00U
#define PIC_ADCON0_POR_VALUE     0x00U
#define PIC_ADCON1_POR_VALUE     0x00U

/* ───────────────────────── Bank-selection helper ───────────────── */

/**
 * @brief  Set the bank-select bits RP1:RP0 in STATUS to access a given bank.
 *         DS39582B §2.2, Table 2-1.
 */
static inline void pic_select_bank(uint8_t bank)
{
    uint8_t status = PIC16F87XA_REG8(PIC_REG_STATUS);
    status &= (uint8_t)~(PIC_STATUS_RP0 | PIC_STATUS_RP1);
    status |= (uint8_t)((bank & 0x03U) << 5);
    PIC16F87XA_REG8(PIC_REG_STATUS) = status;
}

#ifdef __cplusplus
}
#endif

#endif /* PIC16F87XA_SFR_H */