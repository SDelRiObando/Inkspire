#include "inkspire/project.hpp"

#include <SDL_image.h>
#include <miniz.h>
#include <nlohmann/json.hpp>
#include <vector>
#include <cstring>
#include <iostream>

using json = nlohmann::json;

namespace Inkspire
{
    // ---- Custom SDL_RWops that writes into a growable std::vector ----
    // We pass a pointer to this struct as the RWops' "hidden" data.
    struct MemWriter
    {
        std::vector<unsigned char> bytes;
        Sint64 pos = 0;
    };

    static Sint64 mem_size(SDL_RWops *ctx)
    {
        MemWriter *m = static_cast<MemWriter *>(ctx->hidden.unknown.data1);
        return (Sint64)m->bytes.size();
    }

    static Sint64 mem_seek(SDL_RWops *ctx, Sint64 offset, int whence)
    {
        MemWriter *m = static_cast<MemWriter *>(ctx->hidden.unknown.data1);
        Sint64 newPos = 0;
        if (whence == RW_SEEK_SET)
            newPos = offset;
        else if (whence == RW_SEEK_CUR)
            newPos = m->pos + offset;
        else if (whence == RW_SEEK_END)
            newPos = (Sint64)m->bytes.size() + offset;
        if (newPos < 0)
            return -1;
        m->pos = newPos;
        return m->pos;
    }

    static size_t mem_write(SDL_RWops *ctx, const void *ptr, size_t sz, size_t num)
    {
        MemWriter *m = static_cast<MemWriter *>(ctx->hidden.unknown.data1);
        size_t total = sz * num;
        // Ensure the vector is big enough to hold from pos to pos+total
        if (m->pos + (Sint64)total > (Sint64)m->bytes.size())
            m->bytes.resize(m->pos + total);
        std::memcpy(m->bytes.data() + m->pos, ptr, total);
        m->pos += total;
        return num;
    }

    static size_t mem_read(SDL_RWops *, void *, size_t, size_t) { return 0; } // write-only

    static int mem_close(SDL_RWops *ctx)
    {
        if (ctx)
            SDL_FreeRW(ctx);
        return 0;
    }

    static SDL_RWops *makeMemWriter(MemWriter *m)
    {
        SDL_RWops *rw = SDL_AllocRW();
        if (!rw)
            return nullptr;
        rw->size = mem_size;
        rw->seek = mem_seek;
        rw->read = mem_read;
        rw->write = mem_write;
        rw->close = mem_close;
        rw->type = SDL_RWOPS_UNKNOWN;
        rw->hidden.unknown.data1 = m;
        return rw;
    }

    // Encode a surface to PNG bytes entirely in memory.
    static bool surfaceToPngBytes(SDL_Surface *surf, std::vector<unsigned char> &out)
    {
        MemWriter m;
        SDL_RWops *rw = makeMemWriter(&m);
        if (!rw)
            return false;

        // IMG_SavePNG_RW writes PNG data through our callbacks. freesrc=0: we close rw ourselves.
        int result = IMG_SavePNG_RW(surf, rw, 0);
        SDL_RWclose(rw); // triggers mem_close -> SDL_FreeRW

        if (result != 0)
        {
            std::cerr << "IMG_SavePNG_RW failed: " << IMG_GetError() << '\n';
            return false;
        }
        out = std::move(m.bytes);
        return true;
    }

    bool saveProject(Canvas &canvas, SDL_Renderer *renderer, const std::vector<SDL_Color> &swatches, const std::string &path)
    {
        json meta;
        meta["width"] = canvas.getWidth();
        meta["height"] = canvas.getHeight();
        SDL_Color bg = canvas.getBackgroundColor();
        meta["background"] = {{"r", bg.r}, {"g", bg.g}, {"b", bg.b}, {"a", bg.a}};
        meta["activeLayer"] = canvas.getActiveLayer();

        const auto &layers = canvas.getLayers();
        meta["layers"] = json::array();

        mz_zip_archive zip;
        memset(&zip, 0, sizeof(zip));
        if (!mz_zip_writer_init_file(&zip, path.c_str(), 0))
        {
            std::cerr << "Failed to open zip for writing: " << path << '\n';
            return false;
        }
        meta["swatches"] = json::array();
        for (const auto &c : swatches)
            meta["swatches"].push_back({{"r", c.r}, {"g", c.g}, {"b", c.b}, {"a", c.a}});
        for (size_t i = 0; i < layers.size(); ++i)
        {
            const Layer &L = layers[i];
            json lj;
            lj["name"] = L.name;
            lj["visible"] = L.visible;
            lj["kind"] = (L.kind == Layer::Kind::Background) ? "background" : "drawing";

            if (L.kind == Layer::Kind::Drawing)
            {
                std::string pngName = "layer" + std::to_string(i) + ".png";
                lj["image"] = pngName;

                SDL_Surface *surf = canvas.readLayerPixels((int)i);
                if (surf)
                {
                    std::vector<unsigned char> pngBytes;
                    if (surfaceToPngBytes(surf, pngBytes))
                        mz_zip_writer_add_mem(&zip, pngName.c_str(),
                                              pngBytes.data(), pngBytes.size(),
                                              MZ_BEST_COMPRESSION);
                    SDL_FreeSurface(surf);
                }
            }
            meta["layers"].push_back(lj);
        }

        std::string metaStr = meta.dump(2);
        mz_zip_writer_add_mem(&zip, "project.json",
                              metaStr.data(), metaStr.size(),
                              MZ_BEST_COMPRESSION);

        mz_zip_writer_finalize_archive(&zip);
        mz_zip_writer_end(&zip);
        return true;
    }

    bool loadProject(Canvas &canvas, SDL_Renderer *renderer, std::vector<SDL_Color> &swatchesOut, const std::string &path)
    {
        // 1. Open the zip for reading
        mz_zip_archive zip;
        memset(&zip, 0, sizeof(zip));
        if (!mz_zip_reader_init_file(&zip, path.c_str(), 0))
        {
            std::cerr << "Failed to open .inks for reading: " << path << '\n';
            return false;
        }

        // 2. Extract project.json into memory
        size_t jsonSize = 0;
        void *jsonData = mz_zip_reader_extract_file_to_heap(&zip, "project.json", &jsonSize, 0);
        if (!jsonData)
        {
            std::cerr << "No project.json in file\n";
            mz_zip_reader_end(&zip);
            return false;
        }

        json meta;
        try
        {
            meta = json::parse(std::string((char *)jsonData, jsonSize));
        }
        catch (const std::exception &e)
        {
            std::cerr << "Bad project.json: " << e.what() << '\n';
            mz_free(jsonData);
            mz_zip_reader_end(&zip);
            return false;
        }
        mz_free(jsonData);

        // 3. Read background color
        SDL_Color bg = {255, 255, 255, 255};
        if (meta.contains("background"))
        {
            auto &b = meta["background"];
            bg = {(Uint8)b["r"], (Uint8)b["g"], (Uint8)b["b"], (Uint8)b["a"]};
        }
        int activeIdx = meta.value("activeLayer", 1);

        swatchesOut.clear();
        if (meta.contains("swatches"))
        {
            for (auto &s : meta["swatches"])
                swatchesOut.push_back({(Uint8)s["r"], (Uint8)s["g"], (Uint8)s["b"], (Uint8)s["a"]});
        }

        // 4. Rebuild the layer list
        std::vector<Layer> newLayers;
        for (auto &lj : meta["layers"])
        {
            Layer L;
            L.name = lj.value("name", std::string("Layer"));
            L.visible = lj.value("visible", true);
            std::string kind = lj.value("kind", std::string("drawing"));

            if (kind == "background")
            {
                L.kind = Layer::Kind::Background;
                L.bgColor = bg;
            }
            else
            {
                L.kind = Layer::Kind::Drawing;
                // Extract this layer's PNG and turn it into a texture
                std::string imgName = lj.value("image", std::string());
                size_t pngSize = 0;
                void *pngData = mz_zip_reader_extract_file_to_heap(&zip, imgName.c_str(), &pngSize, 0);
                if (pngData)
                {
                    SDL_RWops *rw = SDL_RWFromConstMem(pngData, (int)pngSize);
                    SDL_Surface *surf = IMG_Load_RW(rw, 1); // 1 = auto-close rw
                    mz_free(pngData);
                    if (surf)
                    {
                        L.texture = canvas.makeLayerTextureFromSurface(surf);
                        SDL_FreeSurface(surf);
                    }
                }
            }
            newLayers.push_back(L);
        }

        mz_zip_reader_end(&zip);

        // 5. Hand the rebuilt stack to the canvas
        canvas.loadLayers(std::move(newLayers), activeIdx, bg);
        return true;
    }
}