#include "inkspire/canvas.hpp"
#include <iostream>

namespace Inkspire
{
    Canvas::Canvas() = default;

    Canvas::~Canvas()
    {
        shutdown();
    }

    bool Canvas::createTexture()
    {
        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                    SDL_TEXTUREACCESS_TARGET, width, height);
        if (!texture)
        {
            std::cerr << "Failed to create canvas texture: " << SDL_GetError() << '\n';
            return false;
        }
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
        return true;
    }

    bool Canvas::init(SDL_Renderer *renderer, int width, int height)
    {
        this->renderer = renderer;
        this->width = width;
        this->height = height;

        if (!createTexture())
            return false;
        if (!createStrokeTexture())
            return false;

        clear();
        beginStroke(); // start with a clean scratch buffer
        return true;
    }

    void Canvas::clear()
    {
        SDL_SetRenderTarget(renderer, texture);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);
        SDL_SetRenderTarget(renderer, nullptr);
    }

    void Canvas::fillCircle(int cx, int cy, int radius)
    {
        // Draw a filled circle by drawing horizontal spans for each row
        for (int dy = -radius; dy <= radius; ++dy)
        {
            int dx = (int)std::sqrt((double)(radius * radius - dy * dy));
            SDL_RenderDrawLine(renderer, cx - dx, cy + dy, cx + dx, cy + dy);
        }
    }

    bool Canvas::createStrokeTexture()
    {
        strokeTex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                      SDL_TEXTUREACCESS_TARGET, width, height);
        if (!strokeTex)
        {
            std::cerr << "Failed to create stroke texture: " << SDL_GetError() << '\n';
            return false;
        }
        SDL_SetTextureBlendMode(strokeTex, SDL_BLENDMODE_BLEND);
        return true;
    }
    void Canvas::beginStroke()
    {
        // Clear the scratch buffer to transparent
        SDL_SetRenderTarget(renderer, strokeTex);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);
        SDL_SetRenderTarget(renderer, nullptr);
    }

    void Canvas::strokeLine(int x1, int y1, int x2, int y2, int size, SDL_Color color)
    {
        // Stamp onto the scratch buffer at FULL opacity (alpha 255).
        // Overlapping stamps merge instead of double-blending.
        SDL_SetRenderTarget(renderer, strokeTex);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);

        int radius = size / 2;
        if (radius < 1)
        {
            SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
        }
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
        // Bake the scratch buffer onto the canvas ONCE at the chosen opacity
        SDL_SetTextureAlphaMod(strokeTex, (Uint8)(opacity * 255));
        SDL_SetTextureBlendMode(strokeTex, SDL_BLENDMODE_BLEND);

        SDL_SetRenderTarget(renderer, texture);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_RenderCopy(renderer, strokeTex, nullptr, nullptr);
        SDL_SetRenderTarget(renderer, nullptr);

        SDL_SetTextureAlphaMod(strokeTex, 255); // reset
        beginStroke();                          // clear scratch for next stroke
    }

    void Canvas::drawLine(int x1, int y1, int x2, int y2, Tool tool, int size, SDL_Color color)
    {
        SDL_SetRenderTarget(renderer, texture);

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
        {
            // size 1: just a thin line
            SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
        }
        else
        {
            // Stamp circles along the line for a round-tipped thick stroke
            int dx = x2 - x1;
            int dy = y2 - y1;
            int steps = (int)std::sqrt((double)(dx * dx + dy * dy));
            if (steps == 0)
            {
                fillCircle(x1, y1, radius);
            }
            else
            {
                for (int i = 0; i <= steps; ++i)
                {
                    int px = x1 + dx * i / steps;
                    int py = y1 + dy * i / steps;
                    fillCircle(px, py, radius);
                }
            }
        }

        SDL_SetRenderTarget(renderer, nullptr);
    }

    void Canvas::preserveAcrossResize()
    {
        if (!texture)
            return;

        // after createTexture() succeeds in preserveAcrossResize:
        if (strokeTex)
        {
            SDL_DestroyTexture(strokeTex);
            strokeTex = nullptr;
        }
        createStrokeTexture();
        beginStroke();

        // 1. Read the current texture's pixels into a CPU surface
        SDL_Surface *snapshot = SDL_CreateRGBSurfaceWithFormat(
            0, width, height, 32, SDL_PIXELFORMAT_RGBA8888);
        if (!snapshot)
        {
            std::cerr << "preserveAcrossResize: failed to create surface: " << SDL_GetError() << '\n';
            return;
        }

        SDL_SetRenderTarget(renderer, texture);
        if (SDL_RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_RGBA8888,
                                 snapshot->pixels, snapshot->pitch) != 0)
        {
            std::cerr << "preserveAcrossResize: RenderReadPixels failed: " << SDL_GetError() << '\n';
            SDL_SetRenderTarget(renderer, nullptr);
            SDL_FreeSurface(snapshot);
            return;
        }
        SDL_SetRenderTarget(renderer, nullptr);

        // 2. Recreate the target texture (the old one may be invalid after resize)
        SDL_DestroyTexture(texture);
        texture = nullptr;
        if (!createTexture())
        {
            SDL_FreeSurface(snapshot);
            return;
        }

        // 3. Blit the snapshot back into the fresh target texture
        SDL_Texture *restore = SDL_CreateTextureFromSurface(renderer, snapshot);
        SDL_FreeSurface(snapshot);
        if (!restore)
        {
            std::cerr << "preserveAcrossResize: failed to create restore texture: " << SDL_GetError() << '\n';
            return;
        }
        SDL_SetTextureBlendMode(restore, SDL_BLENDMODE_NONE);

        SDL_SetRenderTarget(renderer, texture);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, restore, nullptr, nullptr);
        SDL_SetRenderTarget(renderer, nullptr);

        SDL_DestroyTexture(restore);
    }

    void Canvas::renderCheckerboard(SDL_Renderer *renderer, const SDL_Rect &dst)
    {
        const int tile = 16;
        const SDL_Color light = {200, 200, 200, 255};
        const SDL_Color dark = {150, 150, 150, 255};

        SDL_RenderSetClipRect(renderer, &dst);
        for (int y = 0; y < dst.h; y += tile)
        {
            for (int x = 0; x < dst.w; x += tile)
            {
                bool even = ((x / tile) + (y / tile)) % 2 == 0;
                const SDL_Color &c = even ? light : dark;
                SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
                SDL_Rect rect = {dst.x + x, dst.y + y, tile, tile};
                SDL_RenderFillRect(renderer, &rect);
            }
        }
        SDL_RenderSetClipRect(renderer, nullptr);
    }

    void Canvas::render(SDL_Renderer *renderer, const SDL_Rect &dst, float strokeOpacity, bool strokeActive)
    {
        SDL_RenderCopy(renderer, texture, nullptr, &dst);

        if (strokeActive)
        {
            // Preview the in-progress stroke at the chosen opacity
            SDL_SetTextureAlphaMod(strokeTex, (Uint8)(strokeOpacity * 255));
            SDL_RenderCopy(renderer, strokeTex, nullptr, &dst);
            SDL_SetTextureAlphaMod(strokeTex, 255);
        }
    }

    void Canvas::shutdown()
    {
        if (texture)
        {
            SDL_DestroyTexture(texture);
            texture = nullptr;
        }
        if (strokeTex)
        {
            SDL_DestroyTexture(strokeTex);
            strokeTex = nullptr;
        }
    }
}