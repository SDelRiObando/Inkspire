#include "inkspire/layerspanel.hpp"

namespace Inkspire
{
    void LayersPanel::init(TTF_Font *font, int menuBarHeight)
    {
        this->font = font;
        this->topY = menuBarHeight;
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

    bool LayersPanel::handleEvent(const SDL_Event &e, Canvas &canvas, int windowW)
    {
        if (e.type != SDL_MOUSEBUTTONDOWN || e.button.button != SDL_BUTTON_LEFT)
            return false;

        int mx = e.button.x, my = e.button.y;
        int panelX = windowW - panelW;
        if (mx < panelX)
            return false; // click not in this panel

        // Add / delete buttons (in the header)
        if (inRect(mx, my, addRect))
        {
            canvas.addLayer();
            return true;
        }
        if (inRect(mx, my, delRect))
        {
            canvas.deleteLayer(canvas.getActiveLayer());
            return true;
        }

        // Layer rows — drawn top-to-bottom but layers are stored bottom-to-top,
        // so the topmost row is the LAST layer in the vector.
        const auto &layers = canvas.getLayers();
        int n = (int)layers.size();
        int y = topY + headerH;
        for (int row = 0; row < n; ++row)
        {
            int layerIndex = n - 1 - row; // top row = top layer = last in vector
            SDL_Rect rowRect = {panelX, y, panelW, rowH};
            if (inRect(mx, my, rowRect))
            {
                // Left ~28px = eye toggle; rest = select
                SDL_Rect eye = {panelX + 6, y + rowH / 2 - 8, 16, 16};
                if (inRect(mx, my, eye))
                    canvas.toggleVisible(layerIndex);
                else
                    canvas.setActiveLayer(layerIndex); // background rejected internally
                return true;
            }
            y += rowH;
        }

        // Consume any other click in the panel column
        return true;
    }

    void LayersPanel::render(SDL_Renderer *renderer, Canvas &canvas, int windowW, int windowH)
    {
        int panelX = windowW - panelW;

        // Panel background
        SDL_SetRenderDrawColor(renderer, 40, 40, 43, 255);
        SDL_Rect panel = {panelX, topY, panelW, windowH - topY};
        SDL_RenderFillRect(renderer, &panel);

        SDL_Color textColor = {230, 230, 230, 255};
        renderText(renderer, "Layers", panelX + 10, topY + 8, textColor);

        // Add / delete buttons
        addRect = {panelX + panelW - 56, topY + 6, 22, 20};
        delRect = {panelX + panelW - 30, topY + 6, 22, 20};
        SDL_SetRenderDrawColor(renderer, 60, 60, 65, 255);
        SDL_RenderFillRect(renderer, &addRect);
        SDL_RenderFillRect(renderer, &delRect);
        SDL_SetRenderDrawColor(renderer, 210, 210, 210, 255);
        // "+" on add
        SDL_RenderDrawLine(renderer, addRect.x + 11, addRect.y + 4, addRect.x + 11, addRect.y + 16);
        SDL_RenderDrawLine(renderer, addRect.x + 5, addRect.y + 10, addRect.x + 17, addRect.y + 10);
        // "-" on delete
        SDL_RenderDrawLine(renderer, delRect.x + 5, delRect.y + 10, delRect.x + 17, delRect.y + 10);

        // Layer rows, top layer first
        const auto &layers = canvas.getLayers();
        int n = (int)layers.size();
        int active = canvas.getActiveLayer();
        int y = topY + headerH;
        for (int row = 0; row < n; ++row)
        {
            int layerIndex = n - 1 - row;
            const Layer &L = layers[layerIndex];

            SDL_Rect rowRect = {panelX, y, panelW, rowH};
            // Highlight the active layer
            SDL_SetRenderDrawColor(renderer, layerIndex == active ? 70 : 48,
                                   layerIndex == active ? 70 : 48,
                                   layerIndex == active ? 80 : 52, 255);
            SDL_RenderFillRect(renderer, &rowRect);
            SDL_SetRenderDrawColor(renderer, 30, 30, 33, 255);
            SDL_RenderDrawRect(renderer, &rowRect);

            // Eye icon (filled if visible, outline if hidden)
            SDL_Rect eye = {panelX + 6, y + rowH / 2 - 8, 16, 16};
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
            renderText(renderer, L.name, panelX + 30, y + rowH / 2 - 8, textColor);

            y += rowH;
        }
    }
}