# Inkspire
An open-source drawing application

## Tools & Dependencies
 - A modern C++ compiler
 - clangd LSP
 - CMake 3.30+ (or latest)
 - OpenGL version ?
 - GLFW 3.3
 - ImGui v1.91.9

## Dev References
 - [learncpp.com website](https://www.learncpp.com/)
 - [CMake Tutorial](https://hsf-training.github.io/hsf-training-cmake-webpage/02-building/index.html)
 - [GLFW prepared downloads](https://www.glfw.org/download)
 - [LearnOpenGL website](https://learnopengl.com/)
 - [CMake With ImGui article](https://technotes.blog/2023/05/12/integrating-imgui-with-cmake-a-step-by-step-guide-with-code-examples/)

## Project Setup:

### File Management:
 - Clone the repo with Git.
 - Create the `build/` and `deps/` folders in the project root.
    - Go to ImGui releases for the zip or tar.gz of v1.91.9, then unzip and drag it under `deps/`.
    - Go to GLFW releases for pre-compiled-binaries of v3.4, then place it unzipped into `deps/`.
    - Ensure the `deps/glad` directory has its files within sub-paths `glad/glad.h` and `KHR/khrplatform.h`, and it finally must have `glad.c` under `deps/glad`.

### Building on macOS (x86-64):
 - Permit and run the build script: enter `chmod +x ./utility.sh` before `./utility.sh build debug`.
 - Test run the app with `./build/src/inkspire`.
 - Optionally, run all tests with `./utility.sh test` from the _project root_.

### Building on Windows (x86-64):
 - Run these commands:
   1. `cmake --fresh -S . -B build -DBUILD_MODE:STRING=debug`
   2. `cmake --build build`
 - Test run the executable at `./build/src/inkspire.exe`.
 - Optionally, run all tests with `ctest --test-dir build --timeout 3 -V` from the _project root_.
