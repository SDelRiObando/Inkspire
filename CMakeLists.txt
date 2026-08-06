cmake_minimum_required(VERSION 3.21)
project(inkspire VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable folders in IDEs like VSCode/Visual Studio
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Source files
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
  src/*.cpp
)

# Create executable target first
add_executable(inkspire ${SOURCES})

# Copy assets next to the executable so the app finds them at runtime
add_custom_command(TARGET inkspire POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:inkspire>/assets
)

# Let CMake know where to look for headers (after target creation)
target_include_directories(inkspire PRIVATE ${PROJECT_SOURCE_DIR}/include)

# Enable vcpkg manifest mode
if(DEFINED ENV{VCPKG_ROOT})
  message(STATUS "Using vcpkg from $ENV{VCPKG_ROOT}")
else()
  message(WARNING "VCPKG_ROOT not set; dependencies may not be found")
endif()

# Find and link SDL2 libraries
find_package(SDL2 CONFIG REQUIRED)
find_package(SDL2_image CONFIG REQUIRED)
find_package(SDL2_ttf CONFIG REQUIRED)
find_package(miniz CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(tinyfiledialogs CONFIG REQUIRED)

target_link_libraries(inkspire PRIVATE
  SDL2::SDL2
  SDL2::SDL2main
  SDL2_image::SDL2_image
  SDL2_ttf::SDL2_ttf
  miniz::miniz
  nlohmann_json::nlohmann_json
  tinyfiledialogs::tinyfiledialogs
)
