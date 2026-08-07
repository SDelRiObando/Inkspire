#pragma once

#include <SDL.h>

namespace Inkspire
{
    class Canvas; // forward-declare; panels that need it pull from the context

    // Everything a panel might need to render and hit-test itself. The layout
    // owns the panel's rectangle and passes it in here — panels never compute
    // their own screen position. This inversion is what makes docking possible:
    // the same panel can be handed any rectangle, anywhere, and render into it.
    struct PanelContext
    {
        SDL_Renderer *renderer = nullptr;
        SDL_Rect bounds{0, 0, 0, 0}; // where this panel must draw / hit-test
        Canvas *canvas = nullptr;    // shared app state (may be null for panels
                                     // that don't touch it)
    };

    // Base class for every dockable panel (Layers, Tools, Navigator, ...).
    // Concrete panels keep their own domain API (e.g. ToolPanel::brushColor())
    // as ordinary non-virtual methods; this interface governs only layout,
    // rendering, and event routing.
    class Panel
    {
    public:
        virtual ~Panel() = default;

        // Stable identity, used later for layout serialization ("layers",
        // "tools", "navigator"). Must not change across runs.
        virtual const char *id() const = 0;

        // Human-readable label shown on this panel's tab.
        virtual const char *title() const = 0;

        // The category this panel belongs to. A dock column's header shows the
        // category name(s) of the panels it holds — a single category ("Tool
        // Options") when they all agree, or a combined name ("Layers + Tool
        // Options") when a column mixes categories. Panels that are their own
        // category (Layers, Tools) just return their title; the tool-option
        // panels (size, settings, color, palette) all return "Tool Options".
        virtual const char *category() const { return title(); }

        // Draw into ctx.bounds. Implementations should also cache the bounds
        // (via setBounds) so handleEvent can hit-test against the same rect.
        virtual void render(const PanelContext &ctx) = 0;

        // Handle an event. Return true if it was consumed (so nothing else
        // downstream reacts to it). Hit-testing should use the cached bounds.
        virtual bool handleEvent(const SDL_Event &e, const PanelContext &ctx) = 0;

        // Optional per-panel action buttons drawn into this panel's title header
        // (right-aligned), e.g. Layers' add/delete. App draws the header chrome
        // and the title, then calls this with `headerRect` so the panel can add
        // its own controls. Return the left x of the drawn buttons so App can
        // keep the title text from overlapping them (return headerRect right
        // edge when nothing is drawn — the default). Cache rects for hit-testing.
        virtual int renderHeaderActions(SDL_Renderer *renderer, const SDL_Rect &headerRect,
                                        const PanelContext &ctx)
        {
            (void)renderer;
            (void)ctx;
            return headerRect.x + headerRect.w;
        }

        // Handle an event over this panel's title header (the action buttons).
        // Called before the header press arms a redock drag, so a button click
        // isn't swallowed by dragging. Return true if consumed. Default: ignore.
        virtual bool handleHeaderEvent(const SDL_Event &e, const SDL_Rect &headerRect,
                                       const PanelContext &ctx)
        {
            (void)e;
            (void)headerRect;
            (void)ctx;
            return false;
        }

        // Size hints the layout may honor or override. Width for side docks,
        // height for stacked docks. 0 means "no preference / fill available".
        virtual int preferredWidth() const { return 200; }
        virtual int preferredHeight() const { return 0; }

        const SDL_Rect &bounds() const { return bounds_; }

    protected:
        void setBounds(const SDL_Rect &r) { bounds_ = r; }
        SDL_Rect bounds_{0, 0, 0, 0}; // last assigned rectangle
    };
}