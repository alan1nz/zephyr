# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(blink_unit_testing LANGUAGES C CXX)

# Skip Catch2's own self-tests/docs/examples when built as a subdirectory.
set(CATCH_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(CATCH_INSTALL_DOCS OFF CACHE BOOL "" FORCE)
set(CATCH_INSTALL_EXTRAS OFF CACHE BOOL "" FORCE)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../../../../modules/thirdparty/catch2 ${CMAKE_BINARY_DIR}/catch2)

target_sources(app PRIVATE src/main.cpp)

# Link against Catch2, not Catch2WithMain: we supply our own main() so it can
# explicitly exit the process once the session finishes.
target_link_libraries(app PRIVATE Catch2::Catch2)
