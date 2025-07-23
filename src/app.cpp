#include "inkspire/app.hpp"
#include "inkspire/canvas.hpp" // assuming you will create this
#include <iostream>

namespace Inkspire
{
    App::App() = default;

    App::~App()
    {
        shutdown();
    }

    bool App::init()
    {
        if (SDL_Init(SDL_INIT_VIDEO) != 0)
        {
            std::cerr << "SDL_Init failed: " << SDL_GetError() << '\n';
            return false;
        }

        window = SDL_CreateWindow("Inkspire", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, 0);
        if (!window)
        {
            std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << '\n';
            return false;
        }

        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        if (!renderer)
        {
            std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << '\n';
            return false;
        }

        canvas = new Canvas();
        if (!canvas->init(renderer, 800, 600))
        {
            std::cerr << "Canvas init failed\n";
            return false;
        }

        return true;
    }

    void App::shutdown()
    {
        if (canvas)
        {
            canvas->shutdown();
            delete canvas;
            canvas = nullptr;
        }

        if (renderer)
        {
            SDL_DestroyRenderer(renderer);
            renderer = nullptr;
        }

        if (window)
        {
            SDL_DestroyWindow(window);
            window = nullptr;
        }

        SDL_Quit();
    }

    void App::handle_events(bool &running)
    {
        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
                running = false;

            else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
            {
                drawing = true;
                lastX = e.button.x;
                lastY = e.button.y;
            }
            else if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT)
            {
                drawing = false;
            }
            else if (e.type == SDL_MOUSEMOTION && drawing)
            {
                int x = e.motion.x;
                int y = e.motion.y;
                canvas->drawLine(lastX, lastY, x, y);
                lastX = x;
                lastY = y;
            }
        }
    }

    void App::update()
    {
        // TODO: Input/Brush logic
    }

    void App::render()
    {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);

        if (canvas)
            canvas->render(renderer);

        SDL_RenderPresent(renderer);
    }

    int App::run()
    {
        if (!init())
            return 1;

        bool running = true;
        while (running)
        {
            handle_events(running);
            update();
            render();
            SDL_Delay(16); // ~60fps
        }

        shutdown();
        return 0;
    }
}
