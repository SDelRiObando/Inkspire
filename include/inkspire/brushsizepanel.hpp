#pragma once

#include <SDL.h>
#include <SDL_ttf.h>
#include <string>
#include "panel.hpp"
#include "toolstate.hpp"
#include "scrollstate.hpp"

namespace Inkspire
{
    // Quick brush/eraser size presets: a grid of dots from size 1 to size 100.
    // Clicking one snaps the active tool's size to that value. Split out of the
    // old ToolOptionsPanel; shares ToolState with the other tool-option panels.
    class BrushSizePanel : public Panel
    {
    public:
        void init(TTF_Font *font, ToolState *state);

        const char *id() const override { return "brushsize"; }
        const char *title() const override { return "Brush Size"; }
        const char *category() const override { return "Tool Options"; }
        void render(const PanelContext &ctx) override;
        bool handleEvent(const SDL_Event &e, const PanelContext &ctx) override;
        int preferredWidth() const override { return panelW; }

    private:
        void renderText(SDL_Renderer *renderer, const std::string &text, int x, int y, SDL_Color color);

        TTF_Font *font = nullptr;
        ToolState *state = nullptr;

        int panelW = 180;

        ScrollState scroll; // vertical scroll when content exceeds the panel
        int contentH_ = 0;  // total content height, captured each render

        // Preset size dots (the values each dot jumps to), 1..100.
        static constexpr int presetCount = 24;
        int presets[presetCount] = {1, 2, 3, 4, 5, 6,
                                    7, 8, 10, 12, 15, 18,
                                    20, 25, 30, 35, 40, 45,
                                    50, 60, 70, 80, 90, 100};

        SDL_Rect presetRects[presetCount]{};
    };
}