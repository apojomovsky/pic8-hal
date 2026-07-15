# pic8-usb, USB CDC-ACM device for PIC18F2455/2550/4455/4550, wrapping the
# vendored M-Stack USB stack (third_party/m-stack).
#
#   cmake -B build && cmake --build build
#   ctest --test-dir build --output-on-failure
#
# Unlike every other pic8-* module, this one has no HAL-family split and no
# host build of the real implementation: pic8_usb.c depends directly on
# M-Stack's usb.h/usb_cdc.h (a real PIC18 SIE peripheral, not something a
# host build links against). The host build here links
# pic8_usb_host_stub.c instead -- a deliberately separate, independent test
# double, not a compile-time swap of the same source (see
# pic8-usb/docs/pic8-usb-plan.md, "Host build story", and
# pic8_usb_host_stub.c's own file header for why).
#
# The real target build goes entirely through the XC8 Makefile under
# mcu/pic18fxx5x-usb-mplabx/ (Phase 3 of the plan, not wired up yet), not
# through this CMakeLists.txt at all.

cmake_minimum_required(VERSION 3.16)
project(pic8_usb C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(pic8_usb_hoststub STATIC src/pic8_usb_host_stub.c)
target_include_directories(pic8_usb_hoststub PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)

enable_testing()
add_executable(test_pic8_usb tests/test_pic8_usb.c)
target_link_libraries(test_pic8_usb PRIVATE pic8_usb_hoststub)
add_test(NAME pic8_usb_tests COMMAND test_pic8_usb)

# ---- example: builds against the host stub for a bounded smoke run; the
# same source is meant to also build against the real target's pic8_usb.c
# via mcu/pic18fxx5x-usb-mplabx/Makefile once that exists (Phase 3) ----
add_executable(example_usb_echo examples/example_usb_echo.c)
target_compile_definitions(example_usb_echo PRIVATE PIC8_USB_EXAMPLE_HOST_ITERS=4)
target_link_libraries(example_usb_echo PRIVATE pic8_usb_hoststub)
add_test(NAME pic8_usb_example_echo COMMAND example_usb_echo)
