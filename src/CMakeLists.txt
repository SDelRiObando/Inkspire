# Define path variables for dependencies: ImGui, GLFW, GLAD
set(IMGUI_ROOT_DIR "${INKSPIRE_DEPS_DIR}/imgui-1.91.9")
set(IMGUI_SOURCES ${IMGUI_ROOT_DIR}/imgui.cpp ${IMGUI_ROOT_DIR}/imgui_draw.cpp ${IMGUI_ROOT_DIR}/imgui_demo.cpp ${IMGUI_ROOT_DIR}/imgui_widgets.cpp ${IMGUI_ROOT_DIR}/imgui_tables.cpp ${IMGUI_ROOT_DIR}/misc/cpp/imgui_stdlib.cpp ${IMGUI_ROOT_DIR}/backends/imgui_impl_glfw.cpp ${IMGUI_ROOT_DIR}/backends/imgui_impl_opengl3.cpp)

set(GLFW_ROOT_DIR "${INKSPIRE_DEPS_DIR}/${PLATFORM_GLFW_FOLDER}")
set(GLFW_INC_DIR "${GLFW_ROOT_DIR}/include")
set(GLFW_LIB_DIR "${GLFW_ROOT_DIR}/lib-x86_64")

set(GLAD_ROOT_DIR "${INKSPIRE_DEPS_DIR}/glad")
set(GLAD_INC_DIR "${GLAD_ROOT_DIR}")

add_library(glad "")
target_include_directories(glad PUBLIC ${GLAD_INC_DIR})
target_sources(glad PRIVATE "${GLAD_ROOT_DIR}/glad.c")

# Compile app subdirectories of code.
# add_subdirectory(some_folder)

# Define and build Inkspire.
add_executable(inkspire "")
target_include_directories(inkspire PUBLIC ${INKSPIRE_INCLUDE_DIR} PRIVATE ${GLFW_INC_DIR} PRIVATE ${IMGUI_ROOT_DIR} PRIVATE ${GLAD_INC_DIR})
target_link_directories(inkspire PRIVATE ${INKSPIRE_LIBS_DIR} PRIVATE ${GLFW_LIB_DIR})

# Uses tips from https://www.scivision.dev/cmake-macos-framework-link/
if (NOT ${IS_ON_WINDOWS})
    if (CMAKE_OBJCXX_LINK_LIBRARY_USING_FRAMEWORK_SUPPORTED)
        target_link_libraries(inkspire
            PRIVATE glfw3
            PRIVATE "$<LINK_LIBRARY:FRAMEWORK,Cocoa>"
            PRIVATE "$<LINK_LIBRARY:FRAMEWORK,IOKit>"
            PRIVATE glad
        )
    else ()
        message(FATAL_ERROR "Could not find macOS objcxx frameworks e.g Cocoa, IOKit!")
    endif ()
elseif (${IS_ON_WINDOWS})
    # TODO: place any special windows DLLs or Frameworks to link here: replace '...'
    # target_link_libraries(inkspire ...)
    message (FATAL_ERROR "TODO- Windows build not implemented!")
else ()
    message(FATAL_ERROR "Invalid preset")
endif ()

target_sources(inkspire PRIVATE inkspire.cpp PRIVATE ${IMGUI_SOURCES})
