# Inkspire

An open-source digital drawing application built with modern **C++20** and **SDL2**. Inkspire provides a clean, fully dockable panel workspace for sketching and painting, with layers, customizable tools, and an intuitive editing workflow.

Inkspire is cross-platform by design (SDL2). It is currently developed and tested on **Windows**; macOS build instructions are provided but have not yet been verified.

## Features

### Drawing

* Brush and Eraser tools selectable from the **Tools** panel
* Adjustable brush size with preset sizes and a fine-grained slider
* Smooth, round, variable-thickness brush strokes
* True paint-buildup brush opacity — individual strokes remain consistent while overlapping strokes naturally accumulate like real ink

### Colors

* HSV color wheel with a brightness slider, in the **Tool Options** panel
* Saveable color swatches for quick reuse
* Adjustable canvas background color

### Layers

* Dedicated, scrollable **Layers** panel
* Add and delete layers
* Per-layer visibility toggles (click the eye, or press **H** for the active layer)
* Background layer plus stacked drawing layers

### Dockable Panels

Inkspire's interface is built around a flexible docking system:

* **Drag panels by their header** to rearrange the workspace
* **Dock to any edge** of another panel (split) or its center (tab together)
* **Dock to the canvas edges**, which are always available — no side can ever become unreachable
* **Float panels** by dragging them onto the canvas interior; move them freely and re-dock them anytime
* **Tab multiple panels** into a single column and switch between them
* **Collapse a column** to a thin rail with the collapse button, and expand it again
* **Resize** any dock by dragging the dividers between panels

The default panels are **Tools** (tool selection), **Tool Options** (size, opacity, color), and **Layers**.

### Workspace & Navigation

* Fixed-size canvas within a resizable workspace
* Mouse-wheel zoom (zooms toward the cursor) and keyboard zoom shortcuts
* Space-bar panning
* Drawing is preserved while resizing the window
* Fullscreen mode
* Resizable window

### Editing

* Undo and Redo with a configurable history depth
* Clear the active layer

### Projects

* Save and open editable **`.inks`** project files, preserving layers, the active layer, background color, and saved swatches

## Controls

| Action                          | Control                              |
| ------------------------------- | ------------------------------------ |
| Select a tool                   | Click a button in the **Tools** panel |
| Draw                            | Left-click and drag on the canvas    |
| Brush / Eraser                  | **B** / **E**                        |
| Save                            | **Ctrl + S** or **File → Save**      |
| Copy active layer               | **Ctrl + C**                         |
| Paste as new layer              | **Ctrl + V**                         |
| Pan canvas                      | Hold **Space** and drag              |
| Zoom                            | Mouse scroll wheel                   |
| Zoom in / out                   | **Ctrl + =** / **Ctrl + -**          |
| Reset zoom                      | **Ctrl + 0**                         |
| Undo                            | **Ctrl + Z** or **Edit → Undo**      |
| Redo                            | **Ctrl + Y** / **Ctrl + Shift + Z** or **Edit → Redo** |
| Toggle current layer visibility | **H**                                |
| Background color picker         | **G**                                |
| Select layer (1–9)              | **1**–**9**                          |
| Toggle fullscreen               | **F11** or **View → Toggle Fullscreen** |
| Rearrange panels                | Drag a panel by its header           |
| Float a panel                   | Drag its header onto the canvas interior |
| Collapse / expand a column      | Click the column's collapse button   |
| Resize a dock                   | Drag the divider between panels       |
| Scroll a panel                  | Mouse wheel, or drag the scrollbar    |

## Technologies

* Modern **C++20**
* **SDL2**, **SDL2_image**, **SDL2_ttf**
* **miniz**, **nlohmann/json**, **tinyfiledialogs** (project save/load)
* **CMake 3.21+**
* **vcpkg** (Manifest Mode)

## Prerequisites

* A C++20-compatible compiler
* CMake 3.21 or newer
* vcpkg configured in Manifest Mode

Clone vcpkg and set the `VCPKG_ROOT` environment variable to its installation directory. Dependencies are declared in `vcpkg.json` and installed automatically.

On macOS, install the Xcode Command Line Tools:

```bash
xcode-select --install
```

## Building

### Windows

```powershell
cmake --preset win-x86-64-debug
cmake --build build
.\build\src\inkspire.exe
```

### macOS

> Note: macOS support is provided on a best-effort basis and has not yet been verified.

```bash
cmake --preset mac-x86-64-debug
cmake --build build
./build/src/inkspire
```

## Development References

* SDL2 Wiki: https://wiki.libsdl.org/SDL2/
* SDL2_image Documentation: https://wiki.libsdl.org/SDL2_image/
* SDL2_ttf Documentation: https://wiki.libsdl.org/SDL2_ttf/
* vcpkg Manifest Mode: https://learn.microsoft.com/en-us/vcpkg/consume/manifest-mode

## Roadmap

Planned features include:

* Real OS-window floating panels (multi-monitor support)
* Saving and restoring panel layouts
* A settings screen (including configurable undo depth)
* Image export (PNG/JPG)
* Additional drawing tools (fill, shapes, line)
* Layer reordering and transformations
* Selection tools
* Keyboard shortcut customization
* An automated test suite
* Verified macOS builds

## License

Inkspire is released under the [MIT License](LICENSE). You are free to use, modify, and distribute it, subject to the terms in the `LICENSE` file.