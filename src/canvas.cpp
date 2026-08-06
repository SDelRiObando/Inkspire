#include "inkspire/canvas.hpp"
#include <iostream>
#include <cmath>

namespace Inkspire
{
    Canvas::Canvas() = default;
    Canvas::~Canvas() { shutdown(); }

    bool Canvas::createDrawingTexture(SDL_Texture *&tex)
    {
        tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                SDL_TEXTUREACCESS_TARGET, width, height);
        if (!tex)
        {
            std::cerr << "Failed to create layer texture: " << SDL_GetError() << '\n';
            return false;
        }
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
        // clear to transparent
        SDL_SetRenderTarget(renderer, tex);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);
        SDL_SetRenderTarget(renderer, nullptr);
        return true;
    }

    bool Canvas::createStrokeTexture()
    {
        strokeTex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                      SDL_TEXTUREACCESS_TARGET, width, height);
        if (!strokeTex)
        {
            std::cerr << "stroke tex failed: " << SDL_GetError() << '\n';
            return false;
        }
        SDL_SetTextureBlendMode(strokeTex, SDL_BLENDMODE_BLEND);
        return true;
    }

    bool Canvas::init(SDL_Renderer *renderer, int width, int height)
    {
        this->renderer = renderer;
        this->width = width;
        this->height = height;

        // Layer 0: white background
        Layer bg;
        bg.kind = Layer::Kind::Background;
        bg.bgColor = {255, 255, 255, 255};
        bg.name = "Background";
        layers.push_back(bg);

        // Layer 1: a drawing layer (active)
        Layer draw;
        draw.kind = Layer::Kind::Drawing;
        draw.name = "Layer 1";
        if (!createDrawingTexture(draw.texture))
            return false;
        layers.push_back(draw);

        activeLayer = 1;

        if (!createStrokeTexture())
            return false;
        beginStroke();
        return true;
    }

    SDL_Texture *Canvas::activeTexture()
    {
        if (activeLayer < 0 || activeLayer >= (int)layers.size())
            return nullptr;
        return layers[activeLayer].texture;
    }

    bool Canvas::activeIsDrawable() const
    {
        return activeLayer >= 0 && activeLayer < (int)layers.size() && layers[activeLayer].kind == Layer::Kind::Drawing;
    }

    void Canvas::setActiveLayer(int i)
    {
        if (i < 0 || i >= (int)layers.size())
            return;
        if (layers[i].kind != Layer::Kind::Drawing)
            return; // can't make background active for drawing
        activeLayer = i;
    }

    void Canvas::toggleVisible(int i)
    {
        if (i >= 0 && i < (int)layers.size())
            layers[i].visible = !layers[i].visible;
    }

    void Canvas::setBackgroundColor(SDL_Color c)
    {
        for (auto &L : layers)
            if (L.kind == Layer::Kind::Background)
                L.bgColor = c;
    }

    void Canvas::clear()
    {
        SDL_Texture *tex = activeTexture();
        if (!tex)
            return;
        SDL_SetRenderTarget(renderer, tex);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);
        SDL_SetRenderTarget(renderer, nullptr);
    }

    void Canvas::fillCircle(int cx, int cy, int radius)
    {
        for (int dy = -radius; dy <= radius; ++dy)
        {
            int dx = (int)std::sqrt((double)(radius * radius - dy * dy));
            SDL_RenderDrawLine(renderer, cx - dx, cy + dy, cx + dx, cy + dy);
        }
    }

    void Canvas::beginStroke()
    {
        SDL_SetRenderTarget(renderer, strokeTex);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);
        SDL_SetRenderTarget(renderer, nullptr);
    }

    void Canvas::strokeLine(int x1, int y1, int x2, int y2, int size, SDL_Color color)
    {
        SDL_SetRenderTarget(renderer, strokeTex);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);

        int radius = size / 2;
        if (radius < 1)
            SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
        else
        {
            int dx = x2 - x1, dy = y2 - y1;
            int steps = (int)std::sqrt((double)(dx * dx + dy * dy));
            if (steps == 0)
                fillCircle(x1, y1, radius);
            else
                for (int i = 0; i <= steps; ++i)
                    fillCircle(x1 + dx * i / steps, y1 + dy * i / steps, radius);
        }
        SDL_SetRenderTarget(renderer, nullptr);
    }

    void Canvas::commitStroke(float opacity)
    {
        SDL_Texture *tex = activeTexture();
        if (!tex)
        {
            beginStroke();
            return;
        }

        SDL_SetTextureAlphaMod(strokeTex, (Uint8)(opacity * 255));
        SDL_SetTextureBlendMode(strokeTex, SDL_BLENDMODE_BLEND);

        SDL_SetRenderTarget(renderer, tex);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_RenderCopy(renderer, strokeTex, nullptr, nullptr);
        SDL_SetRenderTarget(renderer, nullptr);

        SDL_SetTextureAlphaMod(strokeTex, 255);
        beginStroke();
    }

    void Canvas::drawLine(int x1, int y1, int x2, int y2, Tool tool, int size, SDL_Color color)
    {
        // Eraser path (direct) — targets the active drawing layer
        SDL_Texture *tex = activeTexture();
        if (!tex)
            return;

        SDL_SetRenderTarget(renderer, tex);
        if (tool == Tool::Eraser)
        {
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        }
        else
        {
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        }
        int radius = size / 2;
        if (radius < 1)
            SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
        else
        {
            int dx = x2 - x1, dy = y2 - y1;
            int steps = (int)std::sqrt((double)(dx * dx + dy * dy));
            if (steps == 0)
                fillCircle(x1, y1, radius);
            else
                for (int i = 0; i <= steps; ++i)
                    fillCircle(x1 + dx * i / steps, y1 + dy * i / steps, radius);
        }
        SDL_SetRenderTarget(renderer, nullptr);
    }

    void Canvas::renderCheckerboard(SDL_Renderer *renderer, const SDL_Rect &dst)
    {
        const int tile = 16;
        const SDL_Color light = {200, 200, 200, 255};
        const SDL_Color dark = {150, 150, 150, 255};
        SDL_RenderSetClipRect(renderer, &dst);
        for (int y = 0; y < dst.h; y += tile)
            for (int x = 0; x < dst.w; x += tile)
            {
                bool even = ((x / tile) + (y / tile)) % 2 == 0;
                const SDL_Color &c = even ? light : dark;
                SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
                SDL_Rect rect = {dst.x + x, dst.y + y, tile, tile};
                SDL_RenderFillRect(renderer, &rect);
            }
        SDL_RenderSetClipRect(renderer, nullptr);
    }

    void Canvas::addLayer()
    {
        Layer L;
        L.kind = Layer::Kind::Drawing;
        L.name = "Layer " + std::to_string(layers.size());
        if (!createDrawingTexture(L.texture))
            return;

        // Insert just above the active layer
        int insertAt = activeLayer + 1;
        if (insertAt > (int)layers.size())
            insertAt = (int)layers.size();
        layers.insert(layers.begin() + insertAt, L);
        activeLayer = insertAt; // make the new layer active
    }

    void Canvas::deleteLayer(int i)
    {
        if (i < 0 || i >= (int)layers.size())
            return;
        if (layers[i].kind == Layer::Kind::Background)
            return; // never delete background

        // Count drawing layers — don't delete the last one
        int drawingCount = 0;
        for (auto &L : layers)
            if (L.kind == Layer::Kind::Drawing)
                drawingCount++;
        if (drawingCount <= 1)
            return;

        if (layers[i].texture)
            SDL_DestroyTexture(layers[i].texture);
        layers.erase(layers.begin() + i);

        // Fix up activeLayer so it still points at a valid drawing layer
        if (activeLayer >= (int)layers.size())
            activeLayer = (int)layers.size() - 1;
        if (activeLayer >= 0 && layers[activeLayer].kind != Layer::Kind::Drawing)
        {
            // find nearest drawing layer
            for (int j = (int)layers.size() - 1; j >= 0; --j)
                if (layers[j].kind == Layer::Kind::Drawing)
                {
                    activeLayer = j;
                    break;
                }
        }
    }

    SDL_Color Canvas::getBackgroundColor() const
    {
        for (const auto &L : layers)
            if (L.kind == Layer::Kind::Background)
                return L.bgColor;
        return {255, 255, 255, 255};
    }

    SDL_Surface *Canvas::readLayerPixels(int i)
    {
        if (i < 0 || i >= (int)layers.size())
            return nullptr;
        if (layers[i].kind != Layer::Kind::Drawing || !layers[i].texture)
            return nullptr;

        SDL_Surface *surf = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_RGBA8888);
        if (!surf)
            return nullptr;

        SDL_SetRenderTarget(renderer, layers[i].texture);
        SDL_RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_RGBA8888, surf->pixels, surf->pitch);
        SDL_SetRenderTarget(renderer, nullptr);
        return surf;
    }

    SDL_Texture *Canvas::makeLayerTextureFromSurface(SDL_Surface *surf)
    {
        // Create a target texture and copy the surface into it
        SDL_Texture *tex = nullptr;
        if (!createDrawingTexture(tex))
            return nullptr;

        SDL_Texture *fromSurf = SDL_CreateTextureFromSurface(renderer, surf);
        if (fromSurf)
        {
            SDL_SetTextureBlendMode(fromSurf, SDL_BLENDMODE_NONE);
            SDL_SetRenderTarget(renderer, tex);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, fromSurf, nullptr, nullptr);
            SDL_SetRenderTarget(renderer, nullptr);
            SDL_DestroyTexture(fromSurf);
        }
        return tex;
    }

    void Canvas::loadLayers(std::vector<Layer> newLayers, int activeIdx, SDL_Color bgColor)
    {
        // Destroy current layer textures
        for (auto &L : layers)
            if (L.texture)
            {
                SDL_DestroyTexture(L.texture);
                L.texture = nullptr;
            }
        layers.clear();

        layers = std::move(newLayers);

        // Apply background color to the background layer
        for (auto &L : layers)
            if (L.kind == Layer::Kind::Background)
                L.bgColor = bgColor;

        // Clamp active layer to a valid drawing layer
        activeLayer = activeIdx;
        if (activeLayer < 0 || activeLayer >= (int)layers.size() ||
            layers[activeLayer].kind != Layer::Kind::Drawing)
        {
            for (int j = (int)layers.size() - 1; j >= 0; --j)
                if (layers[j].kind == Layer::Kind::Drawing)
                {
                    activeLayer = j;
                    break;
                }
        }

        beginStroke();
        clearHistory(); // freshly opened document — discard prior history
    }

    // ---------------------------------------------------------------------
    // Undo / redo
    // ---------------------------------------------------------------------

    Canvas::Snapshot Canvas::captureSnapshot()
    {
        Snapshot s;
        s.activeLayer = activeLayer;
        s.layers.reserve(layers.size());
        for (int i = 0; i < (int)layers.size(); ++i)
        {
            const Layer &L = layers[i];
            SnapLayer sl;
            sl.kind = L.kind;
            sl.name = L.name;
            sl.visible = L.visible;
            sl.bgColor = L.bgColor;
            if (L.kind == Layer::Kind::Drawing && L.texture)
                sl.pixels = readLayerPixels(i); // owned copy; nullptr on failure
            s.layers.push_back(std::move(sl));
        }
        return s;
    }

    void Canvas::restoreSnapshot(const Snapshot &s)
    {
        // Tear down current live textures
        for (auto &L : layers)
            if (L.texture)
            {
                SDL_DestroyTexture(L.texture);
                L.texture = nullptr;
            }
        layers.clear();

        // Rebuild live layers from the snapshot
        layers.reserve(s.layers.size());
        for (const auto &sl : s.layers)
        {
            Layer L;
            L.kind = sl.kind;
            L.name = sl.name;
            L.visible = sl.visible;
            L.bgColor = sl.bgColor;
            if (sl.kind == Layer::Kind::Drawing && sl.pixels)
                L.texture = makeLayerTextureFromSurface(sl.pixels);
            else if (sl.kind == Layer::Kind::Drawing)
                createDrawingTexture(L.texture); // empty layer fallback
            layers.push_back(std::move(L));
        }

        activeLayer = s.activeLayer;
        if (activeLayer < 0 || activeLayer >= (int)layers.size() ||
            layers[activeLayer].kind != Layer::Kind::Drawing)
        {
            for (int j = (int)layers.size() - 1; j >= 0; --j)
                if (layers[j].kind == Layer::Kind::Drawing)
                {
                    activeLayer = j;
                    break;
                }
        }

        beginStroke(); // discard any in-progress scratch
    }

    void Canvas::freeSnapshot(Snapshot &s)
    {
        for (auto &sl : s.layers)
            if (sl.pixels)
            {
                SDL_FreeSurface(sl.pixels);
                sl.pixels = nullptr;
            }
        s.layers.clear();
    }

    void Canvas::pushUndo()
    {
        undoStack.push_back(captureSnapshot());

        // Any new action invalidates the redo branch
        for (auto &s : redoStack)
            freeSnapshot(s);
        redoStack.clear();

        // Enforce the depth cap by dropping the oldest entry
        if ((int)undoStack.size() > MAX_UNDO)
        {
            freeSnapshot(undoStack.front());
            undoStack.erase(undoStack.begin());
        }
    }

    bool Canvas::undo()
    {
        if (undoStack.empty())
            return false;
        redoStack.push_back(captureSnapshot()); // so redo can come back here
        Snapshot s = std::move(undoStack.back());
        undoStack.pop_back();
        restoreSnapshot(s);
        freeSnapshot(s);
        return true;
    }

    bool Canvas::redo()
    {
        if (redoStack.empty())
            return false;
        undoStack.push_back(captureSnapshot());
        Snapshot s = std::move(redoStack.back());
        redoStack.pop_back();
        restoreSnapshot(s);
        freeSnapshot(s);
        return true;
    }

    void Canvas::clearHistory()
    {
        for (auto &s : undoStack)
            freeSnapshot(s);
        for (auto &s : redoStack)
            freeSnapshot(s);
        undoStack.clear();
        redoStack.clear();
    }

    void Canvas::reset()
    {
        // Destroy all existing layer textures
        for (auto &L : layers)
            if (L.texture)
            {
                SDL_DestroyTexture(L.texture);
                L.texture = nullptr;
            }
        layers.clear();

        // Rebuild the default document: white background + one drawing layer
        Layer bg;
        bg.kind = Layer::Kind::Background;
        bg.bgColor = {255, 255, 255, 255};
        bg.name = "Background";
        layers.push_back(bg);

        Layer draw;
        draw.kind = Layer::Kind::Drawing;
        draw.name = "Layer 1";
        createDrawingTexture(draw.texture);
        layers.push_back(draw);

        activeLayer = 1;
        beginStroke();  // clear any in-progress scratch
        clearHistory(); // new document — old undo states no longer apply
    }

    void Canvas::render(SDL_Renderer *renderer, const SDL_Rect &dst, float strokeOpacity, bool strokeActive)
    {
        for (size_t i = 0; i < layers.size(); ++i)
        {
            Layer &L = layers[i];
            if (!L.visible)
                continue;

            if (L.kind == Layer::Kind::Background)
            {
                SDL_SetRenderDrawColor(renderer, L.bgColor.r, L.bgColor.g, L.bgColor.b, L.bgColor.a);
                SDL_RenderFillRect(renderer, &dst);
            }
            else
            {
                SDL_RenderCopy(renderer, L.texture, nullptr, &dst);
                if ((int)i == activeLayer && strokeActive)
                {
                    SDL_SetTextureAlphaMod(strokeTex, (Uint8)(strokeOpacity * 255));
                    SDL_RenderCopy(renderer, strokeTex, nullptr, &dst);
                    SDL_SetTextureAlphaMod(strokeTex, 255);
                }
            }
        }
    }

    void Canvas::shutdown()
    {
        clearHistory(); // free all snapshot surfaces
        for (auto &L : layers)
            if (L.texture)
            {
                SDL_DestroyTexture(L.texture);
                L.texture = nullptr;
            }
        layers.clear();
        if (strokeTex)
        {
            SDL_DestroyTexture(strokeTex);
            strokeTex = nullptr;
        }
    }

    void Canvas::preserveAcrossResize()
    {
        // Recreate the scratch buffer (empty)
        if (strokeTex)
        {
            SDL_DestroyTexture(strokeTex);
            strokeTex = nullptr;
        }
        createStrokeTexture();
        beginStroke();

        // Snapshot + restore each drawing layer's texture
        for (auto &L : layers)
        {
            if (L.kind != Layer::Kind::Drawing || !L.texture)
                continue;

            SDL_Surface *snap = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_RGBA8888);
            if (!snap)
                continue;

            SDL_SetRenderTarget(renderer, L.texture);
            SDL_RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_RGBA8888, snap->pixels, snap->pitch);
            SDL_SetRenderTarget(renderer, nullptr);

            SDL_DestroyTexture(L.texture);
            L.texture = nullptr;
            if (!createDrawingTexture(L.texture))
            {
                SDL_FreeSurface(snap);
                continue;
            }

            SDL_Texture *restore = SDL_CreateTextureFromSurface(renderer, snap);
            SDL_FreeSurface(snap);
            if (!restore)
                continue;
            SDL_SetTextureBlendMode(restore, SDL_BLENDMODE_NONE);

            SDL_SetRenderTarget(renderer, L.texture);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, restore, nullptr, nullptr);
            SDL_SetRenderTarget(renderer, nullptr);
            SDL_DestroyTexture(restore);
        }
    }
}