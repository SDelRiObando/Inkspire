#include "inkspire/brushsettingspanel.hpp"
#include <algorithm>
#include <string>

namespace Inkspire
{
    void BrushSettingsPanel::init(TTF_Font *font, ToolState *state)
    {
        this->font = font;
        this->state = state;
    }

    static bool pointInRect(int px, int py, const SDL_Rect &r)
    {
        return px >= r.x && px < r.x + r.w && py >= r.y && py < r.y + r.h;
    }

    void BrushSettingsPanel::renderText(SDL_Renderer *renderer, const std::string &text, int x, int y, SDL_Color color)
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

    bool BrushSettingsPanel::handleEvent(const SDL_Event &e, const PanelContext &ctx)
    {
        (void)ctx;
        if (!state)
            return false;

        Tool current = state->tool();

        // Scrollbar first. But not while actively dragging a slider — a slider
        // drag owns the mouse until release, so the scroll thumb can't steal it.
        if (!draggingSlider && !draggingOpacity &&
            scroll.handleEvent(e, bounds_, contentH_))
            return true;

        if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
        {
            int mx = e.button.x, my = e.button.y;

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

            // Consume clicks inside the panel generally.
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

    void BrushSettingsPanel::render(const PanelContext &ctx)
    {
        setBounds(ctx.bounds);
        SDL_Renderer *renderer = ctx.renderer;
        const SDL_Rect &b = ctx.bounds;
        if (!state)
            return;

        Tool current = state->tool();
        SDL_Color textColor = {230, 230, 230, 255};

        SDL_SetRenderDrawColor(renderer, 48, 48, 52, 255);
        SDL_RenderFillRect(renderer, &b);

        int pad = 12;
        int x = b.x + pad;
        int contentW = b.w - pad * 2;

        // Lay out in content space; screenY maps a content y to screen, applying
        // the scroll offset. Everything is clipped to the panel.
        SDL_RenderSetClipRect(renderer, &b);
        auto screenY = [&](int contentY)
        { return b.y + contentY - scroll.offset(); };
        int cy = pad;

        const char *title = (current == Tool::Brush) ? "Brush" : "Eraser";
        renderText(renderer, title, x, screenY(cy), textColor);
        cy += 26;

        renderText(renderer, "Size", x, screenY(cy), textColor);
        cy += 22;

        // Size slider (horizontal)
        int sTrackY = screenY(cy);
        sliderTrack = {x, sTrackY, contentW, 6};
        sliderHit = {x, sTrackY - 10, contentW, 26}; // fat invisible grab zone
        SDL_SetRenderDrawColor(renderer, 25, 25, 27, 255);
        SDL_RenderFillRect(renderer, &sliderTrack);

        float t = (float)(state->brushSize() - 1) / 99.0f;
        int handleX = sliderTrack.x + (int)(t * sliderTrack.w);
        SDL_Rect handle = {handleX - 5, sliderTrack.y - 6, 10, 18};
        SDL_SetRenderDrawColor(renderer, 180, 180, 190, 255);
        SDL_RenderFillRect(renderer, &handle);
        cy += 24;

        renderText(renderer, std::to_string(state->brushSize()) + " px", x, screenY(cy), textColor);
        cy += 26;

        // Opacity slider (brush only)
        if (current == Tool::Brush)
        {
            renderText(renderer, "Opacity", x, screenY(cy), textColor);
            cy += 22;

            int oTrackY = screenY(cy);
            opacityTrack = {x, oTrackY, contentW, 6};
            opacityHit = {x, oTrackY - 10, contentW, 26};
            SDL_SetRenderDrawColor(renderer, 25, 25, 27, 255);
            SDL_RenderFillRect(renderer, &opacityTrack);

            float op = state->brushOpacity();
            int ohx = opacityTrack.x + (int)(op * opacityTrack.w);
            SDL_Rect oh = {ohx - 5, opacityTrack.y - 6, 10, 18};
            SDL_SetRenderDrawColor(renderer, 180, 180, 190, 255);
            SDL_RenderFillRect(renderer, &oh);
            cy += 24;

            renderText(renderer, std::to_string((int)(op * 100)) + "%", x, screenY(cy), textColor);
            cy += 26;
        }
        else
        {
            // Eraser: opacity slider is inert, so reset its hit rect so stale
            // geometry from a previous brush render can't be clicked.
            opacityTrack = {0, 0, 0, 0};
            opacityHit = {0, 0, 0, 0};
        }

        SDL_RenderSetClipRect(renderer, nullptr);

        contentH_ = cy + pad;
        scroll.clamp(contentH_, b.h);
        scroll.renderIndicator(renderer, b, contentH_);
    }
}