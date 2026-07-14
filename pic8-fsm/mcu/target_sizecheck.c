/**
 * @file    target_sizecheck.c
 * @brief   Minimal on-target program exercising fsm.c, used only by the
 *          mcu build (see the Makefiles under mcu/) to prove fsm.c
 *          cross-compiles cleanly with real XC8 for PIC16/PIC18 silicon
 *          and to report flash/RAM footprint. Not a correctness test:
 *          dispatch correctness is fully covered on host by
 *          ../tests/test_fsm.c (there is no per-family variant of fsm.c,
 *          so the host suite already proves the shipped code).
 */

#include "fsm.h"

enum { ST_A, ST_B };
enum { EV_GO };

static const fsm_transition_t transitions[] = {
    { ST_A, EV_GO, NULL, NULL, ST_B },
    { ST_B, EV_GO, NULL, NULL, ST_A },
};

static fsm_t g_fsm;

int main(void)
{
    FSM_INIT(&g_fsm, transitions, ST_A, NULL);
    for (;;) {
        fsm_dispatch(&g_fsm, EV_GO);
    }
}
