#pragma once
#include "canvas.hpp"
#include "menu.hpp"
#include "toolpanel.hpp"
#include "colorpicker.hpp"
#include <SDL.h>
#include <memory>

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
        void recenterCanvas();
        void toggleFullscreen();
        bool screenToCanvas(int sx, int sy, int &cx, int &cy) const;

        SDL_Window *window = nullptr;
        SDL_Renderer *renderer = nullptr;

        std::unique_ptr<Canvas> canvas;

        TTF_Font *font = nullptr;
        MenuBar menuBar;
        ToolPanel toolPanel;

        Tool currentTool = Tool::Brush;

        // Window + workspace state
        int windowW = 800;
        int windowH = 600;
        bool fullscreen = false;

        ColorPicker bgPicker;
        bool bgPopupOpen = false;

        // Where the top-left of the paper sits within the window
        int canvasX = 0;
        int canvasY = 0;

        // Fixed paper dimensions
        int canvasW = 800;
        int canvasH = 600;

        bool quit = false;
        bool drawing = false;
        bool strokeActive = false;
        int lastX = 0, lastY = 0;
    };
}