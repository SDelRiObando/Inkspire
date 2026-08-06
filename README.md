# Inkspire

An open-source digital drawing application for **macOS** and **Windows**, built with modern **C++20** and **SDL2**. Inkspire provides a clean, panel-based workspace for sketching and painting with support for layers, customizable tools, and an intuitive editing workflow.

## Features

### Drawing

* Brush and Eraser tools selectable from the **Tools** panel
* Adjustable brush size with preset sizes and fine-grained controls
* Smooth, round, variable-thickness brush strokes
* True paint-buildup brush opacity—individual strokes remain consistent while overlapping strokes naturally accumulate like real ink

### Colors

* HSV color wheel with brightness slider
* Saveable color swatches for quick color selection

### Layers

* Dedicated **Layers** panel
* Toggle the active layer's visibility with **H**

### Workspace

* Modern panel-based interface
* **Tools** panel for selecting drawing tools
* **Tool Options** panel for configuring the active tool
* **Layers** panel for managing artwork
* Fixed canvas within a resizable workspace
* Drawing is preserved while resizing the window
* Fullscreen mode

### Editing & Navigation

* Undo and Redo
* Mouse wheel zoom
* Keyboard zoom shortcuts
* Space-bar panning
* Resizable window

## Controls

| Action                          | Control                              |
| ------------------------------- | ------------------------------------ |
| Select a tool                   | Click an icon in the **Tools** panel |
| Draw                            | Left-click and drag on the canvas    |
| Pan canvas                      | Hold **Space** and drag              |
| Zoom                            | Mouse scroll wheel                   |
| Zoom in                         | **Ctrl + +**                         |
| Zoom out                        | **Ctrl + -**                         |
| Undo                            | **Ctrl + Z** or **Edit → Undo**      |
| Redo                            | **Ctrl + Y** or **Edit → Redo**      |
| Toggle current layer visibility | **H**                                |
| Toggle fullscreen               | **F11**                              |

## Technologies

* Modern **C++20**
* **SDL2**
* **SDL2_image**
* **SDL2_ttf**
* **CMake 3.21+**
* **vcpkg** (Manifest Mode)

## Prerequisites

* C++20-compatible compiler
* CMake 3.21 or newer
* vcpkg configured in Manifest Mode

Clone vcpkg and set the `VCPKG_ROOT` environment variable to its installation directory.

On macOS, install the Xcode Command Line Tools:

```bash
xcode-select --install
```

## Building

### macOS

```bash
cmake --preset mac-x86-64-debug
cmake --build build
./build/src/inkspire
```

Run tests:

```bash
ctest --test-dir build -V
```

### Windows

```powershell
cmake --preset win-x86-64-debug
cmake --build build
.\build\src\inkspire.exe
```

Run tests:

```powershell
ctest --test-dir build --timeout 3 -V
```

## Development References

* SDL2 Wiki: https://wiki.libsdl.org/SDL2/
* SDL2_image Documentation: https://wiki.libsdl.org/SDL2_image/
* SDL2_ttf Documentation: https://wiki.libsdl.org/SDL2_ttf/
* vcpkg Manifest Mode: https://learn.microsoft.com/en-us/vcpkg/consume/manifest-mode

## Roadmap

Planned features include:

* Additional drawing tools
* Layer transformations
* Selection tools
* Brush presets
* Keyboard shortcut customization
* More advanced canvas navigation
* Export improvements
* Performance optimizations

## License

This project is open source. See the repository's license for details.
