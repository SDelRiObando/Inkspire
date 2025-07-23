#pragma once

#include <SDL.h>

namespace Inkspire
{

    class Canvas
    {
    public:
        Canvas();
        ~Canvas();

        bool init(SDL_Renderer *renderer, int width, int height);
        void clear(Uint8 r = 255, Uint8 g = 255, Uint8 b = 255, Uint8 a = 255);
        void drawLine(int x1, int y1, int x2, int y2, SDL_Color color = {0, 0, 0, 255});
        void render(SDL_Renderer *renderer);
        void shutdown();

    private:
        SDL_Texture *texture = nullptr;
        int width = 0;
        int height = 0;
        SDL_Renderer *renderer = nullptr;
    };

}