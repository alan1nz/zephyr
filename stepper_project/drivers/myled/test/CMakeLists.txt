cmake_minimum_required(VERSION 3.28.3)


set(GTEST_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../../modules/thirdparty/gtest)


add_subdirectory(${GTEST_DIR} ${CMAKE_BINARY_DIR}/googletest-build)

add_executable(myled_test testmyLEDCore.cpp ../myled_core.c)


