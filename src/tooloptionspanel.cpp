#include "inkspire/tooloptionspanel.hpp"
#include <algorithm>
#include <cmath>

namespace Inkspire
{
    void ToolOptionsPanel::init(TTF_Font *font, ToolState *state)
    {
        this->font = font;
        this->state = state;
    }

    static bool pointInRect(int px, int py, const SDL_Rect &r)
    {
        return px >= r.x && px < r.x + r.w && py >= r.y && py < r.y + r.h;
    }

    void ToolOptionsPanel::renderText(SDL_Renderer *renderer, const std::string &text, int x, int y, SDL_Color color)
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

    bool ToolOptionsPanel::handleEvent(const SDL_Event &e, const PanelContext &ctx)
    {
        (void)ctx;
        if (!state)
            return false;

        Tool current = state->tool();

        // Color picker (brush only) gets first crack.
        if (current == Tool::Brush && state->colorPicker().handleEvent(e))
            return true;

        if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
        {
            int mx = e.button.x, my = e.button.y;

            // Preset dots
            for (int i = 0; i < presetCount; ++i)
            {
                if (pointInRect(mx, my, presetRects[i]))
                {
                    state->activeSize() = presets[i];
                    return true;
                }
            }

            // Size slider
            if (pointInRect(mx, my, sliderHit))
            {
                draggingSlider = true;
                float t = (float)(mx - sliderTrack.x) / sliderTrack.w;
                state->activeSize() = 1 + (int)(std::clamp(t, 0.0f, 1.0f) * 99);
                return true;
            }

            // Opacity slider (brush only)
            if (current == Tool::Brush && pointInRect(mx, my, opacityHit))
            {
                draggingOpacity = true;
                float t = (float)(mx - opacityTrack.x) / opacityTrack.w;
                state->setOpacity(std::clamp(t, 0.0f, 1.0f));
                return true;
            }

            // Consume clicks inside the panel generally
            if (pointInRect(mx, my, bounds_))
                return true;
        }
        else if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT)
        {
            if (draggingSlider || draggingOpacity)
            {
                draggingSlider = false;
                draggingOpacity = false;
                return true;
            }
        }
        else if (e.type == SDL_MOUSEMOTION && draggingSlider)
        {
            float t = (float)(e.motion.x - sliderTrack.x) / sliderTrack.w;
            state->activeSize() = 1 + (int)(std::clamp(t, 0.0f, 1.0f) * 99);
            return true;
        }
        else if (e.type == SDL_MOUSEMOTION && draggingOpacity)
        {
            float t = (float)(e.motion.x - opacityTrack.x) / opacityTrack.w;
            state->setOpacity(std::clamp(t, 0.0f, 1.0f));
            return true;
        }
        return false;
    }

    void ToolOptionsPanel::render(const PanelContext &ctx)
    {
        setBounds(ctx.bounds);
        SDL_Renderer *renderer = ctx.renderer;
        const SDL_Rect &b = ctx.bounds;
        if (!state)
            return;

        Tool current = state->tool();
        SDL_Color textColor = {230, 230, 230, 255};

        // Panel background
        SDL_SetRenderDrawColor(renderer, 48, 48, 52, 255);
        SDL_RenderFillRect(renderer, &b);

        int pad = 12;
        int x = b.x + pad;
        int y = b.y + pad;
        int contentW = b.w - pad * 2;

        const char *title = (current == Tool::Brush) ? "Brush" : "Eraser";
        renderText(renderer, title, x, y, textColor);
        y += 26;

        renderText(renderer, "Size", x, y, textColor);
        y += 22;

        // Preset dots — grid layout
        const int cols = 4;
        int rows = (presetCount + cols - 1) / cols; // ceil
        int cellW = contentW / cols;
        int cellH = 30;
        int maxDotR = cellH / 2 - 3; // largest dot fits inside a cell

        for (int i = 0; i < presetCount; ++i)
        {
            int col = i % cols;
            int row = i / cols;
            int cx = x + col * cellW + cellW / 2;
            int cy = y + row * cellH + cellH / 2;

            presetRects[i] = {cx - cellW / 2, cy - cellH / 2, cellW, cellH}; // full cell clickable

            float frac = (float)presets[i] / (float)presets[presetCount - 1]; // 0..1
            int radius = 2 + (int)(frac * (maxDotR - 2));

            bool selected = (state->brushSize() == presets[i]);
            SDL_SetRenderDrawColor(renderer, selected ? 120 : 200, selected ? 160 : 200, selected ? 255 : 200, 255);
            for (int dy = -radius; dy <= radius; ++dy)
            {
                int dx = (int)std::sqrt((double)(radius * radius - dy * dy));
                SDL_RenderDrawLine(renderer, cx - dx, cy + dy, cx + dx, cy + dy);
            }
        }
        y += rows * cellH + 12;

        // Size slider (horizontal)
        sliderTrack = {x, y, contentW, 6};
        sliderHit = {x, y - 10, contentW, 26}; // fat invisible grab zone
        SDL_SetRenderDrawColor(renderer, 25, 25, 27, 255);
        SDL_RenderFillRect(renderer, &sliderTrack);

        float t = (float)(state->brushSize() - 1) / 99.0f;
        int handleX = sliderTrack.x + (int)(t * sliderTrack.w);
        SDL_Rect handle = {handleX - 5, sliderTrack.y - 6, 10, 18};
        SDL_SetRenderDrawColor(renderer, 180, 180, 190, 255);
        SDL_RenderFillRect(renderer, &handle);
        y += 24;

        renderText(renderer, std::to_string(state->brushSize()) + " px", x, y, textColor);
        y += 26;

        // Brush-only: opacity slider + color picker.
        if (current == Tool::Brush)
        {
            renderText(renderer, "Opacity", x, y, textColor);
            y += 22;

            opacityTrack = {x, y, contentW, 6};
            opacityHit = {x, y - 10, contentW, 26};
            SDL_SetRenderDrawColor(renderer, 25, 25, 27, 255);
            SDL_RenderFillRect(renderer, &opacityTrack);

            float op = state->brushOpacity();
            int ohx = opacityTrack.x + (int)(op * opacityTrack.w);
            SDL_Rect oh = {ohx - 5, opacityTrack.y - 6, 10, 18};
            SDL_SetRenderDrawColor(renderer, 180, 180, 190, 255);
            SDL_RenderFillRect(renderer, &oh);
            y += 24;

            renderText(renderer, std::to_string((int)(op * 100)) + "%", x, y, textColor);
            y += 26;

            renderText(renderer, "Color", x, y, textColor);
            y += 22;
            y = state->colorPicker().render(renderer, x, y, contentW);
        }
    }
}
