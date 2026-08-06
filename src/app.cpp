#include "inkspire/app.hpp"
#include "inkspire/canvas.hpp"
#include "inkspire/project.hpp"
#include <tinyfiledialogs.h>
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
                     { canvas->reset();
                       toolPanel.resetSwatches();
                       currentFilePath.clear();
                       updateWindowTitle(); });
        file.addItem("Save", [this]()
                     { saveCurrent(); });
        file.addItem("Save As", [this]()
                     { saveAs(); });
        file.addItem("Open", [this]()
                     {
                         const char *filters[1] = { "*.inks" };
                         const char *path = tinyfd_openFileDialog(
                             "Open Project",
                             "",
                             1,
                             filters,
                             "Inkspire Project",
                             0); // 0 = single file
                         if (path)
                         {
                            std::vector<SDL_Color> loadedSwatches;
                            if (loadProject(*canvas, renderer, loadedSwatches, path))
                            {
                                toolPanel.setSwatches(loadedSwatches);
                                currentFilePath = path;
                                updateWindowTitle();
                            }
                            else
                                std::cerr << "Open failed\n";
                         } });
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
        layersPanel.init(font, menuBar.height());

        bgPicker.init(font);

        recenterCanvas();

        updateWindowTitle();
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

    void App::saveAs()
    {
        const char *filters[1] = {"*.inks"};
        const char *path = tinyfd_saveFileDialog(
            "Save Project As", "untitled.inks", 1, filters, "Inkspire Project");
        if (path)
        {
            if (saveProject(*canvas, renderer, toolPanel.getSwatches(), path))
            {
                currentFilePath = path; // this file is now "the current file"
                updateWindowTitle();
            }
            else
                std::cerr << "Save failed\n";
        }
    }

    void App::saveCurrent()
    {
        if (currentFilePath.empty())
        {
            saveAs(); // no file yet — behave like Save As
            return;
        }
        // Quick-save: no dialog, reuse the known path
        if (!saveProject(*canvas, renderer, toolPanel.getSwatches(), currentFilePath))
            std::cerr << "Save failed\n";
    }

    void App::updateWindowTitle()
    {
        std::string title = "Inkspire — ";
        if (currentFilePath.empty())
        {
            title += "Untitled";
        }
        else
        {
            // Extract just the filename from the full path
            size_t slash = currentFilePath.find_last_of("/\\");
            std::string name = (slash == std::string::npos)
                                   ? currentFilePath
                                   : currentFilePath.substr(slash + 1);
            title += name;
        }
        SDL_SetWindowTitle(window, title.c_str());
    }

    void App::recenterCanvas()
    {
        int barH = menuBar.height();
        int leftUsed = toolPanel.width() + toolPanel.flyoutWidth();
        int rightUsed = layersPanel.width();
        int availW = windowW - leftUsed - rightUsed;

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
            {
                running = false;
                continue;
            }

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

            if (layersPanel.handleEvent(e, *canvas, windowW))
                continue;

            if (bgPopupOpen && e.type != SDL_KEYDOWN)
            {
                bgPicker.handleEvent(e);
                canvas->setBackgroundColor(bgPicker.color());
                continue;
            }

            if (e.type == SDL_KEYDOWN)
            {
                SDL_Keycode k = e.key.keysym.sym;
                if (k == SDLK_b)
                    currentTool = Tool::Brush;
                else if (k == SDLK_e)
                    currentTool = Tool::Eraser;
                else if (k == SDLK_F11)
                    toggleFullscreen();
                else if (k == SDLK_g)
                {
                    if (bgPopupOpen)
                        canvas->setBackgroundColor(bgPicker.color());
                    bgPopupOpen = !bgPopupOpen;
                }
                else if (k == SDLK_h)
                    canvas->toggleVisible(canvas->getActiveLayer());
                else if (k >= SDLK_1 && k <= SDLK_9)
                    canvas->setActiveLayer(k - SDLK_1);
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
        layersPanel.render(renderer, *canvas, windowW, windowH);
        menuBar.render(renderer);

        // Background Color Picker
        if (bgPopupOpen)
        {
            int pw = 200, ph = 280;
            int px = windowW / 2 - pw / 2;
            int py = windowH / 2 - ph / 2;
            SDL_SetRenderDrawColor(renderer, 45, 45, 50, 255);
            SDL_Rect panel = {px, py, pw, ph};
            SDL_RenderFillRect(renderer, &panel);
            SDL_SetRenderDrawColor(renderer, 100, 100, 105, 255);
            SDL_RenderDrawRect(renderer, &panel);

            // Title
            SDL_Color white = {230, 230, 230, 255};
            SDL_Surface *surf = TTF_RenderText_Blended(font, "Background Color", white);
            if (surf)
            {
                SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
                SDL_Rect dst = {px + 12, py + 10, surf->w, surf->h};
                SDL_RenderCopy(renderer, tex, nullptr, &dst);
                SDL_DestroyTexture(tex);
                SDL_FreeSurface(surf);
            }

            bgPicker.render(renderer, px + 12, py + 60, pw - 24, true, false);
        }
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
