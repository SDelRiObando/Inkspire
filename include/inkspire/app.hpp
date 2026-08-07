#pragma once
#include "canvas.hpp"
#include "menu.hpp"
#include "panel.hpp"
#include "docklayout.hpp"
#include "toolstate.hpp"
#include "toolspanel.hpp"
#include "brushsizepanel.hpp"
#include "brushsettingspanel.hpp"
#include "brushcolorpanel.hpp"
#include "colorpalettepanel.hpp"
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

        // Panel layout: the dock tree owns all rectangles. relayout() recomputes
        // it for the current window; panelById resolves a dock id to the live
        // Panel object; panelRect looks up a panel's assigned rectangle.
        void relayout();
        Panel *panelById(const std::string &id);
        SDL_Rect panelRect(const std::string &id) const;
        PanelContext makeContext(const SDL_Rect &bounds);
        void renderPanel(const PanelPlacement &pl); // header/rail chrome + content
        // The name shown on a column's header: the shared category of all its
        // panels ("Tool Options"), or a combined name when it mixes categories
        // ("Layers + Tool Options"). Order follows the column's panel order,
        // each distinct category listed once.
        std::string columnCategory(const PanelPlacement &pl) const;
        void drawText(const std::string &s, int x, int y, SDL_Color c);

        // Floating panels: torn off the dock, drawn on top, freely positioned.
        // Not part of the dock tree. Re-dockable by dragging back. Defined here,
        // before the methods and members that reference it.
        struct FloatingPanel
        {
            std::string id;
            SDL_Rect rect{0, 0, 0, 0}; // full frame (header + content) in window space
        };

        void renderFloats();
        int floatHeaderAt(int x, int y) const;                      // index of float whose header holds (x,y), else -1
        int floatContentPanelAt(int x, int y) const;                // index whose content holds (x,y), else -1
        void tearOff(const std::string &panelId, int atX, int atY); // dock → float
        SDL_Rect floatContentRect(const FloatingPanel &f) const;

        SDL_Window *window = nullptr;
        SDL_Renderer *renderer = nullptr;

        std::unique_ptr<Canvas> canvas;

        TTF_Font *font = nullptr;
        MenuBar menuBar;
        ToolState toolState;
        ToolsPanel toolsPanel;
        BrushSizePanel brushSizePanel;
        BrushSettingsPanel brushSettingsPanel;
        BrushColorPanel brushColorPanel;
        ColorPalettePanel colorPalettePanel;

        Tool currentTool = Tool::Brush;

        // Window + workspace state
        int windowW = 800;
        int windowH = 600;
        bool fullscreen = false;

        LayersPanel layersPanel;

        DockLayout dock; // owns all panel rectangles + the canvas center rect

        // Divider dragging (resize docks)
        DockNode *dragDivider_ = nullptr; // non-null while dragging a divider
        SDL_Cursor *curDefault_ = nullptr;
        SDL_Cursor *curResizeH_ = nullptr; // horizontal resize (vertical divider)
        SDL_Cursor *curResizeV_ = nullptr; // vertical resize (horizontal divider)

        // Panel dragging (drag-to-redock). A header press arms a pending drag;
        // moving past a threshold promotes it to an active drag.
        DockNode *pendingDragLeaf_ = nullptr; // header pressed, not yet dragging
        std::string dragPanelId_;             // id of the panel being dragged
        std::string dragPanelTitle_;          // its title, for the ghost label
        bool panelDragging_ = false;          // past the threshold
        int dragStartX_ = 0, dragStartY_ = 0;
        int dragCurX_ = 0, dragCurY_ = 0;
        bool dragFromFloat_ = false; // true if the drag started on a float
        int dragFloatIndex_ = -1;    // which float, when dragFromFloat_

        // In-window floating panels (struct FloatingPanel defined above).
        std::vector<FloatingPanel> floats_;
        static constexpr int floatHeaderH_ = 22;
        static constexpr int floatDefaultW_ = 200;
        static constexpr int floatDefaultH_ = 260;

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