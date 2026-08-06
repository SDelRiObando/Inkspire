#pragma once
#include "canvas.hpp"
#include "menu.hpp"
#include "toolpanel.hpp"
#include "colorpicker.hpp"
#include "layerspanel.hpp"
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
        void updateWindowTitle();
        void recenterCanvas();
        void toggleFullscreen();
        bool screenToCanvas(int sx, int sy, int &cx, int &cy) const;
        void saveCurrent(); // quick-save to currentFilePath, or Save As if none
        void saveAs();      // always prompt

        // Zoom / pan. zoom is a view-only scale factor; the document itself is
        // always edited at native resolution. zoomAt keeps the given screen
        // point fixed while changing the factor (pivot-preserving zoom).
        void zoomAt(int screenPivotX, int screenPivotY, float newZoom);
        void zoomInAtCenter();
        void zoomOutAtCenter();
        void resetZoom();
        int scaledCanvasW() const; // canvasW * zoom, rounded
        int scaledCanvasH() const; // canvasH * zoom, rounded

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

        LayersPanel layersPanel;

        ColorPicker bgPicker;
        bool bgPopupOpen = false;

        // Where the top-left of the paper sits within the window
        int canvasX = 0;
        int canvasY = 0;

        // Fixed paper dimensions (native document size — never changes with zoom)
        int canvasW = 800;
        int canvasH = 600;

        // View transform
        float zoom = 1.0f;                       // 1.0 = 100%
        static constexpr float MIN_ZOOM = 0.1f;  // 10%
        static constexpr float MAX_ZOOM = 32.0f; // 3200%

        // Panning with space + drag
        bool spaceDown = false; // space held → pan mode
        bool panning = false;   // actively dragging a pan
        int panLastX = 0, panLastY = 0;

        // File path
        std::string currentFilePath; // empty = no file yet (untitled)

        bool quit = false;
        bool drawing = false;
        bool strokeActive = false;
        int lastX = 0, lastY = 0;
    };
}