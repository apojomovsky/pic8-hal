/**
 * @file    host/pic16f87xa_platform.h
 * @brief   Host-simulation platform: how SFRs are stored and how the weak
 *          attribute is spelled, for the CMake host build.
 *
 * @details
 *   This is the host half of the SFR mapping layer. The companion
 *   target/pic16f87xa_platform.h is used by the XC8 build. Which one is
 *   included is decided by the build's include path (CMake puts
 *   include/host first; the XC8 Makefile puts include/target first), so
 *   pic16f87xa.h includes "pic16f87xa_platform.h" unconditionally and
 *   there is no `#ifdef` around code anywhere in the HAL.
 *
 *   On the host every SFR access indexes the 512-byte memory-backed
 *   register file pic16f87xa_sim_sfr[] (defined in
 *   src/sim/pic16f87xa_sim.c), so tests can poke registers directly.
 *   GCC/Clang provides a real weak attribute for optional handler
 *   override.
 */

#ifndef PIC16F87XA_PLATFORM_H
#define PIC16F87XA_PLATFORM_H

#include <stdint.h>

/* 512-byte memory-backed register file (DS39582B Figure 2-3/2-4 layout),
 * defined in src/sim/pic16f87xa_sim.c. */
extern uint8_t pic16f87xa_sim_sfr[0x200];

/* GCC/Clang weak attribute, lets user code override a peripheral's
 * IRQHandler if it ever needs to. */
#define PIC16F87XA_WEAK   __attribute__((weak))

/* SFR access resolves to an index into the simulated register file. */
#define PIC16F87XA_SFR_PTR(addr)       (&pic16f87xa_sim_sfr[(uint16_t)(addr)])
#define pic16f87xa_sfr_read8(addr)     (pic16f87xa_sim_sfr[(uint16_t)(addr)])
#define pic16f87xa_sfr_write8(addr, v) \
    do { pic16f87xa_sim_sfr[(uint16_t)(addr)] = (uint8_t)(v); } while (0)

/* Address of a register as a uint8_t lvalue (read/write/RMW). */
#define PIC16F87XA_REG8(addr)          (pic16f87xa_sim_sfr[(uint16_t)(addr)])

#endif /* PIC16F87XA_PLATFORM_H */