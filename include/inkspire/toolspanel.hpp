#pragma once

#include <SDL.h>
#include <SDL_ttf.h>
#include <string>
#include "canvas.hpp" // for Tool
#include "panel.hpp"
#include "toolstate.hpp"

namespace Inkspire
{
    // The tool button strip (Brush / Eraser). A thin dockable panel; the actual
    // tool settings live in ToolOptionsPanel. Both share a ToolState.
    class ToolsPanel : public Panel
    {
    public:
        void init(TTF_Font *font, ToolState *state);

        const char *id() const override { return "tools"; }
        const char *title() const override { return "Tools"; }
        void render(const PanelContext &ctx) override;
        bool handleEvent(const SDL_Event &e, const PanelContext &ctx) override;
        int preferredWidth() const override { return panelW; }

        int width() const { return panelW; }

    private:
        void renderText(SDL_Renderer *renderer, const std::string &text, int x, int y, SDL_Color color);

        TTF_Font *font = nullptr;
        ToolState *state = nullptr;

        int panelW = 64;
        int buttonH = 48;

        SDL_Rect brushRect{0, 0, 0, 0};
        SDL_Rect eraserRect{0, 0, 0, 0};
    };
}