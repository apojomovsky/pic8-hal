/**
 * @file    pic18_isr_vector.c
 * @brief   Real-target interrupt-vector entries (XC8 target build only).
 *
 * @details
 *   The PIC18F2455 family has two interrupt vectors: 0008h high-priority
 *   and 0018h low-priority (DS39632E §9.0). On a real XC8 target this file
 *   installs the two `__interrupt(...)` handlers the CPU jumps to. Both
 *   delegate to the shared fan-out @ref pic8_dispatch_all_irqs, which
 *   routes to every peripheral IRQHandler (each handler checks its own
 *   flag, so calling the full dispatch from both vectors is correct; the
 *   hardware already separated high- from low-priority sources by which
 *   vector it took). This is the "both vectors delegate to the shared
 *   dispatch" option noted in docs/multi-family-plan.md Phase 2 task 6.
 *
 *   The syntax is the XC8 v3.x dual-priority/legacy-mode form confirmed
 *   in Phase 1 from the XC8 C Compiler User Guide (DS50002737K §5.9.1.2):
 *   `__interrupt(high_priority)` / `__interrupt(low_priority)`.
 *
 *   This file is built only by the XC8 Makefile (real target). The host
 *   CMake build does NOT compile it, `__interrupt()` is an XC8-specific
 *   attribute and the host has no interrupt vectors, so the selection is
 *   done at build time, with no `#ifdef` here. On the host the harness
 *   registers the same dispatcher as the sim IRQ callback instead.
 */

/* Declared in pic8_harness.h (shared). Declared here as a strong extern
 * prototype instead of including that header, to keep the harness's
 * unused inline (pic8_harness_report) out of this translation unit's
 * warning surface, the same pattern pic16_isr_vector.c uses. */
extern void pic8_dispatch_all_irqs(void);

/**
 * @brief  High-priority interrupt vector (0008h). Delegates to the shared
 *         dispatcher.
 */
void __interrupt(high_priority) PIC18_IRQ_HandlerHigh(void)
{
    pic8_dispatch_all_irqs();
}

/**
 * @brief  Low-priority interrupt vector (0018h). Delegates to the shared
 *         dispatcher.
 */
void __interrupt(low_priority) PIC18_IRQ_HandlerLow(void)
{
    pic8_dispatch_all_irqs();
}