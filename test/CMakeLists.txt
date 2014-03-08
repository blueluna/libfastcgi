cmake_minimum_required(VERSION 2.8)
project(test)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

include_directories("include")
include_directories("../include")

file(GLOB test_source "src/[^.]*.c")
add_executable(test EXCLUDE_FROM_ALL ${test_source})
target_link_libraries(test klunk)
