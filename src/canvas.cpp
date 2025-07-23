#include "inkspire/canvas.hpp"
#include <iostream>

namespace Inkspire
{
    Canvas::Canvas() {}

    Canvas::~Canvas()
    {
        shutdown();
    }

    bool Canvas::init(SDL_Renderer *renderer, int width, int height)
    {
        this->renderer = renderer;
        this->width = width;
        this->height = height;

        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, width, height);
        if (!texture)
        {
            std::cerr << "Failed to create canvas texture: " << SDL_GetError() << '\n';
            return false;
        }

        // Clear the texture to white
        clear();

        return true;
    }

    void Canvas::clear(Uint8 r, Uint8 g, Uint8 b, Uint8 a)
    {
        SDL_SetRenderTarget(renderer, texture);
        SDL_SetRenderDrawColor(renderer, r, g, b, a);
        SDL_RenderClear(renderer);
        SDL_SetRenderTarget(renderer, nullptr);
    }

    void Canvas::drawLine(int x1, int y1, int x2, int y2, SDL_Color color)
    {
        // Set the render target to the texture (canvas)
        SDL_SetRenderTarget(renderer, texture);
        // Set draw color
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        // Draw the line
        SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
        // Reset render target back to default (the window)
        SDL_SetRenderTarget(renderer, nullptr);
    }

    void Canvas::render(SDL_Renderer *renderer)
    {
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    }

    void Canvas::shutdown()
    {
        if (texture)
        {
            SDL_DestroyTexture(texture);
            texture = nullptr;
        }
    }
}
