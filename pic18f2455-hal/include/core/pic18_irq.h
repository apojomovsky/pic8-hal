/**
 * @file    core/pic18_irq.h
 * @brief   PIC18F2455 family interrupt controller: the IRQn enum and the
 *          enable / disable / flag / priority helpers. The family-blind
 *          dispatch contract (pic8_dispatch_all_irqs) lives in
 *          pic8_harness.h; the shared priority enum lives in pic8_irq.h.
 *
 * @details
 *   Mirrors `HAL_NVIC_*` from STM32Cube and the PIC16 `HAL_IRQ_*` API: the
 *   application never touches INTCON / INTCON2 / INTCON3 / PIE1 / PIR1 /
 *   IPR1 directly; it goes through these helpers so the routing logic is
 *   portable. The `HAL_IRQ_*` function names are shared by every 8-bit PIC
 *   family (each implements them against its own interrupt registers); the
 *   `PIC18_IRQn` enum and the register addresses behind those helpers are
 *   PIC18F2455-specific.
 *
 *   PIC18 interrupt architecture (DS39632E §9.0):
 *     - Two vectors: 0008h high-priority, 0018h low-priority.
 *     - IPEN (RCON<7>) enables the priority scheme. When IPEN = 0 the
 *       controller is in PIC16-compatible mode (single vector at 0008h,
 *       GIE/PEIE master enables, no priority); when IPEN = 1 the two
 *       vectors and the per-source priority bits (INTCON2 / INTCON3 /
 *       IPR1) take effect, with GIEH gating high- and GIEL gating
 *       low-priority sources.
 *     - INT0 has no priority bit and is always high-priority.
 *
 *   Phase 2 default: the core runs in priority mode (IPEN = 1). The
 *   master-enable helpers HAL_IRQ_Disable / Restore operate on GIEH and
 *   GIEL together, so HAL_IRQ_Restore(1) is the drop-in equivalent of
 *   PIC16's "GIE = 1" (enable all interrupts). HAL_IRQ_SetPriority writes
 *   the matching priority bit; sources default to high after reset
 *   (INTCON2 / INTCON3 / IPR1 reset to all-ones, DS39632E Table 5-1).
 *
 *   Sources covered (the MVP + the PIR1 peripheral set):
 *     - INTCON:   TMR0, INT0, RB<7:4> change.
 *     - INTCON3:  INT1, INT2.
 *     - PIR1:     TMR1, TMR2, CCP1, SSP, USART RX/TX, ADC, SPP.
 */

#ifndef PIC18_IRQ_H
#define PIC18_IRQ_H

#include "pic18f2455.h"
#include "pic18f2455_sfr.h"
#include "core/pic8_irq.h"   /* shared HAL_IRQ_Priority enum (family-blind) */

/**
 * @brief Logical identity of every interrupt source on the part.
 *        Used as the parameter for enable / disable / clear / status /
 *        priority calls.
 */
typedef enum {
    PIC18_IRQ_INT0      = 0,  /**< External INT0 (RB0), always high-prio. */
    PIC18_IRQ_INT1      = 1,  /**< External INT1 (RB1).                    */
    PIC18_IRQ_INT2      = 2,  /**< External INT2 (RB2).                    */
    PIC18_IRQ_RB        = 3,  /**< RB<7:4> change.                         */
    PIC18_IRQ_TMR0      = 4,  /**< Timer0 overflow.                        */
    PIC18_IRQ_TMR1      = 5,  /**< Timer1 overflow (PIR1<TMR1IF>).         */
    PIC18_IRQ_TMR2      = 6,  /**< Timer2 == PR2 match (PIR1<TMR2IF>).     */
    PIC18_IRQ_CCP1      = 7,  /**< CCP1 event (PIR1<CCP1IF>).              */
    PIC18_IRQ_SSP       = 8,  /**< MSSP event (PIR1<SSPIF>).               */
    PIC18_IRQ_USART_TX  = 9,  /**< USART TX shift done (PIR1<TXIF>).       */
    PIC18_IRQ_USART_RX  = 10, /**< USART RX byte ready (PIR1<RCIF>).       */
    PIC18_IRQ_ADC       = 11, /**< A/D conversion done (PIR1<ADIF>).       */
#if PIC18F2455_FAMILY_HAS_SPP
    PIC18_IRQ_SPP       = 12, /**< Streaming Parallel Port (PIR1<SPPIF>).  */
#endif
} PIC18_IRQn;

/* ───────────────────────── enable / disable ─────────────────────── */

/**
 * @brief Globally mask all interrupts by clearing the master enable(s)
 *        (INTCON<GIEH/GIEL>, DS39632E §9.0). In priority mode both
 *        GIEH and GIEL are cleared.
 * @return 1 if any master enable was set (interrupts were on), else 0.
 */
uint8_t HAL_IRQ_Disable(void);

/**
 * @brief Restore the master interrupt enable(s). `prev_state` is the value
 *        returned by @ref HAL_IRQ_Disable. Restoring to "on" also ensures
 *        IPEN = 1 (priority mode) so the two-vector scheme is active. Pair
 *        with @ref HAL_IRQ_Disable. `HAL_IRQ_Restore(1)` enables all
 *        interrupts (the drop-in for PIC16's `GIE = 1`).
 */
void HAL_IRQ_Restore(uint8_t prev_state);

/**
 * @brief Enable one interrupt source. The peripheral enable bit lives in
 *        INTCON / INTCON3 / PIE1 per the source. The master enable(s) must
 *        still be set via @ref HAL_IRQ_Restore for the source to fire.
 */
void HAL_IRQ_Enable(PIC18_IRQn irq);

/** Disable one interrupt source. */
void HAL_IRQ_DisableSrc(PIC18_IRQn irq);

/**
 * @brief Clear the interrupt flag of `irq`. **MUST** be called inside the
 *        ISR before re-enabling interrupts to avoid an infinite re-entry
 *        (DS39632E §9.0).
 */
void HAL_IRQ_ClearFlag(PIC18_IRQn irq);

/** Returns the current pending state of `irq` (1 = pending). */
uint8_t HAL_IRQ_GetFlag(PIC18_IRQn irq);

/**
 * @brief Set the priority of `irq` (high or low vector). Writes the
 *        matching bit in INTCON2 / INTCON3 / IPR1. INT0 has no priority
 *        bit (always high); setting its priority is a no-op. Takes effect
 *        only in priority mode (IPEN = 1, which @ref HAL_IRQ_Restore
 *        enables). Part of the shared `HAL_IRQ_*` contract (the PIC16
 *        implementation is a no-op).
 */
void HAL_IRQ_SetPriority(PIC18_IRQn irq, HAL_IRQ_Priority prio);

#endif /* PIC18_IRQ_H */