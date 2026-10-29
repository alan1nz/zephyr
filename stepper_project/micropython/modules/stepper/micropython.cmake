# SPDX-License-Identifier: Apache-2.0
#
# MicroPython C module exposing the Zephyr stepper driver API as `stepper`.

add_library(usermod_stepper INTERFACE)

target_sources(usermod_stepper INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/modstepper.c
)

target_include_directories(usermod_stepper INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(usermod INTERFACE usermod_stepper)
