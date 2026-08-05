#pragma once

#include <SDL.h>
#include <SDL_ttf.h>
#include <vector>

namespace Inkspire
{
    class ColorPicker
    {
    public:
        void init(TTF_Font *font);

        // Lay out and draw within the given top-left area; returns the y below the picker
        int render(SDL_Renderer *renderer, int x, int y, int width, bool showSwatches = true, bool showPlus = true);

        // Returns true if the event was consumed
        bool handleEvent(const SDL_Event &e);

        SDL_Color color() const { return current; } // alpha handled by stroke commit now
    private:
        void rebuildWheel(SDL_Renderer *renderer);
        void pickFromWheel(int mx, int my);

        TTF_Font *font = nullptr;

        SDL_Color current = {0, 0, 0, 255};

        // HSV state
        float hue = 0.0f; // 0..360
        float sat = 0.0f; // 0..1
        float val = 0.0f; // 0..1 (starts black)

        // Wheel geometry (set during render)
        SDL_Texture *wheelTex = nullptr;
        int wheelSize = 0; // diameter in px
        SDL_Rect wheelRect{0, 0, 0, 0};
        int wheelBuiltForVal = -1; // cache: rebuild wheel only when value changes

        // Value slider
        SDL_Rect valSlider{0, 0, 0, 0};
        bool draggingVal = false;
        bool draggingWheel = false;

        // Swatches
        std::vector<SDL_Color> swatches;
        std::vector<SDL_Rect> swatchRects;
        SDL_Rect plusRect{0, 0, 0, 0};
    };
}