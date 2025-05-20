cmake_minimum_required(VERSION 3.30)

project(inkspire VERSION 0.1.0 LANGUAGES CXX OBJCXX C)

# Set build variables
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(INKSPIRE_DEPS_DIR "${CMAKE_SOURCE_DIR}/deps")
set(INKSPIRE_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include")
set(INKSPIRE_SOURCE_DIR "${CMAKE_SOURCE_DIR}/src")
set(INKSPIRE_LIBS_DIR "${CMAKE_SOURCE_DIR}/build")

# Set compiler arguments for debug or release mode, checks for bad options.
if (BUILD_MODE STREQUAL "debug")
    add_compile_options(-Wall -Wextra -Wpedantic -Wno-deprecated -g -Og)
elseif (BUILD_MODE STREQUAL "release")
    add_compile_options(-Wall -Wextra -Wpedantic -Wno-deprecated -O3)
else()
    message(FATAL_ERROR "Invalid option ${BUILD_MODE}")
endif()

# Build the app and unit tests.
add_subdirectory(src)
enable_testing()
add_subdirectory(tests)
