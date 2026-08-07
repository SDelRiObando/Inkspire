#include "inkspire/colorpalettepanel.hpp"

namespace Inkspire
{
    void ColorPalettePanel::init(TTF_Font *font, ToolState *state)
    {
        this->font = font;
        this->state = state;
    }

    static bool pointInRect(int px, int py, const SDL_Rect &r)
    {
        return px >= r.x && px < r.x + r.w && py >= r.y && py < r.y + r.h;
    }

    void ColorPalettePanel::renderText(SDL_Renderer *renderer, const std::string &text, int x, int y, SDL_Color color)
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

    bool ColorPalettePanel::handleEvent(const SDL_Event &e, const PanelContext &ctx)
    {
        (void)ctx;
        if (!state)
            return false;

        // The + button and swatch clicks are handled inside the shared
        // ColorPicker by geometry (already scrolled), so forwarding is enough.
        if (state->tool() == Tool::Brush && state->colorPicker().handleEvent(e))
            return true;

        if (scroll.handleEvent(e, bounds_, contentH_))
            return true;

        if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT &&
            pointInRect(e.button.x, e.button.y, bounds_))
            return true;

        return false;
    }

    void ColorPalettePanel::render(const PanelContext &ctx)
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

        if (state->tool() != Tool::Brush)
        {
            renderText(renderer, "Palette (brush only)", x, b.y + pad, {150, 150, 150, 255});
            contentH_ = 0;
            scroll.clamp(0, b.h);
            return;
        }

        SDL_RenderSetClipRect(renderer, &b);
        int top = b.y + pad - scroll.offset();

        renderText(renderer, "Palette", x, top, textColor);
        int belowY = state->colorPicker().renderSwatches(renderer, x, top + 22, contentW);

        SDL_RenderSetClipRect(renderer, nullptr);

        contentH_ = (belowY + pad) - (b.y - scroll.offset());
        scroll.clamp(contentH_, b.h);
        scroll.renderIndicator(renderer, b, contentH_);
    }
}