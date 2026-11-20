cmake_minimum_required(VERSION 3.20)
project(parser LANGUAGES CXX)

add_library(parser STATIC src/gcode_parser.cpp)
target_include_directories(parser PUBLIC include)
target_compile_features(parser PUBLIC cxx_std_20)

option(PARSER_BUILD_TESTS "Build the parser unit tests" OFF)
if(PARSER_BUILD_TESTS)
  # Either point PARSER_CATCH2_SOURCE at a Catch2 v3 source tree, or make Catch2 v3 findable.
  if(PARSER_CATCH2_SOURCE AND NOT TARGET Catch2::Catch2WithMain)
    add_subdirectory(${PARSER_CATCH2_SOURCE} ${CMAKE_BINARY_DIR}/catch2 EXCLUDE_FROM_ALL)
  endif()
  if(NOT TARGET Catch2::Catch2WithMain)
    find_package(Catch2 3 REQUIRED)
  endif()

  add_executable(parser_tests tests/gcode_parser_test.cpp)
  target_link_libraries(parser_tests PRIVATE parser Catch2::Catch2WithMain)

  include(CTest)
  add_test(NAME parser_tests COMMAND parser_tests)
endif()
