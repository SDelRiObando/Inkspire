#pragma once

#include <SDL.h>
#include <SDL_ttf.h>
#include <string>
#include "canvas.hpp" // for Tool
#include "panel.hpp"
#include "toolstate.hpp"
#include "scrollstate.hpp"

namespace Inkspire
{
    // Brush settings: the size slider and (brush-only) opacity slider. More
    // controls will live here later. Split out of the old ToolOptionsPanel;
    // shares ToolState with the other tool-option panels.
    class BrushSettingsPanel : public Panel
    {
    public:
        void init(TTF_Font *font, ToolState *state);

        const char *id() const override { return "brushsettings"; }
        const char *title() const override { return "Brush Settings"; }
        const char *category() const override { return "Tool Options"; }
        void render(const PanelContext &ctx) override;
        bool handleEvent(const SDL_Event &e, const PanelContext &ctx) override;
        int preferredWidth() const override { return panelW; }

    private:
        void renderText(SDL_Renderer *renderer, const std::string &text, int x, int y, SDL_Color color);

        TTF_Font *font = nullptr;
        ToolState *state = nullptr;

        int panelW = 180;

        ScrollState scroll; // vertical scroll when content overflows
        int contentH_ = 0;  // total content height, captured each render

        SDL_Rect sliderTrack{0, 0, 0, 0};
        SDL_Rect sliderHit{0, 0, 0, 0};
        bool draggingSlider = false;

        SDL_Rect opacityTrack{0, 0, 0, 0};
        SDL_Rect opacityHit{0, 0, 0, 0};
        bool draggingOpacity = false;
    };
}