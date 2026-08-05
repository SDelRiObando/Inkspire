#pragma once

#include <SDL.h>
#include <SDL_ttf.h>
#include <string>
#include <vector>
#include <functional>

namespace Inkspire
{
    struct MenuItem
    {
        std::string label;
        std::function<void()> action;
    };

    class Menu
    {
    public:
        Menu(const std::string &title);

        void addItem(const std::string &label, std::function<void()> action);

        std::string title;
        std::vector<MenuItem> items;

        // Screen rect of the top-level title in the bar (set during layout)
        SDL_Rect titleRect{0, 0, 0, 0};
    };

    class MenuBar
    {
    public:
        bool init(TTF_Font *font);

        Menu &addMenu(const std::string &title);

        // Returns true if the event was consumed by the menu (so the canvas ignores it)
        bool handleEvent(const SDL_Event &e);
        void render(SDL_Renderer *renderer);

        int height() const { return barHeight; }

    private:
        void layout(SDL_Renderer *renderer);
        void renderText(SDL_Renderer *renderer, const std::string &text, int x, int y, SDL_Color color);
        int textWidth(const std::string &text);

        std::vector<Menu> menus;
        TTF_Font *font = nullptr;

        int barHeight = 28;
        int padding = 12;    // horizontal padding around each title
        int itemHeight = 24; // height of each dropdown row
        int dropdownWidth = 160;

        int openIndex = -1; // which menu is open, -1 = none
        bool laidOut = false;
    };
}