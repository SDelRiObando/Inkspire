#include "inkspire/layerspanel.hpp"

namespace Inkspire
{
    void LayersPanel::init(TTF_Font *font)
    {
        this->font = font;
    }

    static bool inRect(int px, int py, const SDL_Rect &r)
    {
        return px >= r.x && px < r.x + r.w && py >= r.y && py < r.y + r.h;
    }

    void LayersPanel::renderText(SDL_Renderer *renderer, const std::string &text, int x, int y, SDL_Color color)
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

    int LayersPanel::renderHeaderActions(SDL_Renderer *renderer, const SDL_Rect &h,
                                         const PanelContext &ctx)
    {
        (void)ctx;
        // Add / delete buttons, right-aligned in the title header. Inset a little
        // from the right edge so they clear the column collapse toggle.
        int bw = 22, bh = 18;
        int gap = 4;
        int top = h.y + (h.h - bh) / 2;
        int rightInset = 6;
        delRect = {h.x + h.w - rightInset - bw, top, bw, bh};
        addRect = {delRect.x - gap - bw, top, bw, bh};

        SDL_SetRenderDrawColor(renderer, 60, 60, 65, 255);
        SDL_RenderFillRect(renderer, &addRect);
        SDL_RenderFillRect(renderer, &delRect);
        SDL_SetRenderDrawColor(renderer, 210, 210, 210, 255);
        // "+" on add
        SDL_RenderDrawLine(renderer, addRect.x + 11, addRect.y + 4, addRect.x + 11, addRect.y + 14);
        SDL_RenderDrawLine(renderer, addRect.x + 5, addRect.y + 9, addRect.x + 17, addRect.y + 9);
        // "-" on delete
        SDL_RenderDrawLine(renderer, delRect.x + 5, delRect.y + 9, delRect.x + 17, delRect.y + 9);

        return addRect.x; // title text must stop before here
    }

    bool LayersPanel::handleHeaderEvent(const SDL_Event &e, const SDL_Rect &h,
                                        const PanelContext &ctx)
    {
        (void)h;
        if (!ctx.canvas)
            return false;
        if (e.type != SDL_MOUSEBUTTONDOWN || e.button.button != SDL_BUTTON_LEFT)
            return false;
        int mx = e.button.x, my = e.button.y;
        Canvas &canvas = *ctx.canvas;
        if (inRect(mx, my, addRect))
        {
            canvas.pushUndo();
            canvas.addLayer();
            return true;
        }
        if (inRect(mx, my, delRect))
        {
            canvas.pushUndo();
            canvas.deleteLayer(canvas.getActiveLayer());
            return true;
        }
        return false;
    }

    bool LayersPanel::handleEvent(const SDL_Event &e, const PanelContext &ctx)
    {
        if (!ctx.canvas)
            return false;
        Canvas &canvas = *ctx.canvas;
        const SDL_Rect &b = bounds_; // content rect, set during render

        // The scrollbar (wheel, thumb drag, track click) gets first crack. The
        // whole content rect is the scroll viewport now that the header is drawn
        // by the column chrome above this rect.
        int contentH = (int)canvas.getLayers().size() * rowH;
        if (scroll.handleEvent(e, b, contentH))
            return true;

        if (e.type != SDL_MOUSEBUTTONDOWN || e.button.button != SDL_BUTTON_LEFT)
            return false;

        int mx = e.button.x, my = e.button.y;
        if (!inRect(mx, my, b))
            return false; // click not in this panel

        // Layer rows — drawn top-to-bottom but layers are stored bottom-to-top,
        // so the topmost row is the LAST layer in the vector. Rows are shifted by
        // the scroll offset.
        const auto &layers = canvas.getLayers();
        int n = (int)layers.size();
        int y = b.y - scroll.offset();
        for (int row = 0; row < n; ++row)
        {
            int layerIndex = n - 1 - row; // top row = top layer = last in vector
            SDL_Rect rowRect = {b.x, y, b.w, rowH};
            if (inRect(mx, my, rowRect))
            {
                // Left ~28px = eye toggle; rest = select
                SDL_Rect eye = {b.x + 6, y + rowH / 2 - 8, 16, 16};
                if (inRect(mx, my, eye))
                {
                    canvas.pushUndo();
                    canvas.toggleVisible(layerIndex);
                }
                else
                    canvas.setActiveLayer(layerIndex); // background rejected internally
                return true;
            }
            y += rowH;
        }

        // Consume any other click inside the panel
        return true;
    }

    void LayersPanel::render(const PanelContext &ctx)
    {
        setBounds(ctx.bounds);
        SDL_Renderer *renderer = ctx.renderer;
        const SDL_Rect &b = ctx.bounds; // content rect (below the column headers)
        if (!ctx.canvas)
            return;
        Canvas &canvas = *ctx.canvas;

        // Panel background
        SDL_SetRenderDrawColor(renderer, 40, 40, 43, 255);
        SDL_RenderFillRect(renderer, &b);

        SDL_Color textColor = {230, 230, 230, 255};

        // Layer rows fill the whole content rect now — the "Layers" title and the
        // add/delete buttons live on the column-provided header above this rect.
        const auto &layers = canvas.getLayers();
        int n = (int)layers.size();
        int active = canvas.getActiveLayer();

        int contentH = n * rowH;
        scroll.clamp(contentH, b.h);

        SDL_RenderSetClipRect(renderer, &b);
        int y = b.y - scroll.offset();
        for (int row = 0; row < n; ++row)
        {
            int layerIndex = n - 1 - row;
            const Layer &L = layers[layerIndex];

            // Skip rows entirely outside the viewport (cheap cull).
            if (y + rowH < b.y || y > b.y + b.h)
            {
                y += rowH;
                continue;
            }

            SDL_Rect rowRect = {b.x, y, b.w, rowH};
            // Highlight the active layer
            SDL_SetRenderDrawColor(renderer, layerIndex == active ? 70 : 48,
                                   layerIndex == active ? 70 : 48,
                                   layerIndex == active ? 80 : 52, 255);
            SDL_RenderFillRect(renderer, &rowRect);
            SDL_SetRenderDrawColor(renderer, 30, 30, 33, 255);
            SDL_RenderDrawRect(renderer, &rowRect);

            // Eye icon (filled if visible, outline if hidden)
            SDL_Rect eye = {b.x + 6, y + rowH / 2 - 8, 16, 16};
            if (L.visible)
            {
                SDL_SetRenderDrawColor(renderer, 200, 200, 210, 255);
                SDL_RenderFillRect(renderer, &eye);
            }
            else
            {
                SDL_SetRenderDrawColor(renderer, 110, 110, 115, 255);
                SDL_RenderDrawRect(renderer, &eye);
            }

            // Layer name
            renderText(renderer, L.name, b.x + 30, y + rowH / 2 - 8, textColor);

            y += rowH;
        }
        SDL_RenderSetClipRect(renderer, nullptr);

        // Thin position indicator on the right edge when the list overflows.
        scroll.renderIndicator(renderer, b, contentH);
    }
}