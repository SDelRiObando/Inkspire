#pragma once

#include <SDL.h>
#include <SDL_ttf.h>
#include <string>
#include "inkspire/canvas.hpp" // for Tool
#include "inkspire/colorpicker.hpp"

namespace Inkspire
{
    class ToolPanel
    {
    public:
        void init(TTF_Font *font, int menuBarHeight);

        bool handleEvent(const SDL_Event &e);
        void render(SDL_Renderer *renderer, int windowH);

        Tool tool() const { return currentTool; }
        int brushSize() const { return currentTool == Tool::Brush ? brushSz : eraserSz; }
        int width() const { return panelW; }
        int flyoutWidth() const { return flyoutOpen ? flyoutW : 0; }

        float brushOpacity() const { return opacity; }
        SDL_Color brushColor() const { return picker.color(); }

    private:
        void renderText(SDL_Renderer *renderer, const std::string &text, int x, int y, SDL_Color color);
        void renderFlyout(SDL_Renderer *renderer, int windowH);
        bool handleFlyoutEvent(const SDL_Event &e);

        TTF_Font *font = nullptr;

        ColorPicker picker;

        int panelW = 64;
        int topY = 0;
        int buttonH = 48;

        // Flyout geometry
        int flyoutW = 180;
        int flyoutX = 0; // = panelW
        int flyoutY = 0; // aligned to the selected tool's button

        Tool currentTool = Tool::Brush;
        bool flyoutOpen = true; // open by default for the starting tool

        // Independent sizes per tool
        int brushSz = 8;
        int eraserSz = 20;

        // Preset size dots (the values each dot jumps to)
        static constexpr int presetCount = 24;
        int presets[presetCount] = {1, 2, 3, 4, 5, 6,
                                    7, 8, 10, 12, 15, 18,
                                    20, 25, 30, 35, 40, 45,
                                    50, 60, 70, 80, 90, 100};

        // Geometry captured during render for hit-testing
        SDL_Rect brushRect{0, 0, 0, 0};
        SDL_Rect eraserRect{0, 0, 0, 0};
        SDL_Rect sliderTrack{0, 0, 0, 0};
        SDL_Rect sliderHit{0, 0, 0, 0};
        SDL_Rect presetRects[presetCount]{};
        bool draggingSlider = false;

        float opacity = 1.0f; // 0..1, brush only

        SDL_Rect opacityTrack{0, 0, 0, 0};
        SDL_Rect opacityHit{0, 0, 0, 0};
        bool draggingOpacity = false;

        int &activeSize() { return currentTool == Tool::Brush ? brushSz : eraserSz; }
    };
}