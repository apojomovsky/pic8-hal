/**
 * @file    core/pic8_irq.h
 * @brief   The family-blind part of the `HAL_IRQ_*` contract: the interrupt
 *          priority enum. Shared by every 8-bit PIC family.
 *
 * @details
 *   The `HAL_IRQ_*` enable/disable/clear/flag helpers are declared in each
 *   family's own `pic*_irq.h` because they take that family's `PIC*_IRQn`
 *   enum (the set of interrupt sources differs per family). The one part
 *   of the contract that is genuinely architecture-blind is the priority
 *   vocabulary: "high" vs "low". It lives here so every family spells it
 *   the same way.
 *
 *   Why this exists: PIC18 has two interrupt vectors (0008h high, 0018h
 *   low) with per-source priority bits (DS39632E §9.0); PIC16 has a single
 *   vector and no priority (DS39582B §14.11). The priority contract was
 *   extended (per docs/multi-family-plan.md, Phase 2 task 5) by adding
 *   `HAL_IRQ_SetPriority(irq, prio)` as a separate setter rather than an
 *   optional parameter, so `HAL_IRQ_Enable` keeps its existing signature
 *   and every existing PIC16 caller (the task manager, all examples) needs
 *   zero changes. PIC16's `HAL_IRQ_SetPriority` is a no-op; PIC18's writes
 *   the matching IPR / INTCON2 / INTCON3 priority bit.
 *
 *   `HAL_IRQ_SetPriority` itself is declared in each family's `pic*_irq.h`
 *   (its `irq` parameter is the family's `PIC*_IRQn` type), not here; this
 *   header only owns the shared priority enum it takes.
 */

#ifndef PIC8_IRQ_H
#define PIC8_IRQ_H

/**
 * @brief   Interrupt priority level. Shared vocabulary; the effect is
 *          family-specific (PIC16 ignores it, PIC18 routes the source to
 *          the high or low vector via its IPR bits).
 */
typedef enum {
    HAL_IRQ_PRIORITY_LOW  = 0,  /**< Low-priority vector (PIC18 0018h). */
    HAL_IRQ_PRIORITY_HIGH = 1   /**< High-priority vector (PIC18 0008h). */
} HAL_IRQ_Priority;

#endif /* PIC8_IRQ_H */