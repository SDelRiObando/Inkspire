#include "inkspire/app.hpp"
#include "inkspire/canvas.hpp"
#include "inkspire/project.hpp"
#include <tinyfiledialogs.h>
#include <iostream>
#include <algorithm>

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

        // Crisp, blocky pixels when zoomed in (nearest-neighbor sampling), the
        // expected default for a drawing app. Must be set before textures are
        // created so they inherit it.
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

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
        edit.addItem("Undo", [this]()
                     { canvas->undo(); });
        edit.addItem("Redo", [this]()
                     { canvas->redo(); });
        edit.addItem("Clear", [this]()
                     { canvas->pushUndo();
                       canvas->clear(); });

        Menu &view = menuBar.addMenu("View");
        view.addItem("Zoom In", [this]()
                     { zoomInAtCenter(); });
        view.addItem("Zoom Out", [this]()
                     { zoomOutAtCenter(); });
        view.addItem("Reset Zoom", [this]()
                     { resetZoom(); });
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

    int App::scaledCanvasW() const { return (int)(canvasW * zoom + 0.5f); }
    int App::scaledCanvasH() const { return (int)(canvasH * zoom + 0.5f); }

    void App::recenterCanvas()
    {
        int barH = menuBar.height();
        int leftUsed = toolPanel.width() + toolPanel.flyoutWidth();
        int rightUsed = layersPanel.width();
        int availW = windowW - leftUsed - rightUsed;

        int sw = scaledCanvasW();
        int sh = scaledCanvasH();

        if (availW > sw)
            canvasX = leftUsed + (availW - sw) / 2;
        else
            canvasX = leftUsed;

        int availH = windowH - barH;
        if (availH > sh)
            canvasY = barH + (availH - sh) / 2;
        else
            canvasY = barH;
    }

    void App::zoomAt(int screenPivotX, int screenPivotY, float newZoom)
    {
        newZoom = std::clamp(newZoom, MIN_ZOOM, MAX_ZOOM);
        if (newZoom == zoom)
            return;

        // Keep the document point currently under (screenPivotX, screenPivotY)
        // under that same screen point after the zoom changes.
        float ratio = newZoom / zoom;
        canvasX = (int)(screenPivotX - (screenPivotX - canvasX) * ratio + 0.5f);
        canvasY = (int)(screenPivotY - (screenPivotY - canvasY) * ratio + 0.5f);
        zoom = newZoom;
    }

    void App::zoomInAtCenter() { zoomAt(windowW / 2, windowH / 2, zoom * 1.25f); }
    void App::zoomOutAtCenter() { zoomAt(windowW / 2, windowH / 2, zoom / 1.25f); }

    void App::resetZoom()
    {
        zoom = 1.0f;
        recenterCanvas();
    }

    void App::toggleFullscreen()
    {
        fullscreen = !fullscreen;
        SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
        // The resize event that follows will update windowW/H and recenter
    }

    bool App::screenToCanvas(int sx, int sy, int &cx, int &cy) const
    {
        // Screen → document: undo the pan offset, then undo the zoom scale.
        cx = (int)((sx - canvasX) / zoom);
        cy = (int)((sy - canvasY) / zoom);
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
                // Snapshot once, on the mouse-down that starts the interaction,
                // so a click-drag across the value slider is a single undo entry
                // rather than one per motion event.
                if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
                    canvas->pushUndo();
                bgPicker.handleEvent(e);
                canvas->setBackgroundColor(bgPicker.color());
                continue;
            }

            // ---- View navigation: wheel zoom + space-drag pan ----
            // These are handled ahead of the drawing logic so a pan gesture
            // never leaks through as a stroke.
            if (e.type == SDL_MOUSEWHEEL)
            {
                int mx, my;
                SDL_GetMouseState(&mx, &my);
                float step = (e.wheel.y > 0) ? 1.25f : (1.0f / 1.25f);
                if (e.wheel.y != 0)
                    zoomAt(mx, my, zoom * step); // zoom toward the cursor
                continue;
            }

            if (spaceDown && e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
            {
                panning = true;
                panLastX = e.button.x;
                panLastY = e.button.y;
                continue;
            }
            if (panning && e.type == SDL_MOUSEMOTION)
            {
                canvasX += e.motion.x - panLastX;
                canvasY += e.motion.y - panLastY;
                panLastX = e.motion.x;
                panLastY = e.motion.y;
                continue;
            }
            if (panning && e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT)
            {
                panning = false;
                continue;
            }

            if (e.type == SDL_KEYDOWN)
            {
                SDL_Keycode k = e.key.keysym.sym;
                bool ctrl = (e.key.keysym.mod & KMOD_CTRL) != 0;
                bool shift = (e.key.keysym.mod & KMOD_SHIFT) != 0;

                // Ctrl+Z = undo, Ctrl+Y or Ctrl+Shift+Z = redo
                if (ctrl && k == SDLK_z && !shift)
                    canvas->undo();
                else if (ctrl && (k == SDLK_y || (k == SDLK_z && shift)))
                    canvas->redo();
                // Ctrl+= / Ctrl+- zoom (center); Ctrl+0 resets to 100%.
                // SDLK_EQUALS is the unshifted "=" key (the "+" key).
                else if (ctrl && (k == SDLK_EQUALS || k == SDLK_PLUS || k == SDLK_KP_PLUS))
                    zoomInAtCenter();
                else if (ctrl && (k == SDLK_MINUS || k == SDLK_KP_MINUS))
                    zoomOutAtCenter();
                else if (ctrl && k == SDLK_0)
                    resetZoom();
                else if (k == SDLK_SPACE)
                    spaceDown = true; // hold to pan-drag
                else if (k == SDLK_b)
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
                {
                    canvas->pushUndo();
                    canvas->toggleVisible(canvas->getActiveLayer());
                }
                else if (k >= SDLK_1 && k <= SDLK_9)
                    canvas->setActiveLayer(k - SDLK_1);
            }
            else if (e.type == SDL_KEYUP)
            {
                if (e.key.keysym.sym == SDLK_SPACE)
                {
                    spaceDown = false;
                    panning = false; // safety: releasing space ends any pan
                }
            }
            else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
            {
                int cx, cy;
                if (screenToCanvas(e.button.x, e.button.y, cx, cy))
                {
                    // One undo entry per stroke: snapshot before the first pixel
                    // changes. True for the brush (committed at mouse-up) and the
                    // eraser (drawn continuously), so it must happen here, once.
                    if (canvas->activeIsDrawable())
                        canvas->pushUndo();

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
            SDL_Rect paper = {canvasX, canvasY, scaledCanvasW(), scaledCanvasH()};
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