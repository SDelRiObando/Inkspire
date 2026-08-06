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

        // Human-readable label shown in the panel header once docking exists.
        virtual const char *title() const = 0;

        // Draw into ctx.bounds. Implementations should also cache the bounds
        // (via setBounds) so handleEvent can hit-test against the same rect.
        virtual void render(const PanelContext &ctx) = 0;

        // Handle an event. Return true if it was consumed (so nothing else
        // downstream reacts to it). Hit-testing should use the cached bounds.
        virtual bool handleEvent(const SDL_Event &e, const PanelContext &ctx) = 0;

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