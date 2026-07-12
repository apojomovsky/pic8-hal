/**
 * @file    pic18f2455_sfr.h
 * @brief   Special Function Register (SFR) address map for the
 *          PIC18F2455/2550/4455/4550 family (MVP subset).
 *
 * @details
 *   Every address, bit mask and reset value in this file is taken 1-to-1
 *   from the DS39632E datasheet:
 *     - Table 5-1, SFR Map (Access Bank 0xF60-0xFFF). The addresses are
 *       cross-checked against the PIC18Fxxxx DFP device header
 *       (pic/include/proc/pic18f4550.inc), the compiler's own datasheet-
 *       derived register map.
 *     - Register 5-2 (STATUS), Register 4-1/9-10 (RCON),
 *       Register 9-1/9-2/9-3 (INTCON/INTCON2/INTCON3),
 *       Register 9-5/9-6/9-8 (PIR1/PIE1/IPR1), Register 10-1 (PORTB),
 *       Register 11-1 (T0CON).
 *
 *   The macro naming (<tt>PIC_REG_* / PIC_*_*</tt>) deliberately matches
 *   pic16f87xa_sfr.h so that driver source reads identically across
 *   families: <tt>PIC8_REG8(PIC_REG_INTCON)</tt> resolves to 0x0B on PIC16
 *   and 0xFF2 on PIC18, selected by which family's headers are on the
 *   include path. Only one family's sfr.h is ever in a given translation
 *   unit, so the shared <tt>PIC_</tt> prefix never collides.
 *
 *   Phase 2 scope (the plan's MVP vertical slice): STATUS, BSR, RCON,
 *   PORTA-E / LATA-E / TRISA-E, INTCON / INTCON2 / INTCON3,
 *   PIR1 / PIE1 / IPR1, TMR0L / TMR0H / T0CON. Every SFR here is in the
 *   Access Bank (0xF60-0xFFF), so no BSR banking is needed to reach any
 *   of them (DS39632E §5.3). PORTD / PORTE / LATD / LATE / TRISD / TRISE
 *   exist only on the 40/44-pin parts (4455/4550) and are gated with
 *   PIC18F2455_FAMILY_HAS_PORTD / HAS_PORTE.
 */

#ifndef PIC18F2455_SFR_H
#define PIC18F2455_SFR_H

#include "pic18f2455.h"

/* ───────────────────────── Access Bank SFR addresses ────────────── */
/* DS39632E Table 5-1, SFR Map. All in the Access Bank (0xF60-0xFFF). */

/* Core CPU status, DS39632E Register 5-2. */
#define PIC_REG_STATUS        0xFD8U   /**< ALU status (N/OV/Z/DC/C).       */
#define PIC_REG_BSR           0xFE0U   /**< Bank Select Register (low nibble). */
#define PIC_REG_RCON          0xFD0U   /**< Reset Control (IPEN/TO/PD/POR/BOR). */

/* I/O ports, DS39632E §10.0, Table 10-1..10-5.
 * PORTx is the pin input sample; LATx is the output latch; TRISx is the
 * data-direction register. PIC18 exposes LATx as its own mapped register
 * (§10.0), so GPIO writes go through LATx, not PORTx. */
#define PIC_REG_PORTA         0xF80U
#define PIC_REG_PORTB         0xF81U
#define PIC_REG_PORTC         0xF82U
#if PIC18F2455_FAMILY_HAS_PORTD
#define PIC_REG_PORTD         0xF83U   /* 40/44-pin only. */
#endif
#if PIC18F2455_FAMILY_HAS_PORTE
#define PIC_REG_PORTE         0xF84U   /* 40/44-pin only. */
#endif

#define PIC_REG_LATA          0xF89U
#define PIC_REG_LATB          0xF8AU
#define PIC_REG_LATC          0xF8BU
#if PIC18F2455_FAMILY_HAS_PORTD
#define PIC_REG_LATD          0xF8CU   /* 40/44-pin only. */
#endif
#if PIC18F2455_FAMILY_HAS_PORTE
#define PIC_REG_LATE          0xF8DU   /* 40/44-pin only. */
#endif

#define PIC_REG_TRISA         0xF92U
#define PIC_REG_TRISB         0xF93U
#define PIC_REG_TRISC         0xF94U
#if PIC18F2455_FAMILY_HAS_PORTD
#define PIC_REG_TRISD         0xF95U   /* 40/44-pin only. */
#endif
#if PIC18F2455_FAMILY_HAS_PORTE
#define PIC_REG_TRISE         0xF96U   /* 40/44-pin only. */
#endif

/* Interrupt control, DS39632E §9.0, Register 9-1/9-2/9-3. */
#define PIC_REG_INTCON        0xFF2U   /**< GIE/PEIE/TMR0IE/INT0IE/RBIE + flags. */
#define PIC_REG_INTCON2       0xFF1U   /**< Pull-ups, INT edge, TMR0IP, RBIP.   */
#define PIC_REG_INTCON3       0xFF0U   /**< INT1/INT2 enable, flag, priority.   */

/* Peripheral interrupt flag/enable/priority, DS39632E Register 9-5/9-6/9-8. */
#define PIC_REG_PIR1          0xF9EU   /**< Peripheral Interrupt Flag Register 1. */
#define PIC_REG_PIE1          0xF9DU   /**< Peripheral Interrupt Enable Reg 1.    */
#define PIC_REG_IPR1          0xF9FU   /**< Peripheral Interrupt Priority Reg 1.  */

/* Timer0, DS39632E §11.0, Register 11-1. */
#define PIC_REG_T0CON         0xFD5U   /**< Timer0 control (on/8-16bit/src/edge/PSA/PS). */
#define PIC_REG_TMR0L         0xFD6U   /**< Timer0 low byte (also 8-bit mode value). */
#define PIC_REG_TMR0H         0xFD7U   /**< Timer0 high byte (16-bit mode only).    */

/* ───────────────────────── STATUS bits (Register 5-2) ───────────── */
#define PIC_STATUS_N          PIC8_BIT(4)   /**< Negative / borrow complement. */
#define PIC_STATUS_OV         PIC8_BIT(3)   /**< Overflow.                     */
#define PIC_STATUS_Z          PIC8_BIT(2)   /**< Zero.                         */
#define PIC_STATUS_DC         PIC8_BIT(1)   /**< Digit carry/borrow.           */
#define PIC_STATUS_C          PIC8_BIT(0)   /**< Carry/borrow.                 */

/* ───────────────────────── RCON bits (Register 4-1 / 9-10) ──────── */
#define PIC_RCON_IPEN         PIC8_BIT(7)   /**< Interrupt Priority Enable.    */
#define PIC_RCON_SBOREN       PIC8_BIT(6)   /**< BOR software enable.          */
#define PIC_RCON_RI           PIC8_BIT(4)   /**< RESET instruction flag.       */
#define PIC_RCON_TO           PIC8_BIT(3)   /**< WDT time-out flag (1=not).    */
#define PIC_RCON_PD           PIC8_BIT(2)   /**< Power-down (Sleep) flag (1=not). */
#define PIC_RCON_POR          PIC8_BIT(1)   /**< Power-on Reset status.        */
#define PIC_RCON_BOR          PIC8_BIT(0)   /**< Brown-out Reset status.       */

/* ───────────────────────── INTCON bits (Register 9-1) ───────────── */
#define PIC_INTCON_GIE        PIC8_BIT(7)   /**< Global Int Enable / high-priority GIE. */
#define PIC_INTCON_GIEH       PIC_INTCON_GIE
#define PIC_INTCON_PEIE       PIC8_BIT(6)   /**< Peripheral Int Enable / low-priority GIEL. */
#define PIC_INTCON_GIEL       PIC_INTCON_PEIE
#define PIC_INTCON_TMR0IE     PIC8_BIT(5)   /**< Timer0 overflow interrupt enable. */
#define PIC_INTCON_INT0IE     PIC8_BIT(4)   /**< INT0 external interrupt enable.   */
#define PIC_INTCON_RBIE       PIC8_BIT(3)   /**< RB<7:4> change interrupt enable.  */
#define PIC_INTCON_TMR0IF     PIC8_BIT(2)   /**< Timer0 overflow interrupt flag.   */
#define PIC_INTCON_INT0IF     PIC8_BIT(1)   /**< INT0 external interrupt flag.     */
#define PIC_INTCON_RBIF       PIC8_BIT(0)   /**< RB<7:4> change interrupt flag.    */

/* ───────────────────────── INTCON2 bits (Register 9-2) ──────────── */
#define PIC_INTCON2_RBPU      PIC8_BIT(7)   /**< PORTB pull-up enable (active-low). */
#define PIC_INTCON2_INTEDG0   PIC8_BIT(6)   /**< INT0 edge select (1=rising).       */
#define PIC_INTCON2_INTEDG1   PIC8_BIT(5)   /**< INT1 edge select.                  */
#define PIC_INTCON2_INTEDG2   PIC8_BIT(4)   /**< INT2 edge select.                  */
#define PIC_INTCON2_TMR0IP    PIC8_BIT(2)   /**< Timer0 overflow priority (1=high). */
#define PIC_INTCON2_RBIP      PIC8_BIT(0)   /**< RB change priority (1=high).       */

/* ───────────────────────── INTCON3 bits (Register 9-3) ──────────── */
#define PIC_INTCON3_INT2IP    PIC8_BIT(7)   /**< INT2 priority (1=high).            */
#define PIC_INTCON3_INT1IP    PIC8_BIT(6)   /**< INT1 priority (1=high).            */
#define PIC_INTCON3_INT2IE    PIC8_BIT(4)   /**< INT2 external interrupt enable.    */
#define PIC_INTCON3_INT1IE    PIC8_BIT(3)   /**< INT1 external interrupt enable.    */
#define PIC_INTCON3_INT2IF    PIC8_BIT(1)   /**< INT2 external interrupt flag.      */
#define PIC_INTCON3_INT1IF    PIC8_BIT(0)   /**< INT1 external interrupt flag.      */

/* ───────────────────────── PIR1 / PIE1 / IPR1 bits (Reg 9-5/9-6/9-8) ── */
/* Same bit layout across the three registers: flag / enable / priority. */
#define PIC_PIR1_SPPIF        PIC8_BIT(7)   /**< Streaming Parallel Port flag.  */
#define PIC_PIR1_ADIF         PIC8_BIT(6)   /**< A/D conversion complete flag.  */
#define PIC_PIR1_RCIF         PIC8_BIT(5)   /**< EUSART receive flag.           */
#define PIC_PIR1_TXIF         PIC8_BIT(4)   /**< EUSART transmit flag.          */
#define PIC_PIR1_SSPIF        PIC8_BIT(3)   /**< MSSP flag.                     */
#define PIC_PIR1_CCP1IF       PIC8_BIT(2)   /**< CCP1 flag.                     */
#define PIC_PIR1_TMR2IF       PIC8_BIT(1)   /**< Timer2 match flag.             */
#define PIC_PIR1_TMR1IF       PIC8_BIT(0)   /**< Timer1 overflow flag.          */
#define PIC_PIE1_SPPIE        PIC8_BIT(7)
#define PIC_PIE1_ADIE         PIC8_BIT(6)
#define PIC_PIE1_RCIE         PIC8_BIT(5)
#define PIC_PIE1_TXIE         PIC8_BIT(4)
#define PIC_PIE1_SSPIE        PIC8_BIT(3)
#define PIC_PIE1_CCP1IE       PIC8_BIT(2)
#define PIC_PIE1_TMR2IE       PIC8_BIT(1)
#define PIC_PIE1_TMR1IE       PIC8_BIT(0)
#define PIC_IPR1_SPPIP        PIC8_BIT(7)
#define PIC_IPR1_ADIP         PIC8_BIT(6)
#define PIC_IPR1_RCIP         PIC8_BIT(5)
#define PIC_IPR1_TXIP         PIC8_BIT(4)
#define PIC_IPR1_SSPIP        PIC8_BIT(3)
#define PIC_IPR1_CCP1IP       PIC8_BIT(2)
#define PIC_IPR1_TMR2IP       PIC8_BIT(1)
#define PIC_IPR1_TMR1IP       PIC8_BIT(0)

/* ───────────────────────── T0CON bits (Register 11-1) ───────────── */
#define PIC_T0CON_TMR0ON      PIC8_BIT(7)   /**< Timer0 on/off.                  */
#define PIC_T0CON_T08BIT      PIC8_BIT(6)   /**< 1=8-bit, 0=16-bit mode.         */
#define PIC_T0CON_T0CS        PIC8_BIT(5)   /**< 0=internal Fosc/4, 1=T0CKI pin. */
#define PIC_T0CON_T0SE        PIC8_BIT(4)   /**< Counter mode edge select.       */
#define PIC_T0CON_PSA         PIC8_BIT(3)   /**< 0=prescaler assigned, 1=not.    */
#define PIC_T0CON_T0PS_MASK   0x07U         /**< T0PS2:T0PS0, prescaler ratio.   */

/* ───────────────────────── Reset values (POR) ───────────────────── */
/* DS39632E Table 5-1 "Value at POR" column + Register 4-1 reset notes.
 * RCON after POR: IPEN=0, SBOREN=1, RI=0, TO=1, PD=1, POR=1, BOR=1
 * (POR/BOR set per Register 4-1 Note 1). IPR1 defaults to all-high
 * priority. T0CON resets to 0xFF (on, 8-bit, external, no prescaler). */
#define PIC_STATUS_POR_VALUE     0x00U
#define PIC_BSR_POR_VALUE        0x00U
#define PIC_RCON_POR_VALUE       0x57U
#define PIC_INTCON_POR_VALUE     0x00U
#define PIC_INTCON2_POR_VALUE    0xFBU
#define PIC_INTCON3_POR_VALUE    0xC0U
#define PIC_PIR1_POR_VALUE       0x00U
#define PIC_PIE1_POR_VALUE       0x00U
#define PIC_IPR1_POR_VALUE       0xFFU
#define PIC_T0CON_POR_VALUE      0xFFU
#define PIC_TRIS_POR_VALUE       0xFFU   /* All pins inputs after POR. */
#define PIC_LAT_POR_VALUE        0x00U   /* Output latches clear after POR. */
#define PIC_PORT_POR_VALUE       0x00U

#endif /* PIC18F2455_SFR_H */