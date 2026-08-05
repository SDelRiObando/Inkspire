#include "inkspire/app.hpp"
#include "inkspire/canvas.hpp"
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

        window = SDL_CreateWindow("Inkspire", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                  windowW, windowH, SDL_WINDOW_RESIZABLE);
        if (!window)
        {
            std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << '\n';
            return false;
        }

        SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl"); // <-- add this line

        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
        if (!renderer)
        {
            std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << '\n';
            return false;
        }

        SDL_RendererInfo info;
        SDL_GetRendererInfo(renderer, &info);
        if (!(info.flags & SDL_RENDERER_TARGETTEXTURE))
        {
            std::cerr << "Renderer does not support target textures\n";
            return false;
        }

        // Initialize SDL_ttf and load the font
        if (TTF_Init() != 0)
        {
            std::cerr << "TTF_Init failed: " << TTF_GetError() << '\n';
            return false;
        }

        font = TTF_OpenFont("assets/fonts/DejaVuSans.ttf", 14);
        if (!font)
        {
            std::cerr << "Failed to load font: " << TTF_GetError() << '\n';
            return false;
        }

        canvas = std::make_unique<Canvas>();
        if (!canvas->init(renderer, 800, 600))
        {
            std::cerr << "Canvas init failed\n";
            return false;
        }

        // Build the menu bar
        menuBar.init(font);
        Menu &file = menuBar.addMenu("File");
        file.addItem("New", [this]()
                     { canvas->clear(); });
        file.addItem("Save", []()
                     { std::cout << "TODO: Save\n"; });
        file.addItem("Open", []()
                     { std::cout << "TODO: Open\n"; });
        file.addItem("Exit", [this]()
                     { quit = true; });

        Menu &edit = menuBar.addMenu("Edit");
        edit.addItem("Undo", []()
                     { std::cout << "TODO: Undo\n"; });
        edit.addItem("Redo", []()
                     { std::cout << "TODO: Redo\n"; });
        edit.addItem("Clear", [this]()
                     { canvas->clear(); });

        Menu &view = menuBar.addMenu("View");
        view.addItem("Toggle Fullscreen", [this]()
                     { toggleFullscreen(); });

        toolPanel.init(font, menuBar.height());
        recenterCanvas();
        return true;
    }

    void App::shutdown()
    {
        canvas.reset();

        if (font)
        {
            TTF_CloseFont(font);
            font = nullptr;
        }
        TTF_Quit();

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

    void App::recenterCanvas()
    {
        int barH = menuBar.height();
        int leftUsed = toolPanel.width() + toolPanel.flyoutWidth();
        int availW = windowW - leftUsed;

        // Center if there's room; otherwise pin to the right of the panel/flyout
        if (availW > canvasW)
            canvasX = leftUsed + (availW - canvasW) / 2;
        else
            canvasX = leftUsed;

        int availH = windowH - barH;
        if (availH > canvasH)
            canvasY = barH + (availH - canvasH) / 2;
        else
            canvasY = barH;
    }

    void App::toggleFullscreen()
    {
        fullscreen = !fullscreen;
        SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
        // The resize event that follows will update windowW/H and recenter
    }

    bool App::screenToCanvas(int sx, int sy, int &cx, int &cy) const
    {
        cx = sx - canvasX;
        cy = sy - canvasY;
        return cx >= 0 && cx < canvasW && cy >= 0 && cy < canvasH;
    }

    void App::handle_events(bool &running)
    {
        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
                running = false;

            if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
            {

                windowW = e.window.data1;
                windowH = e.window.data2;
                if (canvas)
                    canvas->preserveAcrossResize();
                recenterCanvas();
                continue;
            }

            // Menu bar gets first look; if it consumes the event, don't draw
            if (menuBar.handleEvent(e))
                continue;

            if (toolPanel.handleEvent(e))
                continue;

            else if (e.type == SDL_KEYDOWN)
            {
                if (e.key.keysym.sym == SDLK_b)
                    currentTool = Tool::Brush;
                else if (e.key.keysym.sym == SDLK_e)
                    currentTool = Tool::Eraser;
                else if (e.key.keysym.sym == SDLK_F11)
                    toggleFullscreen();
            }
            else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
            {
                int cx, cy;
                if (screenToCanvas(e.button.x, e.button.y, cx, cy))
                {
                    drawing = true;
                    lastX = cx;
                    lastY = cy;
                    if (toolPanel.tool() == Tool::Brush)
                    {
                        canvas->beginStroke();
                        strokeActive = true;
                    }
                }
            }
            else if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT)
            {
                if (strokeActive)
                {
                    canvas->commitStroke(toolPanel.brushOpacity());
                    strokeActive = false;
                }
                drawing = false;
            }
            else if (e.type == SDL_MOUSEMOTION && drawing)
            {
                int cx, cy;
                screenToCanvas(e.motion.x, e.motion.y, cx, cy);
                if (toolPanel.tool() == Tool::Brush)
                    canvas->strokeLine(lastX, lastY, cx, cy, toolPanel.brushSize(), toolPanel.brushColor());
                else
                    canvas->drawLine(lastX, lastY, cx, cy, Tool::Eraser, toolPanel.brushSize());
                lastX = cx;
                lastY = cy;
            }
        }
    }

    void App::update()
    {
        // Per-frame time-based state goes here (animations, smooth pan/zoom).
        // Empty for now — all current behavior is event-driven.
    }

    void App::render()
    {
        // Workspace "desk"
        SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255); // #3C3C3C
        SDL_RenderClear(renderer);

        if (canvas)
        {
            SDL_Rect paper = {canvasX, canvasY, canvasW, canvasH};
            canvas->renderCheckerboard(renderer, paper);
            canvas->render(renderer, paper, toolPanel.brushOpacity(), strokeActive);
        }

        toolPanel.render(renderer, windowH);
        menuBar.render(renderer);
        SDL_RenderPresent(renderer);
    }

    int App::run()
    {
        if (!init())
            return 1;

        bool running = true;
        while (running && !quit)
        {
            handle_events(running);
            update();
            render();
            SDL_Delay(16);
        }

        return 0; // ~App() calls shutdown()
    }
}
