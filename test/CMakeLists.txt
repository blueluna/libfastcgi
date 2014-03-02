cmake_minimum_required(VERSION 2.8)
project(klunk_test)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

include_directories("include")
include_directories("../include")

file(GLOB klunk_test_source "src/[^.]*.c")
add_executable(klunk_test ${klunk_test_source})
target_link_libraries(klunk_test klunk)
