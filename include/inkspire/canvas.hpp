#pragma once

#include <SDL.h>
#include <vector>
#include <string>

namespace Inkspire
{
    enum class Tool
    {
        Brush,
        Eraser
    };

    struct Layer
    {
        enum class Kind
        {
            Background,
            Drawing
        };
        Kind kind = Kind::Drawing;

        SDL_Texture *texture = nullptr;           // Drawing layers
        SDL_Color bgColor = {255, 255, 255, 255}; // Background layers
        bool visible = true;
        std::string name;
    };

    class Canvas
    {
    public:
        Canvas();
        ~Canvas();

        bool init(SDL_Renderer *renderer, int width, int height);
        void clear(); // clears the active drawing layer

        void beginStroke();
        void strokeLine(int x1, int y1, int x2, int y2, int size, SDL_Color color);
        void commitStroke(float opacity);
        void drawLine(int x1, int y1, int x2, int y2, Tool tool, int size, SDL_Color color = {0, 0, 0, 255});

        void renderCheckerboard(SDL_Renderer *renderer, const SDL_Rect &dst);
        void render(SDL_Renderer *renderer, const SDL_Rect &dst, float strokeOpacity, bool strokeActive);
        void reset(); // File > New: discard all layers, restore default document
        void shutdown();

        void preserveAcrossResize();

        // Layer controls
        void addLayer();         // new drawing layer above the active one
        void deleteLayer(int i); // remove layer i (guards background & last drawing layer)
        const std::vector<Layer> &getLayers() const { return layers; }

        int layerCount() const { return (int)layers.size(); }
        void setActiveLayer(int i);
        int getActiveLayer() const { return activeLayer; }
        int getWidth() const { return width; }
        int getHeight() const { return height; }
        SDL_Color getBackgroundColor() const;
        SDL_Surface *readLayerPixels(int i); // caller frees; nullptr for non-drawing layers
        SDL_Texture *makeLayerTextureFromSurface(SDL_Surface *surf);
        void loadLayers(std::vector<Layer> newLayers, int activeIdx, SDL_Color bgColor);
        void toggleVisible(int i);
        void setBackgroundColor(SDL_Color c);
        bool activeIsDrawable() const;

        // ---- Clipboard (copy / paste) ----
        // The clipboard holds a copied image plus the canvas rectangle it came
        // from. Copying the whole active layer stores a full-canvas rect; once a
        // selection tool exists, copyRegion() will store just the selection's
        // bounding box, and paste() will drop it back at that same spot — so the
        // same paste path serves both cases. copyActiveLayer() is the no-selection
        // fallback (equivalent to copyRegion(full-canvas)).
        //
        // Returns false when there's nothing copyable (e.g. active layer isn't a
        // drawing layer). paste() returns false when the clipboard is empty.
        bool copyActiveLayer();
        bool copyRegion(const SDL_Rect &canvasRect); // selection-aware copy
        bool paste();                                // new layer above active
        bool hasClipboard() const { return clipboard_.pixels != nullptr; }

        // Undo / redo. Callers invoke pushUndo() immediately BEFORE a mutating
        // action (one call per undoable unit of work). undo()/redo() restore
        // whole-document state and return true if anything changed.
        void pushUndo();
        bool undo();
        bool redo();
        bool canUndo() const { return !undoStack.empty(); }
        bool canRedo() const { return !redoStack.empty(); }
        void clearHistory();

    private:
        bool createDrawingTexture(SDL_Texture *&tex);
        bool createStrokeTexture();
        void fillCircle(int cx, int cy, int radius);
        SDL_Texture *activeTexture(); // convenience

        // ---- History ----
        // A single undoable state: a full, inert copy of the document. Drawing
        // layers keep their pixels as an SDL_Surface (owned) rather than a live
        // texture, so a snapshot costs nothing to keep around besides memory.
        struct SnapLayer
        {
            Layer::Kind kind = Layer::Kind::Drawing;
            std::string name;
            bool visible = true;
            SDL_Color bgColor = {255, 255, 255, 255};
            SDL_Surface *pixels = nullptr; // owned; only for drawing layers
        };
        struct Snapshot
        {
            int activeLayer = 1;
            std::vector<SnapLayer> layers;
        };

        Snapshot captureSnapshot();              // deep-copy current document
        void restoreSnapshot(const Snapshot &s); // rebuild live document from a snapshot
        static void freeSnapshot(Snapshot &s);   // free owned surfaces

        std::vector<Snapshot> undoStack;
        std::vector<Snapshot> redoStack;

        // ---- Clipboard ----
        // A copied image and the canvas rect it occupied. `pixels` is an owned
        // RGBA surface (freed on overwrite and in shutdown). `srcRect` is where
        // it came from / where paste() places it. For a whole-layer copy the
        // rect is the full canvas; for a future selection it's the selection's
        // bounding box. Kept out of undo history — the clipboard survives undo.
        struct Clipboard
        {
            SDL_Surface *pixels = nullptr; // owned; nullptr when empty
            SDL_Rect srcRect{0, 0, 0, 0};  // canvas-space origin/size of the copy
        };
        Clipboard clipboard_;

        // Default cap on undo depth. More = more history but more memory/slower;
        // fewer = leaner and faster. Exposed via settings later; a constant for now.
        static constexpr int MAX_UNDO = 200;

        std::vector<Layer> layers;
        int activeLayer = 1; // index 0 is the background
        SDL_Texture *strokeTex = nullptr;

        int width = 0;
        int height = 0;
        SDL_Renderer *renderer = nullptr;
    };
}