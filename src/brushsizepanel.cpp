#include "inkspire/brushsizepanel.hpp"
#include <cmath>

namespace Inkspire
{
    void BrushSizePanel::init(TTF_Font *font, ToolState *state)
    {
        this->font = font;
        this->state = state;
    }

    static bool pointInRect(int px, int py, const SDL_Rect &r)
    {
        return px >= r.x && px < r.x + r.w && py >= r.y && py < r.y + r.h;
    }

    void BrushSizePanel::renderText(SDL_Renderer *renderer, const std::string &text, int x, int y, SDL_Color color)
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

    bool BrushSizePanel::handleEvent(const SDL_Event &e, const PanelContext &ctx)
    {
        (void)ctx;
        if (!state)
            return false;

        // Scrollbar (wheel, thumb drag, track click) gets first crack. The
        // viewport is the whole panel; contentH is captured during render.
        if (scroll.handleEvent(e, bounds_, contentH_))
            return true;

        if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
        {
            int mx = e.button.x, my = e.button.y;
            for (int i = 0; i < presetCount; ++i)
            {
                if (pointInRect(mx, my, presetRects[i]))
                {
                    state->activeSize() = presets[i];
                    return true;
                }
            }
            // Consume clicks inside the panel generally.
            if (pointInRect(mx, my, bounds_))
                return true;
        }
        return false;
    }

    void BrushSizePanel::render(const PanelContext &ctx)
    {
        setBounds(ctx.bounds);
        SDL_Renderer *renderer = ctx.renderer;
        const SDL_Rect &b = ctx.bounds;
        if (!state)
            return;

        SDL_Color textColor = {230, 230, 230, 255};

        SDL_SetRenderDrawColor(renderer, 48, 48, 52, 255);
        SDL_RenderFillRect(renderer, &b);

        int pad = 12;
        int x = b.x + pad;
        int contentW = b.w - pad * 2;

        // Lay out in content space (y measured from the panel top, 0-based),
        // then draw each element shifted up by the scroll offset and clipped to
        // the panel. cy() maps a content y to a screen y.
        SDL_RenderSetClipRect(renderer, &b);
        int cy0 = 0;
        auto screenY = [&](int contentY)
        { return b.y + contentY - scroll.offset(); };

        cy0 = pad;
        renderText(renderer, "Size", x, screenY(cy0), textColor);
        cy0 += 22;

        // Preset dots — grid layout.
        const int cols = 4;
        int rows = (presetCount + cols - 1) / cols; // ceil
        int cellW = contentW / cols;
        int cellH = 30;
        int maxDotR = cellH / 2 - 3; // largest dot fits inside a cell
        int gridTop = cy0;

        for (int i = 0; i < presetCount; ++i)
        {
            int col = i % cols;
            int row = i / cols;
            int cx = x + col * cellW + cellW / 2;
            int cyPix = screenY(gridTop + row * cellH) + cellH / 2;

            presetRects[i] = {cx - cellW / 2, cyPix - cellH / 2, cellW, cellH}; // full cell clickable

            float frac = (float)presets[i] / (float)presets[presetCount - 1]; // 0..1
            int radius = 2 + (int)(frac * (maxDotR - 2));

            bool selected = (state->brushSize() == presets[i]);
            SDL_SetRenderDrawColor(renderer, selected ? 120 : 200, selected ? 160 : 200, selected ? 255 : 200, 255);
            for (int dy = -radius; dy <= radius; ++dy)
            {
                int dx = (int)std::sqrt((double)(radius * radius - dy * dy));
                SDL_RenderDrawLine(renderer, cx - dx, cyPix + dy, cx + dx, cyPix + dy);
            }
        }
        cy0 = gridTop + rows * cellH + pad;

        SDL_RenderSetClipRect(renderer, nullptr);

        contentH_ = cy0; // total content height in content space
        scroll.clamp(contentH_, b.h);
        scroll.renderIndicator(renderer, b, contentH_);
    }
}