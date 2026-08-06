#pragma once

#include <SDL.h>
#include <SDL_ttf.h>
#include <string>
#include "canvas.hpp" // for Tool
#include "panel.hpp"
#include "toolstate.hpp"

namespace Inkspire
{
    // Tool settings: brush/eraser size (preset dots + slider), opacity (brush
    // only), and the color wheel + swatches (brush only). Formerly the ToolPanel
    // flyout; now a first-class dockable panel sharing ToolState with ToolsPanel.
    class ToolOptionsPanel : public Panel
    {
    public:
        void init(TTF_Font *font, ToolState *state);

        const char *id() const override { return "tooloptions"; }
        const char *title() const override { return "Tool Options"; }
        void render(const PanelContext &ctx) override;
        bool handleEvent(const SDL_Event &e, const PanelContext &ctx) override;
        int preferredWidth() const override { return panelW; }

    private:
        void renderText(SDL_Renderer *renderer, const std::string &text, int x, int y, SDL_Color color);

        TTF_Font *font = nullptr;
        ToolState *state = nullptr;

        int panelW = 180;

        // Preset size dots (the values each dot jumps to)
        static constexpr int presetCount = 24;
        int presets[presetCount] = {1, 2, 3, 4, 5, 6,
                                    7, 8, 10, 12, 15, 18,
                                    20, 25, 30, 35, 40, 45,
                                    50, 60, 70, 80, 90, 100};

        // Geometry captured during render for hit-testing
        SDL_Rect presetRects[presetCount]{};
        SDL_Rect sliderTrack{0, 0, 0, 0};
        SDL_Rect sliderHit{0, 0, 0, 0};
        bool draggingSlider = false;

        SDL_Rect opacityTrack{0, 0, 0, 0};
        SDL_Rect opacityHit{0, 0, 0, 0};
        bool draggingOpacity = false;
    };
}
