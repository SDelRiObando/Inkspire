#include "inkspire/toolspanel.hpp"

namespace Inkspire
{
    void ToolsPanel::init(TTF_Font *font, ToolState *state)
    {
        this->font = font;
        this->state = state;
    }

    static bool pointInRect(int px, int py, const SDL_Rect &r)
    {
        return px >= r.x && px < r.x + r.w && py >= r.y && py < r.y + r.h;
    }

    void ToolsPanel::renderText(SDL_Renderer *renderer, const std::string &text, int x, int y, SDL_Color color)
    {
        SDL_Surface *surf = TTF_RenderText_Blended(font, text.c_str(), color);
        if (!surf)
            return;
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_Rect dst = {x, y, surf->w, surf->h};
        SDL_RenderCopy(renderer, tex, nullptr, &dst);
        SDL_DestroyTexture(tex);
        SDL_FreeSurface(surf);
    }

    bool ToolsPanel::handleEvent(const SDL_Event &e, const PanelContext &ctx)
    {
        (void)ctx;
        if (!state)
            return false;
        if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
        {
            int mx = e.button.x, my = e.button.y;
            if (pointInRect(mx, my, brushRect))
            {
                state->setTool(Tool::Brush);
                return true;
            }
            if (pointInRect(mx, my, eraserRect))
            {
                state->setTool(Tool::Eraser);
                return true;
            }
            // Consume clicks anywhere in the button column
            if (pointInRect(mx, my, bounds_))
                return true;
        }
        return false;
    }

    void ToolsPanel::render(const PanelContext &ctx)
    {
        setBounds(ctx.bounds);
        SDL_Renderer *renderer = ctx.renderer;
        const SDL_Rect &b = ctx.bounds;
        Tool current = state ? state->tool() : Tool::Brush;

        // Button column background
        SDL_SetRenderDrawColor(renderer, 40, 40, 43, 255);
        SDL_RenderFillRect(renderer, &b);

        SDL_Color textColor = {230, 230, 230, 255};
        int y = b.y + 8;

        brushRect = {b.x + 8, y, b.w - 16, buttonH};
        SDL_SetRenderDrawColor(renderer,
                               current == Tool::Brush ? 80 : 55,
                               current == Tool::Brush ? 80 : 55,
                               current == Tool::Brush ? 90 : 58, 255);
        SDL_RenderFillRect(renderer, &brushRect);
        renderText(renderer, "Brush", brushRect.x + 6, brushRect.y + buttonH / 2 - 8, textColor);
        y += buttonH + 8;

        eraserRect = {b.x + 8, y, b.w - 16, buttonH};
        SDL_SetRenderDrawColor(renderer,
                               current == Tool::Eraser ? 80 : 55,
                               current == Tool::Eraser ? 80 : 55,
                               current == Tool::Eraser ? 90 : 58, 255);
        SDL_RenderFillRect(renderer, &eraserRect);
        renderText(renderer, "Erase", eraserRect.x + 6, eraserRect.y + buttonH / 2 - 8, textColor);
    }
}