/**
 * @file    example_psp.c
 * @brief   Parallel Slave Port driver smoke test.
 *
 *   Verifies that HAL_PSP_Enable() sets TRISE<PSPMODE> and that
 *   the buffer flag helpers return 0 before any external transfer.
 *
 *   Note: requires a 40/44-pin part (compile-time check in the
 *   driver).  The example_psp target is only built for those parts.
 */

#include "pic16f87xa.h"
#include "pic16f87xa_sim.h"
#include "pic16f87xa_sfr.h"
#include "peripherals/pic16f87xa_psp.h"
#include <stdio.h>

#define CHECK(cond, msg) do { \
    if (!(cond)) { printf("FAIL: %s\n", msg); return 1; } \
} while (0)

int main(void)
{
    pic16f87xa_sim_reset();

    /* TRISE reset value: 0x07 = --0 0111 (PSPIE=1, IBF=1, OBF=1, IBOV=0,
     * PSPMODE=0). After our reset clears, expect 0x06 (PSPIE=0). */
    HAL_PSP_Init(NULL);

    /* Enable PSP. */
    HAL_PSP_Enable();

    /* Read TRISE. */
    uint8_t prev = (PIC8_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
    pic_select_bank(1);
    uint8_t trise = PIC8_REG8(0x89U);
    pic_select_bank(prev);
    /* Bit 4 (PSPMODE) should be set. */
    CHECK((trise & 0x10U) != 0U, "PSPMODE not set after HAL_PSP_Enable");

    /* Buffer flags: sim_reset cleared them, no I/O has happened. */
    CHECK(HAL_PSP_IsInputBufferFull()  == 0U, "IBF set before any input");
    CHECK(HAL_PSP_IsOutputBufferFull() == 0U, "OBF set before any output");
    CHECK(HAL_PSP_HasInputOverflow()   == 0U, "IBOV set before any input");

    /* Disable. */
    HAL_PSP_Disable();
    prev = (PIC8_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
    pic_select_bank(1);
    trise = PIC8_REG8(0x89U);
    pic_select_bank(prev);
    CHECK((trise & 0x10U) == 0U, "PSPMODE not cleared after HAL_PSP_Disable");

    HAL_PSP_DeInit();
    prev = (PIC8_REG8(PIC_REG_STATUS) >> 5) & 0x03U;
    pic_select_bank(1);
    trise = PIC8_REG8(0x89U);
    pic_select_bank(prev);
    /* After DeInit TRISE = 0x07 (POR default). */
    CHECK(trise == 0x07U, "TRISE not 0x07 after DeInit");

    printf("OK: PSP driver, enable/disable, buffer flags, TRISE state all pass.\n");
    return 0;
}