#pragma once

#include <string>
#include "canvas.hpp"
#include <vector>

namespace Inkspire
{
    // Saves the full editable document to a .inks file (a zip of project.json + layer PNGs).
    // Returns true on success.
    bool saveProject(Canvas &canvas, SDL_Renderer *renderer, const std::vector<SDL_Color> &swatches, const std::string &path);
    bool loadProject(Canvas &canvas, SDL_Renderer *renderer, std::vector<SDL_Color> &swatchesOut, const std::string &path);
}