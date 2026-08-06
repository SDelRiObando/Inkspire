#pragma once

#include <SDL.h>
#include <SDL_ttf.h>
#include <string>
#include "canvas.hpp"
#include "panel.hpp"
#include "scrollstate.hpp"

namespace Inkspire
{
    class LayersPanel : public Panel
    {
    public:
        void init(TTF_Font *font);

        // Panel interface
        const char *id() const override { return "layers"; }
        const char *title() const override { return "Layers"; }
        void render(const PanelContext &ctx) override;
        bool handleEvent(const SDL_Event &e, const PanelContext &ctx) override;
        int preferredWidth() const override { return panelW; }

    private:
        void renderText(SDL_Renderer *renderer, const std::string &text, int x, int y, SDL_Color color);
        // The scrollable region for the layer rows (panel minus the header).
        SDL_Rect rowViewport(const SDL_Rect &b) const
        {
            SDL_Rect v = {b.x, b.y + headerH, b.w, b.h - headerH};
            if (v.h < 0)
                v.h = 0;
            return v;
        }

        TTF_Font *font = nullptr;
        int panelW = 200;
        int rowH = 40;
        int headerH = 32; // "Layers" title + add/delete buttons

        ScrollState scroll; // vertical scrolling of the layer rows

        // Geometry captured during render for hit-testing
        SDL_Rect addRect{0, 0, 0, 0};
        SDL_Rect delRect{0, 0, 0, 0};
        // Per-row rects are recomputed each event from the same layout
    };
}