cmake_minimum_required(VERSION 2.8)
project(libuv_service)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

include_directories("include")
include_directories("../include")

file(GLOB libuv_service_src "src/[^.]*.c")
add_executable(libuv_service EXCLUDE_FROM_ALL ${libuv_service_src})
target_link_libraries(libuv_service klunk pthread uv)
