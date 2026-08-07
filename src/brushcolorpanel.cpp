#include "inkspire/brushcolorpanel.hpp"

namespace Inkspire
{
    void BrushColorPanel::init(TTF_Font *font, ToolState *state)
    {
        this->font = font;
        this->state = state;
    }

    static bool pointInRect(int px, int py, const SDL_Rect &r)
    {
        return px >= r.x && px < r.x + r.w && py >= r.y && py < r.y + r.h;
    }

    void BrushColorPanel::renderText(SDL_Renderer *renderer, const std::string &text, int x, int y, SDL_Color color)
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

    bool BrushColorPanel::handleEvent(const SDL_Event &e, const PanelContext &ctx)
    {
        (void)ctx;
        if (!state)
            return false;

        // Only the brush uses color. The wheel drives the shared ColorPicker,
        // whose hit-testing is geometric (and already reflects the scrolled
        // positions from the last render), so forwarding the event is enough. It
        // goes first so an in-progress wheel/value drag keeps the mouse.
        if (state->tool() == Tool::Brush && state->colorPicker().handleEvent(e))
            return true;

        // Then the scrollbar: wheel over the panel, thumb drag, track click.
        if (scroll.handleEvent(e, bounds_, contentH_))
            return true;

        // Swallow stray clicks inside the panel body so they don't fall through
        // to the canvas.
        if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT &&
            pointInRect(e.button.x, e.button.y, bounds_))
            return true;

        return false;
    }

    void BrushColorPanel::render(const PanelContext &ctx)
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
            renderText(renderer, "Color (brush only)", x, b.y + pad, {150, 150, 150, 255});
            contentH_ = 0;
            scroll.clamp(0, b.h);
            return;
        }

        SDL_RenderSetClipRect(renderer, &b);
        int top = b.y + pad - scroll.offset();

        renderText(renderer, "Color", x, top, textColor);
        int y = top + 22;
        // Wheel + value slider + current-color preview, no swatch grid. render()
        // returns the y just below the drawn content.
        int belowY = state->colorPicker().render(renderer, x, y, contentW, /*showSwatches=*/false);

        SDL_RenderSetClipRect(renderer, nullptr);

        // Convert the absolute bottom back into content-space height.
        contentH_ = (belowY + pad) - (b.y - scroll.offset());
        scroll.clamp(contentH_, b.h);
        scroll.renderIndicator(renderer, b, contentH_);
    }
}