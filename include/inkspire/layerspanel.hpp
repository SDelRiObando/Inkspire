#pragma once

#include <SDL.h>
#include <SDL_ttf.h>
#include <string>
#include "canvas.hpp"

namespace Inkspire
{
    class LayersPanel
    {
    public:
        void init(TTF_Font *font, int menuBarHeight);

        // Needs the canvas to read/modify layers. Returns true if event consumed.
        bool handleEvent(const SDL_Event &e, Canvas &canvas, int windowW);
        void render(SDL_Renderer *renderer, Canvas &canvas, int windowW, int windowH);

        int width() const { return panelW; }

    private:
        void renderText(SDL_Renderer *renderer, const std::string &text, int x, int y, SDL_Color color);

        TTF_Font *font = nullptr;
        int panelW = 200;
        int topY = 0;
        int rowH = 40;
        int headerH = 32; // "Layers" title + add/delete buttons

        // Geometry captured during render for hit-testing
        SDL_Rect addRect{0, 0, 0, 0};
        SDL_Rect delRect{0, 0, 0, 0};
        // Per-row rects are recomputed each event from the same layout
    };
}