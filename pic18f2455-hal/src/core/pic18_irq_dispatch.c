/**
 * @file    pic18_irq_dispatch.c
 * @brief   Fan-out from the PIC18 interrupt vectors to every peripheral
 *          IRQHandler. Shared by both builds.
 *
 * @details
 *   The PIC18F2455 family has two interrupt vectors: 0008h high-priority
 *   and 0018h low-priority (DS39632E §9.0). On a real target the XC8
 *   `__interrupt(high_priority)` / `__interrupt(low_priority)` handlers
 *   (added in Phase 2, pic18_isr_vector.c) call this; on the host the
 *   harness registers this as the sim IRQ callback. Both reach the same
 *   dispatch, so there is one source of truth and no duplication.
 *
 *   Phase 1: there are no peripheral IRQHandlers yet (no drivers), so this
 *   is an empty body. It exists so the harness's reference to
 *   @ref pic8_dispatch_all_irqs links against the empty family backend,
 *   proving the contract is family-blind. Phase 2 adds the per-source
 *   handler calls (and decides, per the plan's Phase 2 task 6, whether
 *   both vectors delegate here or each priority level dispatches
 *   separately).
 *
 *   The handler prototypes are not pulled in here yet; Phase 2 declares
 *   them as strong externs (matching pic16_irq_dispatch.c) so the linker
 *   resolves every weak handler.
 */

/**
 * @brief  Fan out to every peripheral IRQHandler for the linked family.
 *         Phase 1: empty. The strong extern prototype is declared in
 *         core/pic8_harness.h (shared), so this file needs no include.
 */
void pic8_dispatch_all_irqs(void)
{
    /* No peripheral handlers in Phase 1. */
}