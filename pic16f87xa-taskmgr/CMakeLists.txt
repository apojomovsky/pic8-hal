# pic16f87xa-taskmgr — cooperative task manager for the PIC16F87XA HAL.
#
#   cmake -B build && cmake --build build
#   ./build/example_multi_blink
#
# This build reuses the host simulation backend of the pic16f87xa-hal: it
# pulls the HAL in via add_subdirectory() so the static library
# `pic16f87xa_hal` (and its host platform header / harness) is built once and
# linked here. Real-target builds use the XC8 Makefile under mcu/.
#
# The host/target split is done at build time, not with #ifdef — exactly as
# in the HAL: the CMake build puts include/host first on the HAL's include
# path (inherited from the `pic16f87xa_hal` target) so the memory-backed SFR
# platform and the host harness link. No example or scheduler source needs a
# preprocessor branch for either build.

cmake_minimum_required(VERSION 3.16)
project(pic16f87xa_taskmgr C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Path to the HAL sources. Override with -DHAL_DIR=/abs/path if the HAL is
# not the sibling this tree expects.
set(HAL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../pic16f87xa-hal
    CACHE PATH "Path to the pic16f87xa-hal source tree")

# Build the HAL once into a subdirectory of our build tree so we get the
# `pic16f87xa_hal` target (host platform + harness + sim backend) to link
# against. Its own examples build too; they are harmless and prove the HAL
# still compiles from this tree.
if(NOT TARGET pic16f87xa_hal)
    add_subdirectory(${HAL_DIR} ${CMAKE_CURRENT_BINARY_DIR}/hal)
endif()

# The task manager itself — a small static library so other projects can
# link it the same way the example does.
add_library(task_manager STATIC src/task_manager.c)
target_include_directories(task_manager PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)
# The scheduler uses the HAL's Timer0, IRQ and harness APIs, and needs the
# same device selection. The HAL defaults to PIC16F877A when none is set.
target_link_libraries(task_manager PUBLIC pic16f87xa_hal)
target_compile_definitions(task_manager PRIVATE -DPIC16F877A)

# The example: four blinks on PORTB via the scheduler.
add_executable(example_multi_blink examples/example_multi_blink.c)
target_compile_definitions(example_multi_blink PRIVATE -DPIC16F877A)
target_link_libraries(example_multi_blink PRIVATE task_manager)

# Optional: the example for every device in the family. PORTB-only, so it
# runs on all four; the scheduler's TASK_MGR_MAX_TASKS default scales down to
# 6 slots on the 192 B 873A/874A and stays 8 on the 368 B 876A/877A.
foreach(dev PIC16F873A PIC16F874A PIC16F876A PIC16F877A)
    add_executable(example_multi_blink_${dev} examples/example_multi_blink.c)
    target_compile_definitions(example_multi_blink_${dev} PRIVATE -D${dev})
    target_link_libraries(example_multi_blink_${dev} PRIVATE task_manager)
endforeach()