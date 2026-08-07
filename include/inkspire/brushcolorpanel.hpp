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
    // The HSV color wheel + brightness slider (brush only). Split out of the old
    // ToolOptionsPanel; the swatch palette lives in its own ColorPalettePanel.
    // Both drive the same ColorPicker inside the shared ToolState.
    class BrushColorPanel : public Panel
    {
    public:
        void init(TTF_Font *font, ToolState *state);

        const char *id() const override { return "brushcolor"; }
        const char *title() const override { return "Color Wheel"; }
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
    };
}