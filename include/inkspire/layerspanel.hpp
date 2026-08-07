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

        // The add/delete buttons live on the column-provided title header now,
        // not inside the panel body. App draws the header + "Layers" title; these
        // hooks add the +/− buttons at the right and handle their clicks.
        int renderHeaderActions(SDL_Renderer *renderer, const SDL_Rect &headerRect,
                                const PanelContext &ctx) override;
        bool handleHeaderEvent(const SDL_Event &e, const SDL_Rect &headerRect,
                               const PanelContext &ctx) override;

    private:
        void renderText(SDL_Renderer *renderer, const std::string &text, int x, int y, SDL_Color color);

        TTF_Font *font = nullptr;
        int panelW = 200;
        int rowH = 40;

        ScrollState scroll; // vertical scrolling of the layer rows

        // Geometry captured during render/renderHeaderActions for hit-testing.
        SDL_Rect addRect{0, 0, 0, 0};
        SDL_Rect delRect{0, 0, 0, 0};
        // Per-row rects are recomputed each event from the same layout.
    };
}