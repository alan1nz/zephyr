# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(blink_catch2 LANGUAGES C CXX)

# Skip Catch2's own self-tests/docs/examples when built as a subdirectory.
set(CATCH_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(CATCH_INSTALL_DOCS OFF CACHE BOOL "" FORCE)
set(CATCH_INSTALL_EXTRAS OFF CACHE BOOL "" FORCE)

add_subdirectory(${WEST_TOPDIR}/modules/thirdparty/catch2 ${CMAKE_BINARY_DIR}/catch2)

# Catch2 doesn't go through zephyr_library(), so it won't automatically pick
# up Zephyr's toolchain/target flags (cross-compiler, arch flags, defines).
target_link_libraries(Catch2 PRIVATE zephyr_interface)

target_sources(app PRIVATE src/main.cpp)

target_compile_options(app PRIVATE
    -Wno-error=unused-const-variable
    -Wno-error=unused-variable
)

# Link against Catch2, not Catch2WithMain: we supply our own main() so it can
# explicitly exit the process once the session finishes (see main.cpp).
target_link_libraries(app PRIVATE Catch2::Catch2)
