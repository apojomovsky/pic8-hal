# PIC18F2455 family HAL, host simulation build.
#
#   cmake -B build -S pic18f2455-hal && cmake --build build
#   ./build/example_smoke
#
# Real PIC target builds use the XC8 toolchain. The MPLAB X project lives
# at mcu/pic18f2455-mplabx/; its Makefile produces a .hex via xc8-cc.
#
# The host vs. target split is done at build time, not with #ifdef: this
# build puts include/host ahead of include on the include path, so
# pic18_platform.h resolves to the memory-backed-SFR version, and links
# the host-side harness implementation. The XC8 Makefile does the mirror
# with include/target and the target implementations.
#
# This file is a thin caller of the shared pic8_family.cmake helpers
# (pic8-common/cmake/). Everything identical between families lives there;
# only what is genuinely PIC18F2455-specific stays here: the source list
# and the device list. (Phase 1: no peripheral sources yet, and no
# optional-peripheral gate; both grow in Phase 2.)

cmake_minimum_required(VERSION 3.16)
project(pic18f2455_hal C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Shared layer: status codes, harness, CMake helpers.
set(PIC8_COMMON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../pic8-common)
include(${PIC8_COMMON_DIR}/cmake/pic8_family.cmake)

# Default device, can be overridden with -DPIC18F2455_DEVICE=... The default
# is PIC18F4550, the family's most-featured (40/44-pin) part.
if(NOT DEFINED PIC18F2455_DEVICE)
    set(PIC18F2455_DEVICE "PIC18F4550")
endif()

set(PIC18F2455_DEFAULT_DEVICE PIC18F4550)

# Variables consumed by the shared pic8_family.cmake helpers.
set(PIC8_HAL_LIB              pic18f2455_hal)
set(PIC8_DEFAULT_DEVICE       ${PIC18F2455_DEFAULT_DEVICE})
set(PIC8_FAMILY_INCLUDE_DIR    ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(PIC8_HOST_INCLUDE_DIR      ${CMAKE_CURRENT_SOURCE_DIR}/include/host)
set(PIC8_COMMON_INCLUDE_DIR    ${PIC8_COMMON_DIR}/include)
set(PIC8_FAMILY_DEVICES        PIC18F2455 PIC18F2550 PIC18F4455 PIC18F4550)

set(PIC8_HAL_SOURCES
    src/sim/pic18_sim.c
    src/core/pic18_harness_sim.c
    src/core/pic18_irq.c
    src/core/pic18_irq_dispatch.c
    src/core/pic18f2455_wdt_sleep.c
    src/core/pic18f2455_wdt_sleep_sim.c
    src/peripherals/pic18f2455_gpio.c
    src/peripherals/pic18f2455_timer0.c
    src/peripherals/pic18f2455_timer1.c
    src/peripherals/pic18f2455_timer2.c
    src/peripherals/pic18f2455_timer3.c
    src/peripherals/pic18f2455_ccp.c
    src/peripherals/pic18f2455_ssp.c
)

pic8_add_hal_library(${PIC8_HAL_LIB})

# Phase 2 MVP examples. example_blink is the canonical Timer0 + GPIO +
# interrupt smoke test; example_smoke remains the bare harness-seam test.
pic8_add_example(example_blink  tests/example_blink.c)
pic8_add_example(example_timer1 tests/example_timer1.c)
pic8_add_example(example_timer2 tests/example_timer2.c)
pic8_add_example(example_timer3 tests/example_timer3.c)
pic8_add_example(example_ccp_pwm tests/example_ccp_pwm.c)
pic8_add_example(example_ssp tests/example_ssp.c)
pic8_add_example(example_smoke tests/example_smoke.c)

# Optional: the blink example for every device in the family, proving every
# part compiles and links against the shared contract.
pic8_add_example_per_device(example_blink tests/example_blink.c)