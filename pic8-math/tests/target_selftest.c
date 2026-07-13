/**
 * @file    target_selftest.c
 * @brief   On-target (Tier-3) self-test: run golden_vectors.h through the
 *          real inline-asm routines on silicon and stream PASS/FAIL over
 *          USART.
 *
 * @details
 *   This is the strongest available proof the shipped object code is
 *   correct on real chips of both families: it builds against each family's
 *          real HAL, exercises every golden vector through the per-family
 *          asm backend (src/pic16/ or src/pic18/), and reports a summary
 *          over the already-proven HAL_USART_* driver -- no new hardware
 *          dependency. It mirrors tests/example_usart.c in both HAL trees.
 *
 *   Phase 0: stub -- just the harness contract, no vectors yet. Phase 5
 *   fills in golden_vectors.h replay + USART PASS/FAIL reporting. On a
 *   real target `pic8_harness_running` always returns 1, so the loop never
 *   exits and the report lines are unreachable (firmware runs forever);
 *   that is the intended target behavior, identical to the HAL examples.
 */

#include "core/pic8_harness.h"
#include "pic_math.h"

int main(void)
{
    pic8_harness_init(0UL);

    /* Phase 5: replay golden_vectors.h through the asm backend here and
     * stream a PASS/FAIL summary over HAL_USART_*. */
    for (uint32_t i = 0; pic8_harness_running(i); i++) {
        pic8_harness_tick();
    }

    return pic8_harness_report(1);
}