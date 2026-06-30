# PIC16F87XA HAL — host simulation build.
#
#   cmake -B build && cmake --build build
#   ./build/example_blink
#
# Real PIC target builds use the XC8 toolchain and an MPLAB X project —
# see docs/getting-started.md (TODO) for the recommended layout.

cmake_minimum_required(VERSION 3.16)
project(pic16f87xa_hal C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Default device — can be overridden with -DPIC16F87XA_DEVICE=873A.
if(NOT DEFINED PIC16F87XA_DEVICE)
    set(PIC16F87XA_DEVICE "PIC16F877A")
endif()

set(PIC16F87XA_DEFINES
    -DPIC16F87XA_USE_SIMULATOR
    -DPIC16F87XA_HOST_BUILD
)

# The device-select macro is set per-target so each example can pick its
# own part (PIC16F873A / 874A / 876A / 877A) without polluting other targets.
set(PIC16F87XA_DEFAULT_DEVICE PIC16F877A)

set(HAL_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(pic16f87xa_hal STATIC
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
    src/sim/pic16f87xa_sim.c
)
target_include_directories(pic16f87xa_hal PUBLIC ${HAL_INCLUDE_DIR})
target_compile_options(pic16f87xa_hal PRIVATE ${PIC16F87XA_DEFINES})
target_compile_definitions  (pic16f87xa_hal PRIVATE -D${PIC16F87XA_DEFAULT_DEVICE})

add_executable(example_blink tests/example_blink.c)
target_compile_definitions(example_blink PRIVATE
    -D${PIC16F87XA_DEFAULT_DEVICE}
    -DPIC16F87XA_USE_SIMULATOR
)
target_link_libraries(example_blink PRIVATE pic16f87xa_hal)

add_executable(example_timer1 tests/example_timer1.c)
target_compile_definitions(example_timer1 PRIVATE
    -D${PIC16F87XA_DEFAULT_DEVICE}
    -DPIC16F87XA_USE_SIMULATOR
)
target_link_libraries(example_timer1 PRIVATE pic16f87xa_hal)

add_executable(example_timer2 tests/example_timer2.c)
target_compile_definitions(example_timer2 PRIVATE
    -D${PIC16F87XA_DEFAULT_DEVICE}
    -DPIC16F87XA_USE_SIMULATOR
)
target_link_libraries(example_timer2 PRIVATE pic16f87xa_hal)

add_executable(example_ccp_pwm tests/example_ccp_pwm.c)
target_compile_definitions(example_ccp_pwm PRIVATE
    -D${PIC16F87XA_DEFAULT_DEVICE}
    -DPIC16F87XA_USE_SIMULATOR
)
target_link_libraries(example_ccp_pwm PRIVATE pic16f87xa_hal)

add_executable(example_usart tests/example_usart.c)
target_compile_definitions(example_usart PRIVATE
    -D${PIC16F87XA_DEFAULT_DEVICE}
    -DPIC16F87XA_USE_SIMULATOR
)
target_link_libraries(example_usart PRIVATE pic16f87xa_hal)

add_executable(example_ssp tests/example_ssp.c)
target_compile_definitions(example_ssp PRIVATE
    -D${PIC16F87XA_DEFAULT_DEVICE}
    -DPIC16F87XA_USE_SIMULATOR
)
target_link_libraries(example_ssp PRIVATE pic16f87xa_hal)

add_executable(example_adc tests/example_adc.c)
target_compile_definitions(example_adc PRIVATE
    -D${PIC16F87XA_DEFAULT_DEVICE}
    -DPIC16F87XA_USE_SIMULATOR
)
target_link_libraries(example_adc PRIVATE pic16f87xa_hal)

add_executable(example_comp_vref tests/example_comp_vref.c)
target_compile_definitions(example_comp_vref PRIVATE
    -D${PIC16F87XA_DEFAULT_DEVICE}
    -DPIC16F87XA_USE_SIMULATOR
)
target_link_libraries(example_comp_vref PRIVATE pic16f87xa_hal)

add_executable(example_eeprom tests/example_eeprom.c)
target_compile_definitions(example_eeprom PRIVATE
    -D${PIC16F87XA_DEFAULT_DEVICE}
    -DPIC16F87XA_USE_SIMULATOR
)
target_link_libraries(example_eeprom PRIVATE pic16f87xa_hal)

# Optional: example for every device in the family.
foreach(dev PIC16F873A PIC16F874A PIC16F876A PIC16F877A)
    add_executable(example_blink_${dev} tests/example_blink.c)
    target_compile_definitions(example_blink_${dev} PRIVATE
        -D${dev}
        -DPIC16F87XA_USE_SIMULATOR
    )
    target_link_libraries(example_blink_${dev} PRIVATE pic16f87xa_hal)
endforeach()