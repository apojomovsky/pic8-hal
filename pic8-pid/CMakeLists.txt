# pic8-pid, vendor-agnostic fixed-point PID controller.
#
#   cmake -B build && cmake --build build
#   ctest --test-dir build --output-on-failure
#   ./build/test_pid && ./build/example_pid_setpoint_step
#
# The pid core (src/pid.c) is a fixed-point arithmetic engine on plain
# data. It depends only on pic8-math (for the 16x16->32 signed multiply
# pic_math_mul_s16 the Q8.8 math needs). pic8-math's host build is itself
# HAL-free (see pic8-math/CMakeLists.txt for the reasoning: a stateless
# computation library needs no peripheral model), so this CMake build also
# does NOT add_subdirectory() a HAL -- the family dimension for pic8-pid
# lives entirely in the XC8 Makefiles under mcu/. The public header pid.h
# is dependency-free (only <stdint.h>/<stdbool.h>); only src/pid.c includes
# pic_math.h.

cmake_minimum_required(VERSION 3.16)
project(pic8_pid C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Pull in pic8-math (the host reference build, which is HAL-free).
set(MATH_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../pic8-math
    CACHE PATH "Path to the pic8-math source tree")
if(NOT TARGET pic_math)
    add_subdirectory(${MATH_DIR} ${CMAKE_CURRENT_BINARY_DIR}/math)
endif()

add_library(pid STATIC src/pid.c)
target_include_directories(pid PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(pid PUBLIC pic_math)

# ---- tests: exercise the exact code that ships on-target ----
enable_testing()
add_executable(test_pid tests/test_pid.c)
target_link_libraries(test_pid PRIVATE pid)
add_test(NAME pid_tests COMMAND test_pid)

# ---- example: host-only setpoint-step + manual/auto transfer demo ----
add_executable(example_pid_setpoint_step examples/example_pid_setpoint_step.c)
target_link_libraries(example_pid_setpoint_step PRIVATE pid)
