#pragma once
#include "canvas.hpp" // Include your Canvas header
#include <SDL.h>

namespace Inkspire
{
    class App
    {
    public:
        App();
        ~App();

        int run();

    private:
        bool init();
        void shutdown();
        void handle_events(bool &running);
        void update();
        void render();

        SDL_Window *window = nullptr;
        SDL_Renderer *renderer = nullptr;

        Canvas *canvas = nullptr;

        bool drawing = false;
        int lastX = 0, lastY = 0;
    };
}
