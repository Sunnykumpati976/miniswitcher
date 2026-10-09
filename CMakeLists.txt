cmake_minimum_required(VERSION 3.16)
project(miniswitcher LANGUAGES C CXX)

# Default to an optimised build with symbols: the benchmark numbers in the
# README are meaningless from a -O0 build, and a debug build of a real-time
# pipeline misses every deadline.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "" FORCE)
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wconversion)

option(MS_SANITIZE "Build with address and undefined-behaviour sanitizers" OFF)
if(MS_SANITIZE)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=address,undefined)
endif()

enable_testing()

add_subdirectory(libframe)
add_subdirectory(core)
add_subdirectory(tools)
add_subdirectory(tests)
