# Inkspire
An open-source drawing application for macOS and Windows.

## Features
 - Brush and eraser tools with adjustable size (preset sizes + fine slider)
 - Round, variable-thickness strokes
 - HSV color wheel with a brightness slider
 - Saveable color swatches for quick reuse
 - True paint-buildup brush opacity — a single stroke stays even, while
   overlapping strokes accumulate like real ink
 - Fixed canvas in a workspace, with a full-height tool panel
 - Resizable window and fullscreen mode (F11)
 - Drawing is preserved across window resizing

## Controls
 - **B** — brush, **E** — eraser
 - **F11** — toggle fullscreen
 - Click and drag on the canvas to draw

## Tools & Dependencies
 - A modern C++ compiler (supports **C++20**)
 - CMake 3.21+
 - [vcpkg](https://github.com/microsoft/vcpkg) (manifest mode)
 - SDL2, SDL2_image, SDL2_ttf (installed automatically via vcpkg)

## Dev References
 - [SDL2 Wiki](https://wiki.libsdl.org/SDL2/)
 - [SDL_image docs](https://wiki.libsdl.org/SDL2_image/)
 - [SDL_ttf docs](https://wiki.libsdl.org/SDL2_ttf/)
 - [vcpkg manifest mode](https://learn.microsoft.com/en-us/vcpkg/consume/manifest-mode)

## Project Setup

### Prerequisites
 - Clone vcpkg and set the `VCPKG_ROOT` environment variable to its path.
 - macOS: install the Xcode command-line tools with `xcode-select --install`.

### Building on macOS
 - `cmake --preset mac-x86-64-debug` (or `mac-x86-64-release`)
 - `cmake --build build`
 - Run with `./build/src/inkspire`.
 - Tests: `ctest --test-dir build -V`

### Building on Windows
 - `cmake --preset win-x86-64-debug` (or `win-x86-64-release`)
 - `cmake --build build`
 - Run with `./build/src/inkspire.exe`.
 - Tests: `ctest --test-dir build --timeout 3 -V`