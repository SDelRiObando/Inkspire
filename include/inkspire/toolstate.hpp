#pragma once

#include <SDL.h>
#include <SDL_ttf.h>
#include <vector>
#include "canvas.hpp" // for Tool
#include "colorpicker.hpp"

namespace Inkspire
{
    // Shared tool state, owned by App and referenced by both the ToolsPanel
    // (brush/eraser buttons) and the ToolOptionsPanel (size / opacity / color).
    // Splitting the old ToolPanel into two dockable panels means their common
    // state can't live in either one — it lives here so both, and App, can read
    // and mutate it regardless of where each panel is docked.
    class ToolState
    {
    public:
        void init(TTF_Font *font) { picker.init(font); }

        Tool tool() const { return currentTool; }
        void setTool(Tool t) { currentTool = t; }

        // Active brush size depends on the current tool (independent per tool).
        int brushSize() const { return currentTool == Tool::Brush ? brushSz : eraserSz; }
        int &activeSize() { return currentTool == Tool::Brush ? brushSz : eraserSz; }

        float brushOpacity() const { return opacity; }
        void setOpacity(float o) { opacity = o; }

        SDL_Color brushColor() const { return picker.color(); }

        const std::vector<SDL_Color> &getSwatches() const { return picker.getSwatches(); }
        void setSwatches(const std::vector<SDL_Color> &s) { picker.setSwatches(s); }
        void resetSwatches() { picker.resetSwatches(); }

        ColorPicker &colorPicker() { return picker; }

    private:
        Tool currentTool = Tool::Brush;
        int brushSz = 8;
        int eraserSz = 20;
        float opacity = 1.0f; // 0..1, brush only
        ColorPicker picker;
    };
}
