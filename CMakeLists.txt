cmake_minimum_required(VERSION 3.30)

project(inkspire VERSION 0.1.0 LANGUAGES CXX OBJCXX C)

# Set build variables by preset.
set(INKSPIRE_DEPS_DIR "${CMAKE_SOURCE_DIR}/deps")
set(INKSPIRE_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include")
set(INKSPIRE_SOURCE_DIR "${CMAKE_SOURCE_DIR}/src")
set(INKSPIRE_LIBS_DIR "${CMAKE_SOURCE_DIR}/build")

# Set compiler arguments for debug or release mode by preset.
add_compile_options(${COMPILING_ARGS})

# Build the app and unit tests.
add_subdirectory(src)
enable_testing()
add_subdirectory(tests)
