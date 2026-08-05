#pragma once

#include <SDL.h>

namespace Inkspire
{
    enum class Tool
    {
        Brush,
        Eraser
    };

    class Canvas
    {
    public:
        Canvas();
        ~Canvas();

        bool init(SDL_Renderer *renderer, int width, int height);
        void clear();

        // Brush stroke lifecycle (scratch-buffer for true opacity buildup)
        void beginStroke();
        void strokeLine(int x1, int y1, int x2, int y2, int size, SDL_Color color);
        void commitStroke(float opacity);

        // Eraser stays direct
        void drawLine(int x1, int y1, int x2, int y2, Tool tool, int size, SDL_Color color = {0, 0, 0, 255});

        void renderCheckerboard(SDL_Renderer *renderer, const SDL_Rect &dst);
        void render(SDL_Renderer *renderer, const SDL_Rect &dst, float strokeOpacity, bool strokeActive);
        void shutdown();

        void preserveAcrossResize();

    private:
        bool createTexture();
        bool createStrokeTexture();
        void fillCircle(int cx, int cy, int radius);

        SDL_Texture *texture = nullptr;
        SDL_Texture *strokeTex = nullptr; // scratch buffer for the active stroke
        int width = 0;
        int height = 0;
        SDL_Renderer *renderer = nullptr;
    };
}