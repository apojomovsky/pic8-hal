/**
 * @file    host/pic18_platform.h
 * @brief   Host-simulation platform: how SFRs are stored and how the weak
 *          attribute is spelled, for the CMake host build.
 *
 * @details
 *   This is the host half of the SFR mapping layer. The companion
 *   target/pic18_platform.h is used by the XC8 build. Which one is included
 *   is decided by the build's include path (CMake puts include/host first;
 *   the XC8 Makefile puts include/target first), so pic18f2455.h includes
 *   "pic18_platform.h" unconditionally and there is no `#ifdef` around
 *   code anywhere in the HAL.
 *
 *   On the host every SFR access indexes a memory-backed register file
 *   `pic18_sim_sfr[]` (defined in src/sim/pic18_sim.c), so tests can poke
 *   registers directly. GCC/Clang provides a real weak attribute for
 *   optional handler override.
 *
 *   Phase 1 note: the array is provisionally sized to the full 12-bit
 *   data-memory address space (4096 B) so any address the Phase 2 drivers
 *   use resolves without reallocation. Phase 2 task 2 decides whether the
 *   BSR / Access-Bank scheme is modeled explicitly or kept as this flat
 *   array; either way the macros below stay the same, only the storage
 *   behind them changes.
 */

#ifndef PIC18_PLATFORM_H
#define PIC18_PLATFORM_H

#include <stdint.h>

/* 4096-byte memory-backed register file (DS39632E Figure 5-5 data-memory
 * map footprint), defined in src/sim/pic18_sim.c. Provisional size; see
 * file header. */
extern uint8_t pic18_sim_sfr[0x1000];

/* GCC/Clang weak attribute, lets user code override a peripheral's
 * IRQHandler if it ever needs to. */
#define PIC8_WEAK   __attribute__((weak))

/* SFR access resolves to an index into the simulated register file. */
#define PIC8_SFR_PTR(addr)       (&pic18_sim_sfr[(uint16_t)(addr)])
#define pic8_sfr_read8(addr)     (pic18_sim_sfr[(uint16_t)(addr)])
#define pic8_sfr_write8(addr, v) \
    do { pic18_sim_sfr[(uint16_t)(addr)] = (uint8_t)(v); } while (0)

/* Address of a register as a uint8_t lvalue (read/write/RMW). */
#define PIC8_REG8(addr)          (pic18_sim_sfr[(uint16_t)(addr)])

#endif /* PIC18_PLATFORM_H */