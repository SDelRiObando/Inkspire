#pragma once

#include <SDL.h>
#include <algorithm>

namespace Inkspire
{
    // A small, reusable vertical-scroll helper for panels whose content can
    // exceed their visible area (layers list now; swatch lists, history, etc.
    // later). A panel owns one of these, tells it the content/viewport heights
    // each frame, offsets its own drawing by offset(), and forwards events via
    // handleEvent(). The helper draws a thin position indicator and supports
    // wheel scrolling, thumb dragging, and click-to-jump on the track.
    class ScrollState
    {
    public:
        int offset() const { return offset_; }

        // Keep the offset within [0, max(0, contentH - viewH)]. Call each render
        // once the panel knows how tall its content and viewport are. Remembers
        // both so event handling can map positions and clamp.
        void clamp(int contentH, int viewH)
        {
            contentH_ = contentH;
            viewH_ = viewH;
            offset_ = std::clamp(offset_, 0, maxOffset());
        }

        // Unified event handling: wheel (when over viewRect), thumb drag, and
        // track click-to-jump (within the grab column). Returns true if the
        // event was consumed. `viewRect` is the scrollable viewport; `contentH`
        // is the full content height. Call clamp() first each frame.
        bool handleEvent(const SDL_Event &e, const SDL_Rect &viewRect, int contentH)
        {
            contentH_ = contentH;
            viewH_ = viewRect.h;
            if (contentH <= viewRect.h) // nothing to scroll
            {
                dragging_ = false;
                return false;
            }

            if (e.type == SDL_MOUSEWHEEL)
            {
                int mx, my;
                SDL_GetMouseState(&mx, &my);
                if (inRect(mx, my, viewRect))
                {
                    offset_ = std::clamp(offset_ - e.wheel.y * wheelStep_, 0, maxOffset());
                    return true;
                }
                return false;
            }

            if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
            {
                int mx = e.button.x, my = e.button.y;
                if (!inGrabColumn(mx, my, viewRect))
                    return false;

                SDL_Rect thumb = thumbRect(viewRect);
                if (my >= thumb.y && my < thumb.y + thumb.h)
                {
                    // Grabbed the thumb: begin dragging, remember grab offset.
                    dragging_ = true;
                    dragGrabDY_ = my - thumb.y;
                }
                else
                {
                    // Clicked the track above/below the thumb: jump so the thumb
                    // centers on the click, then allow immediate dragging.
                    dragging_ = true;
                    dragGrabDY_ = thumb.h / 2;
                    setOffsetFromThumbTop(my - dragGrabDY_, viewRect);
                }
                return true;
            }

            if (e.type == SDL_MOUSEMOTION && dragging_)
            {
                setOffsetFromThumbTop(e.motion.y - dragGrabDY_, viewRect);
                return true;
            }

            if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT && dragging_)
            {
                dragging_ = false;
                return true;
            }

            return false;
        }

        // Draw a thin position indicator along the right edge of `viewRect`,
        // only when content overflows.
        void renderIndicator(SDL_Renderer *renderer, const SDL_Rect &viewRect, int contentH) const
        {
            if (contentH <= viewRect.h || viewRect.h <= 0)
                return;

            int trackX = viewRect.x + viewRect.w - barW_ - 2;

            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 30, 30, 33, 180);
            SDL_Rect track = {trackX, viewRect.y, barW_, viewRect.h};
            SDL_RenderFillRect(renderer, &track);

            SDL_Rect thumb = thumbRect(viewRect);
            SDL_SetRenderDrawColor(renderer, dragging_ ? 170 : 120,
                                   dragging_ ? 170 : 120, dragging_ ? 185 : 130, 255);
            SDL_RenderFillRect(renderer, &thumb);
        }

        void reset()
        {
            offset_ = 0;
            dragging_ = false;
        }

    private:
        static bool inRect(int px, int py, const SDL_Rect &r)
        {
            return px >= r.x && px < r.x + r.w && py >= r.y && py < r.y + r.h;
        }

        int maxOffset() const
        {
            int m = contentH_ - viewH_;
            return m < 0 ? 0 : m;
        }

        // The visible thumb rectangle for the current offset.
        SDL_Rect thumbRect(const SDL_Rect &viewRect) const
        {
            int trackX = viewRect.x + viewRect.w - barW_ - 2;
            float visibleFrac = (float)viewRect.h / (float)contentH_;
            int thumbH = std::max(20, (int)(viewRect.h * visibleFrac));
            int mo = maxOffset();
            float t = mo > 0 ? (float)offset_ / (float)mo : 0.0f;
            int thumbY = viewRect.y + (int)(t * (viewRect.h - thumbH));
            return {trackX, thumbY, barW_, thumbH};
        }

        // The wide invisible grab column around the thin visible bar.
        bool inGrabColumn(int px, int py, const SDL_Rect &viewRect) const
        {
            int trackX = viewRect.x + viewRect.w - barW_ - 2;
            SDL_Rect grab = {trackX - grabPad_, viewRect.y,
                             barW_ + grabPad_ * 2, viewRect.h};
            return inRect(px, py, grab);
        }

        // Given a desired thumb-top y (in screen space), set the scroll offset.
        void setOffsetFromThumbTop(int thumbTopY, const SDL_Rect &viewRect)
        {
            float visibleFrac = (float)viewRect.h / (float)contentH_;
            int thumbH = std::max(20, (int)(viewRect.h * visibleFrac));
            int travel = viewRect.h - thumbH; // pixels the thumb can move
            if (travel <= 0)
            {
                offset_ = 0;
                return;
            }
            float t = (float)(thumbTopY - viewRect.y) / (float)travel;
            t = std::clamp(t, 0.0f, 1.0f);
            offset_ = (int)(t * maxOffset());
        }

        int offset_ = 0;
        int contentH_ = 0;
        int viewH_ = 0;
        bool dragging_ = false;
        int dragGrabDY_ = 0; // cursor offset within the thumb at grab time

        static constexpr int barW_ = 4;     // thin visible bar
        static constexpr int grabPad_ = 10; // invisible grab zone on each side
        static constexpr int wheelStep_ = 40;
    };
}