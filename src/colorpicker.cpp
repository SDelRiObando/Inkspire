#include "inkspire/colorpicker.hpp"
#include <cmath>
#include <algorithm>

namespace Inkspire
{
    // HSV (h 0..360, s/v 0..1) -> RGB (0..255)
    static SDL_Color hsvToRgb(float h, float s, float v)
    {
        float c = v * s;
        float hp = h / 60.0f;
        float x = c * (1 - std::fabs(std::fmod(hp, 2.0f) - 1));
        float r = 0, g = 0, b = 0;
        if (hp < 1)
        {
            r = c;
            g = x;
            b = 0;
        }
        else if (hp < 2)
        {
            r = x;
            g = c;
            b = 0;
        }
        else if (hp < 3)
        {
            r = 0;
            g = c;
            b = x;
        }
        else if (hp < 4)
        {
            r = 0;
            g = x;
            b = c;
        }
        else if (hp < 5)
        {
            r = x;
            g = 0;
            b = c;
        }
        else
        {
            r = c;
            g = 0;
            b = x;
        }
        float m = v - c;
        return {(Uint8)((r + m) * 255), (Uint8)((g + m) * 255), (Uint8)((b + m) * 255), 255};
    }

    void ColorPicker::init(TTF_Font *font)
    {
        this->font = font;
        current = hsvToRgb(hue, sat, val);
    }

    void ColorPicker::rebuildWheel(SDL_Renderer *renderer)
    {
        if (wheelTex)
        {
            SDL_DestroyTexture(wheelTex);
            wheelTex = nullptr;
        }

        int size = wheelSize;
        wheelTex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                     SDL_TEXTUREACCESS_TARGET, size, size);
        SDL_SetTextureBlendMode(wheelTex, SDL_BLENDMODE_BLEND);

        SDL_SetRenderTarget(renderer, wheelTex);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);

        float cx = size / 2.0f;
        float cy = size / 2.0f;
        float radius = size / 2.0f;

        for (int py = 0; py < size; ++py)
        {
            for (int px = 0; px < size; ++px)
            {
                float dx = px - cx;
                float dy = py - cy;
                float dist = std::sqrt(dx * dx + dy * dy);
                if (dist > radius)
                    continue; // outside the circle

                float angle = std::atan2(dy, dx) * 180.0f / (float)M_PI;
                if (angle < 0)
                    angle += 360.0f;

                float s = dist / radius;
                SDL_Color c = hsvToRgb(angle, s, val);
                SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, 255);
                SDL_RenderDrawPoint(renderer, px, py);
            }
        }

        SDL_SetRenderTarget(renderer, nullptr);
        wheelBuiltForVal = (int)(val * 100);
    }

    void ColorPicker::pickFromWheel(int mx, int my)
    {
        float cx = wheelRect.x + wheelSize / 2.0f;
        float cy = wheelRect.y + wheelSize / 2.0f;
        float dx = mx - cx;
        float dy = my - cy;
        float dist = std::sqrt(dx * dx + dy * dy);
        float radius = wheelSize / 2.0f;

        float angle = std::atan2(dy, dx) * 180.0f / (float)M_PI;
        if (angle < 0)
            angle += 360.0f;

        hue = angle;
        sat = std::clamp(dist / radius, 0.0f, 1.0f);
        current = hsvToRgb(hue, sat, val);
    }

    bool ColorPicker::handleEvent(const SDL_Event &e)
    {
        if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
        {
            int mx = e.button.x, my = e.button.y;

            // Wheel
            float cx = wheelRect.x + wheelSize / 2.0f;
            float cy = wheelRect.y + wheelSize / 2.0f;
            float dist = std::sqrt((mx - cx) * (mx - cx) + (my - cy) * (my - cy));
            if (dist <= wheelSize / 2.0f)
            {
                draggingWheel = true;
                pickFromWheel(mx, my);
                return true;
            }

            // Value slider
            if (mx >= valSlider.x && mx < valSlider.x + valSlider.w &&
                my >= valSlider.y && my < valSlider.y + valSlider.h)
            {
                draggingVal = true;
                float t = (float)(my - valSlider.y) / valSlider.h;
                val = std::clamp(1.0f - t, 0.0f, 1.0f); // top = bright
                current = hsvToRgb(hue, sat, val);
                return true;
            }

            // Plus button: save current color to a new swatch
            if (mx >= plusRect.x && mx < plusRect.x + plusRect.w &&
                my >= plusRect.y && my < plusRect.y + plusRect.h)
            {
                swatches.push_back(current);
                return true;
            }

            // Swatches: click to select
            for (size_t i = 0; i < swatchRects.size(); ++i)
            {
                const SDL_Rect &r = swatchRects[i];
                if (mx >= r.x && mx < r.x + r.w && my >= r.y && my < r.y + r.h)
                {
                    current = swatches[i];
                    return true;
                }
            }
        }
        else if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT)
        {
            if (draggingWheel || draggingVal)
            {
                draggingWheel = false;
                draggingVal = false;
                return true;
            }
        }
        else if (e.type == SDL_MOUSEMOTION)
        {
            if (draggingWheel)
            {
                pickFromWheel(e.motion.x, e.motion.y);
                return true;
            }
            if (draggingVal)
            {
                float t = (float)(e.motion.y - valSlider.y) / valSlider.h;
                val = std::clamp(1.0f - t, 0.0f, 1.0f);
                current = hsvToRgb(hue, sat, val);
                return true;
            }
        }
        return false;
    }

    int ColorPicker::render(SDL_Renderer *renderer, int x, int y, int width)
    {
        // Wheel + value slider side by side
        int sliderW = 16;
        int gap = 10;
        wheelSize = width - sliderW - gap;
        if (wheelSize > 140)
            wheelSize = 140; // cap size
        wheelRect = {x, y, wheelSize, wheelSize};

        // Rebuild the wheel texture only when brightness changed (expensive per-pixel loop)
        if (!wheelTex || wheelBuiltForVal != (int)(val * 100))
            rebuildWheel(renderer);

        SDL_RenderCopy(renderer, wheelTex, nullptr, &wheelRect);

        // Marker on the wheel showing current hue/sat
        {
            float radius = wheelSize / 2.0f;
            float ang = hue * (float)M_PI / 180.0f;
            int markX = (int)(wheelRect.x + radius + std::cos(ang) * sat * radius);
            int markY = (int)(wheelRect.y + radius + std::sin(ang) * sat * radius);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_Rect m = {markX - 3, markY - 3, 6, 6};
            SDL_RenderDrawRect(renderer, &m);
        }

        // Value slider (vertical, bright at top)
        valSlider = {x + wheelSize + gap, y, sliderW, wheelSize};
        for (int i = 0; i < wheelSize; ++i)
        {
            float v = 1.0f - (float)i / wheelSize;
            SDL_Color c = hsvToRgb(hue, sat, v);
            SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, 255);
            SDL_RenderDrawLine(renderer, valSlider.x, y + i, valSlider.x + sliderW, y + i);
        }
        // slider handle
        int handleY = y + (int)((1.0f - val) * wheelSize);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_Rect vh = {valSlider.x - 2, handleY - 2, sliderW + 4, 4};
        SDL_RenderDrawRect(renderer, &vh);

        int cursorY = y + wheelSize + 12;

        // Current color preview + plus button
        SDL_Rect preview = {x, cursorY, 40, 24};
        SDL_SetRenderDrawColor(renderer, current.r, current.g, current.b, 255);
        SDL_RenderFillRect(renderer, &preview);
        SDL_SetRenderDrawColor(renderer, 90, 90, 95, 255);
        SDL_RenderDrawRect(renderer, &preview);

        plusRect = {x + 48, cursorY, 24, 24};
        SDL_SetRenderDrawColor(renderer, 60, 60, 65, 255);
        SDL_RenderFillRect(renderer, &plusRect);
        SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
        // draw a + sign
        SDL_RenderDrawLine(renderer, plusRect.x + 12, plusRect.y + 5, plusRect.x + 12, plusRect.y + 19);
        SDL_RenderDrawLine(renderer, plusRect.x + 5, plusRect.y + 12, plusRect.x + 19, plusRect.y + 12);

        cursorY += 34;

        // Swatches grid
        int sw = 22, sh = 22, sgap = 6, perRow = width / (sw + sgap);
        if (perRow < 1)
            perRow = 1;
        swatchRects.clear();
        for (size_t i = 0; i < swatches.size(); ++i)
        {
            int col = (int)i % perRow;
            int row = (int)i / perRow;
            SDL_Rect r = {x + col * (sw + sgap), cursorY + row * (sh + sgap), sw, sh};
            swatchRects.push_back(r);
            SDL_SetRenderDrawColor(renderer, swatches[i].r, swatches[i].g, swatches[i].b, 255);
            SDL_RenderFillRect(renderer, &r);
            SDL_SetRenderDrawColor(renderer, 90, 90, 95, 255);
            SDL_RenderDrawRect(renderer, &r);
        }
        int rows = swatches.empty() ? 0 : ((int)swatches.size() - 1) / perRow + 1;
        cursorY += rows * (sh + sgap) + 8;

        return cursorY;
    }
}