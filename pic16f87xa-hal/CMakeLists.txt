# PIC16F87XA HAL — host simulation build.
#
#   cmake -B build && cmake --build build
#   ./build/example_blink
#
# Real PIC target builds use the XC8 toolchain. The MPLAB X project
# lives at mcu/pic16f87xa-mplabx/; its Makefile produces a .hex via xc8-cc.
#
# The host vs. target split is done at build time, not with #ifdef: this
# build puts include/host ahead of include on the include path, so
# pic16f87xa_platform.h resolves to the memory-backed-SFR version, and
# links the host-side harness/wdt_sleep implementations. The XC8 Makefile
# does the mirror with include/target and the target implementations.

cmake_minimum_required(VERSION 3.16)
project(pic16f87xa_hal C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Default device — can be overridden with -DPIC16F87XA_DEVICE=873A.
if(NOT DEFINED PIC16F87XA_DEVICE)
    set(PIC16F87XA_DEVICE "PIC16F877A")
endif()

# The device-select macro is set per-target so each example can pick its
# own part (PIC16F873A / 874A / 876A / 877A) without polluting other targets.
set(PIC16F87XA_DEFAULT_DEVICE PIC16F877A)

# Determine whether the default device has the PSP (40/44-pin only).
if(PIC16F87XA_DEFAULT_DEVICE STREQUAL "PIC16F874A" OR
   PIC16F87XA_DEFAULT_DEVICE STREQUAL "PIC16F877A")
    set(PIC16F87XA_FAMILY_HAS_PSP 1)
else()
    set(PIC16F87XA_FAMILY_HAS_PSP 0)
endif()

set(HAL_INCLUDE_DIR      ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(HAL_HOST_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include/host)

set(HAL_SOURCES_CORE
    src/peripherals/pic16f87xa_gpio.c
    src/peripherals/pic16f87xa_timer0.c
    src/peripherals/pic16f87xa_timer1.c
    src/peripherals/pic16f87xa_timer2.c
    src/peripherals/pic16f87xa_ccp.c
    src/peripherals/pic16f87xa_usart.c
    src/peripherals/pic16f87xa_ssp.c
    src/peripherals/pic16f87xa_adc.c
    src/peripherals/pic16f87xa_comp.c
    src/peripherals/pic16f87xa_vref.c
    src/peripherals/pic16f87xa_eeprom.c
    src/core/pic16f87xa_interrupt.c
    src/core/pic16f87xa_wdt_sleep.c
    src/core/pic16f87xa_wdt_sleep_sim.c
    src/core/pic16f87xa_irq_dispatch.c
    src/core/pic16f87xa_harness_sim.c
    src/sim/pic16f87xa_sim.c
)

# PSP is only on 40/44-pin parts. PIC16F877A is the default.
if(PIC16F87XA_FAMILY_HAS_PSP)
    list(APPEND HAL_SOURCES_CORE src/peripherals/pic16f87xa_psp.c)
endif()

add_library(pic16f87xa_hal STATIC ${HAL_SOURCES_CORE})
# include/host first so pic16f87xa_platform.h resolves to the host version.
target_include_directories(pic16f87xa_hal PUBLIC ${HAL_HOST_INCLUDE_DIR} ${HAL_INCLUDE_DIR})
target_compile_definitions(pic16f87xa_hal PRIVATE -D${PIC16F87XA_DEFAULT_DEVICE})

# Build an example executable against the HAL. Device selection is the only
# per-example compile definition; the host platform is chosen by the
# include path inherited from the library.
function(pic16f87xa_add_example name src)
    add_executable(${name} ${src})
    target_compile_definitions(${name} PRIVATE -D${PIC16F87XA_DEFAULT_DEVICE})
    target_link_libraries(${name} PRIVATE pic16f87xa_hal)
endfunction()

pic16f87xa_add_example(example_blink       tests/example_blink.c)
pic16f87xa_add_example(example_idle_blink  tests/example_idle_blink.c)
pic16f87xa_add_example(example_timer1      tests/example_timer1.c)
pic16f87xa_add_example(example_timer2      tests/example_timer2.c)
pic16f87xa_add_example(example_ccp_pwm     tests/example_ccp_pwm.c)
pic16f87xa_add_example(example_usart       tests/example_usart.c)
pic16f87xa_add_example(example_ssp         tests/example_ssp.c)
pic16f87xa_add_example(example_adc         tests/example_adc.c)
pic16f87xa_add_example(example_comp_vref   tests/example_comp_vref.c)
pic16f87xa_add_example(example_eeprom      tests/example_eeprom.c)
pic16f87xa_add_example(example_wdt_sleep   tests/example_wdt_sleep.c)
if(PIC16F87XA_FAMILY_HAS_PSP)
    pic16f87xa_add_example(example_psp     tests/example_psp.c)
endif()

# Optional: example for every device in the family.
foreach(dev PIC16F873A PIC16F874A PIC16F876A PIC16F877A)
    add_executable(example_blink_${dev} tests/example_blink.c)
    target_compile_definitions(example_blink_${dev} PRIVATE -D${dev})
    target_link_libraries(example_blink_${dev} PRIVATE pic16f87xa_hal)
endforeach()