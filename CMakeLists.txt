cmake_minimum_required(VERSION 2.8)
project(klunk)

set(CMAKE_EXPORT_COMPILE_COMMANDS YES)

set(C_DEFINES "")
set(C_OPTIONS "-std=c99 -fPIC")

#set(CMAKE_CC_COMPILER, "/usr/bin/clang")
#set(CMAKE_CXX_COMPILER, "/usr/bin/clang++")
#set(CMAKE_C_LINK_EXECUTABLE, "/usr/bin/clang")

# set(CMAKE_C_FLAGS "${C_OPTIONS} -Os -DNDEBUG ${C_DEFINES} -Wall -Wextra -pedantic")
set(CMAKE_C_FLAGS "${C_OPTIONS} -O0 -g ${C_DEFINES} -Wall -Wextra -pedantic")
# set(CMAKE_EXE_LINKER_FLAGS "-s")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}")

include_directories("include")

file(GLOB klunk_source "src/[^.]*.c")
add_library(klunk_core OBJECT ${klunk_source})
add_library(klunk_static STATIC $<TARGET_OBJECTS:klunk_core>)
add_library(klunk SHARED $<TARGET_OBJECTS:klunk_core>)

add_subdirectory(test)
add_subdirectory(examples/libuv)
