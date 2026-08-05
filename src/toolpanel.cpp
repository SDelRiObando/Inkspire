#include "inkspire/toolpanel.hpp"
#include <algorithm>
#include <cmath>

namespace Inkspire
{
    void ToolPanel::init(TTF_Font *font, int menuBarHeight)
    {
        this->font = font;
        this->topY = menuBarHeight;
        this->flyoutX = panelW;
        picker.init(font);
    }

    static bool pointInRect(int px, int py, const SDL_Rect &r)
    {
        return px >= r.x && px < r.x + r.w && py >= r.y && py < r.y + r.h;
    }

    void ToolPanel::renderText(SDL_Renderer *renderer, const std::string &text, int x, int y, SDL_Color color)
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

    bool ToolPanel::handleEvent(const SDL_Event &e)
    {
        // Flyout gets first crack (it sits to the right of the buttons)
        if (flyoutOpen && handleFlyoutEvent(e))
            return true;

        if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
        {
            int mx = e.button.x, my = e.button.y;

            if (pointInRect(mx, my, brushRect))
            {
                currentTool = Tool::Brush;
                flyoutOpen = true;
                return true;
            }
            if (pointInRect(mx, my, eraserRect))
            {
                currentTool = Tool::Eraser;
                flyoutOpen = true;
                return true;
            }
            // Consume clicks anywhere in the button column
            if (mx < panelW)
                return true;
        }
        return false;
    }

    bool ToolPanel::handleFlyoutEvent(const SDL_Event &e)
    {
        if (currentTool == Tool::Brush && picker.handleEvent(e))
            return true;

        if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
        {
            int mx = e.button.x, my = e.button.y;

            // Preset dots
            for (int i = 0; i < presetCount; ++i)
            {
                if (pointInRect(mx, my, presetRects[i]))
                {
                    activeSize() = presets[i];
                    return true;
                }
            }

            // Slider
            if (pointInRect(mx, my, sliderHit))
            {
                draggingSlider = true;
                float t = (float)(mx - sliderTrack.x) / sliderTrack.w;
                activeSize() = 1 + (int)(std::clamp(t, 0.0f, 1.0f) * 99);
                return true;
            }

            if (currentTool == Tool::Brush && pointInRect(mx, my, opacityHit))
            {
                draggingOpacity = true;
                float t = (float)(mx - opacityTrack.x) / opacityTrack.w;
                opacity = std::clamp(t, 0.0f, 1.0f);
                return true;
            }

            // Consume clicks inside the flyout rect generally
            SDL_Rect flyout = {flyoutX, flyoutY, flyoutW, 10000};
            if (pointInRect(mx, my, flyout))
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
            activeSize() = 1 + (int)(std::clamp(t, 0.0f, 1.0f) * 99);
            return true;
        }
        else if (e.type == SDL_MOUSEMOTION && draggingOpacity)
        {
            float t = (float)(e.motion.x - opacityTrack.x) / opacityTrack.w;
            opacity = std::clamp(t, 0.0f, 1.0f);
            return true;
        }
        return false;
    }

    void ToolPanel::render(SDL_Renderer *renderer, int windowH)
    {
        // Button column background
        SDL_SetRenderDrawColor(renderer, 40, 40, 43, 255);
        SDL_Rect panel = {0, topY, panelW, windowH - topY};
        SDL_RenderFillRect(renderer, &panel);

        SDL_Color textColor = {230, 230, 230, 255};
        int y = topY + 8;

        brushRect = {8, y, panelW - 16, buttonH};
        SDL_SetRenderDrawColor(renderer,
                               currentTool == Tool::Brush ? 80 : 55,
                               currentTool == Tool::Brush ? 80 : 55,
                               currentTool == Tool::Brush ? 90 : 58, 255);
        SDL_RenderFillRect(renderer, &brushRect);
        renderText(renderer, "Brush", brushRect.x + 6, brushRect.y + buttonH / 2 - 8, textColor);
        y += buttonH + 8;

        eraserRect = {8, y, panelW - 16, buttonH};
        SDL_SetRenderDrawColor(renderer,
                               currentTool == Tool::Eraser ? 80 : 55,
                               currentTool == Tool::Eraser ? 80 : 55,
                               currentTool == Tool::Eraser ? 90 : 58, 255);
        SDL_RenderFillRect(renderer, &eraserRect);
        renderText(renderer, "Erase", eraserRect.x + 6, eraserRect.y + buttonH / 2 - 8, textColor);

        if (flyoutOpen)
            renderFlyout(renderer, windowH);
    }

    void ToolPanel::renderFlyout(SDL_Renderer *renderer, int windowH)
    {
        SDL_Color textColor = {230, 230, 230, 255};

        // Full-height flyout, from the menu bar to the bottom
        flyoutY = topY;
        int flyoutH = windowH - topY;

        SDL_Rect flyout = {flyoutX, flyoutY, flyoutW, flyoutH};
        SDL_SetRenderDrawColor(renderer, 48, 48, 52, 255);
        SDL_RenderFillRect(renderer, &flyout);
        SDL_SetRenderDrawColor(renderer, 90, 90, 95, 255);
        SDL_RenderDrawRect(renderer, &flyout);

        int pad = 12;
        int x = flyoutX + pad;
        int y = flyoutY + pad;

        const char *title = (currentTool == Tool::Brush) ? "Brush" : "Eraser";
        renderText(renderer, title, x, y, textColor);
        y += 26;

        renderText(renderer, "Size", x, y, textColor);
        y += 22;

        // Preset dots
        // Preset dots — grid layout
        const int cols = 4;
        int rows = (presetCount + cols - 1) / cols; // ceil
        int cellW = (flyoutW - pad * 2) / cols;
        int cellH = 30;
        int maxDotR = cellH / 2 - 3; // largest dot fits inside a cell

        for (int i = 0; i < presetCount; ++i)
        {
            int col = i % cols;
            int row = i / cols;
            int cx = x + col * cellW + cellW / 2;
            int cy = y + row * cellH + cellH / 2;

            presetRects[i] = {cx - cellW / 2, cy - cellH / 2, cellW, cellH}; // full cell is clickable

            // Dot radius scales with the size value but never exceeds the cell
            float frac = (float)presets[i] / (float)presets[presetCount - 1]; // 0..1
            int radius = 2 + (int)(frac * (maxDotR - 2));

            bool selected = (activeSize() == presets[i]);
            SDL_SetRenderDrawColor(renderer, selected ? 120 : 200, selected ? 160 : 200, selected ? 255 : 200, 255);
            for (int dy = -radius; dy <= radius; ++dy)
            {
                int dx = (int)std::sqrt((double)(radius * radius - dy * dy));
                SDL_RenderDrawLine(renderer, cx - dx, cy + dy, cx + dx, cy + dy);
            }
        }
        y += rows * cellH + 12; // advance past the whole grid

        // Slider (horizontal)
        sliderTrack = {x, y, flyoutW - pad * 2, 6};
        sliderHit = {x, y - 10, flyoutW - pad * 2, 26}; // fat invisible grab zone
        SDL_SetRenderDrawColor(renderer, 25, 25, 27, 255);
        SDL_RenderFillRect(renderer, &sliderTrack);

        float t = (float)(activeSize() - 1) / 99.0f;
        int handleX = sliderTrack.x + (int)(t * sliderTrack.w);
        SDL_Rect handle = {handleX - 5, sliderTrack.y - 6, 10, 18};
        SDL_SetRenderDrawColor(renderer, 180, 180, 190, 255);
        SDL_RenderFillRect(renderer, &handle);
        y += 24;

        renderText(renderer, std::to_string(activeSize()) + " px", x, y, textColor);
        y += 26;

        // Brush-only: opacity slider + color picker.
        if (currentTool == Tool::Brush)
        {
            renderText(renderer, "Opacity", x, y, textColor);
            y += 22;

            opacityTrack = {x, y, flyoutW - pad * 2, 6};
            opacityHit = {x, y - 10, flyoutW - pad * 2, 26};
            SDL_SetRenderDrawColor(renderer, 25, 25, 27, 255);
            SDL_RenderFillRect(renderer, &opacityTrack);

            int ohx = opacityTrack.x + (int)(opacity * opacityTrack.w);
            SDL_Rect oh = {ohx - 5, opacityTrack.y - 6, 10, 18};
            SDL_SetRenderDrawColor(renderer, 180, 180, 190, 255);
            SDL_RenderFillRect(renderer, &oh);
            y += 24;

            renderText(renderer, std::to_string((int)(opacity * 100)) + "%", x, y, textColor);
            y += 26;

            renderText(renderer, "Color", x, y, textColor);
            y += 22;
            y = picker.render(renderer, x, y, flyoutW - pad * 2);
        }
    }
}