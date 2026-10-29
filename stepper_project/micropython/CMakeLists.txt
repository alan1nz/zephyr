#-------------------------------------------------------------------------------
# Zephyr Example Application (MicroPython)
#
# Copyright (c) 2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0
#
# The application logic lives in Python (src/*.py). This CMake file builds the
# MicroPython Zephyr port as the application, freezes the Python sources into
# the firmware (see manifest.py) and adds the C module that exposes the Zephyr
# stepper driver API to Python (modules/stepper).

cmake_minimum_required(VERSION 3.20.0)

# MicroPython checkout, fetched by west (see west.yml).
set(MICROPY_DIR ${CMAKE_CURRENT_LIST_DIR}/../../modules/lib/micropython
    CACHE PATH "Path to the MicroPython source tree")
set(MICROPY_PORT_DIR ${MICROPY_DIR}/ports/zephyr)

# Devicetree bindings provided by the MicroPython port (e.g. micropython,heap).
list(APPEND DTS_ROOT ${MICROPY_PORT_DIR})

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(app LANGUAGES C)

# C modules compiled into the interpreter.
set(USER_C_MODULES ${CMAKE_CURRENT_LIST_DIR}/modules/stepper)

# MicroPython generates its headers (genhdr/) under CMAKE_BINARY_DIR but only
# adds CMAKE_CURRENT_BINARY_DIR to the include path, which is a different
# directory when the port is built as a subdirectory like this. Add it both for
# the qstr preprocessing step (MICROPY_CPP_INC_EXTRA) and for compilation.
set(MICROPY_CPP_INC_EXTRA ${CMAKE_BINARY_DIR})

add_subdirectory(${MICROPY_PORT_DIR} micropython)

target_include_directories(micropython PRIVATE ${CMAKE_BINARY_DIR})

# The code below locates the git index file for this repository and adds it as a dependency for
# the application VERSION file so that if the repo has a new commit added, even if no files in
# the build have changed, the application version file will be regenerated with the new git commit
find_package(Git QUIET)
if(GIT_FOUND)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} rev-parse --absolute-git-dir
    WORKING_DIRECTORY .
    OUTPUT_VARIABLE application_git_dir
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_STRIP_TRAILING_WHITESPACE
    ERROR_VARIABLE stderr
    RESULT_VARIABLE return_code
  )
  # If there is an error e.g. it is not a git repo, we just silently ignore it and continue
  # without a dependency, this will be the case with freestanding applications.
  if(NOT return_code)
    if(NOT "${stderr}" STREQUAL "")
      message(WARNING "Application build version git rev-parse warned: ${stderr}")
    endif()

    set_property(TARGET app_version_h PROPERTY APP_VERSION_DEPENDS ${application_git_dir}/index)
  endif()
endif()
