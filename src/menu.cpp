#include "inkspire/menu.hpp"
#include <iostream>

namespace Inkspire
{
    Menu::Menu(const std::string &title) : title(title) {}

    void Menu::addItem(const std::string &label, std::function<void()> action)
    {
        items.push_back({label, std::move(action)});
    }

    bool MenuBar::init(TTF_Font *font)
    {
        this->font = font;
        return font != nullptr;
    }

    Menu &MenuBar::addMenu(const std::string &title)
    {
        menus.emplace_back(title);
        laidOut = false; // force re-layout next render
        return menus.back();
    }

    int MenuBar::textWidth(const std::string &text)
    {
        int w = 0, h = 0;
        TTF_SizeText(font, text.c_str(), &w, &h);
        return w;
    }

    void MenuBar::layout(SDL_Renderer *)
    {
        int x = 0;
        for (auto &menu : menus)
        {
            int w = textWidth(menu.title) + padding * 2;
            menu.titleRect = {x, 0, w, barHeight};
            x += w;
        }
        laidOut = true;
    }

    void MenuBar::renderText(SDL_Renderer *renderer, const std::string &text, int x, int y, SDL_Color color)
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

    static bool pointInRect(int px, int py, const SDL_Rect &r)
    {
        return px >= r.x && px < r.x + r.w && py >= r.y && py < r.y + r.h;
    }

    bool MenuBar::handleEvent(const SDL_Event &e)
    {
        if (!laidOut)
            return false; // not positioned yet; ignore until first render

        if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
        {
            int mx = e.button.x, my = e.button.y;

            // Click on a top-level title: toggle it open/closed
            for (size_t i = 0; i < menus.size(); ++i)
            {
                if (pointInRect(mx, my, menus[i].titleRect))
                {
                    openIndex = (openIndex == (int)i) ? -1 : (int)i;
                    return true;
                }
            }

            // Click inside the open dropdown: fire the item's action
            if (openIndex >= 0)
            {
                Menu &m = menus[openIndex];
                int dropX = m.titleRect.x;
                int dropY = barHeight;
                for (size_t j = 0; j < m.items.size(); ++j)
                {
                    SDL_Rect itemRect = {dropX, dropY + (int)j * itemHeight, dropdownWidth, itemHeight};
                    if (pointInRect(mx, my, itemRect))
                    {
                        if (m.items[j].action)
                            m.items[j].action();
                        openIndex = -1;
                        return true;
                    }
                }
            }

            // Click anywhere else: close any open menu
            if (openIndex >= 0)
            {
                openIndex = -1;
                return true; // consume the click that closed the menu
            }

            // Click landed in the bar area but not on a title
            if (my < barHeight)
                return true;
        }
        else if (e.type == SDL_MOUSEMOTION && openIndex >= 0)
        {
            // With a menu open, hovering another title switches to it
            int mx = e.motion.x, my = e.motion.y;
            for (size_t i = 0; i < menus.size(); ++i)
            {
                if (pointInRect(mx, my, menus[i].titleRect))
                {
                    openIndex = (int)i;
                    break;
                }
            }
        }

        return false;
    }

    void MenuBar::render(SDL_Renderer *renderer)
    {
        if (!laidOut)
            layout(renderer);

        // Bar background
        SDL_SetRenderDrawColor(renderer, 45, 45, 48, 255);
        SDL_Rect bar = {0, 0, 10000, barHeight};
        SDL_RenderFillRect(renderer, &bar);

        SDL_Color textColor = {230, 230, 230, 255};

        // Top-level titles
        for (size_t i = 0; i < menus.size(); ++i)
        {
            Menu &m = menus[i];
            if ((int)i == openIndex)
            {
                SDL_SetRenderDrawColor(renderer, 70, 70, 75, 255);
                SDL_RenderFillRect(renderer, &m.titleRect);
            }
            renderText(renderer, m.title, m.titleRect.x + padding,
                       (barHeight - TTF_FontHeight(font)) / 2, textColor);
        }

        // Open dropdown, drawn last so it sits on top
        if (openIndex >= 0)
        {
            Menu &m = menus[openIndex];
            int dropX = m.titleRect.x;
            int dropY = barHeight;
            int dropH = (int)m.items.size() * itemHeight;

            SDL_SetRenderDrawColor(renderer, 55, 55, 58, 255);
            SDL_Rect panel = {dropX, dropY, dropdownWidth, dropH};
            SDL_RenderFillRect(renderer, &panel);

            // Hover highlight
            int mx, my;
            SDL_GetMouseState(&mx, &my);

            for (size_t j = 0; j < m.items.size(); ++j)
            {
                SDL_Rect itemRect = {dropX, dropY + (int)j * itemHeight, dropdownWidth, itemHeight};
                if (pointInRect(mx, my, itemRect))
                {
                    SDL_SetRenderDrawColor(renderer, 80, 80, 90, 255);
                    SDL_RenderFillRect(renderer, &itemRect);
                }
                renderText(renderer, m.items[j].label, itemRect.x + padding,
                           itemRect.y + (itemHeight - TTF_FontHeight(font)) / 2, textColor);
            }

            // Panel border
            SDL_SetRenderDrawColor(renderer, 90, 90, 95, 255);
            SDL_RenderDrawRect(renderer, &panel);
        }
    }
}