#include "inkspire/app.hpp"
#include "inkspire/canvas.hpp"
#include "inkspire/project.hpp"
#include <tinyfiledialogs.h>
#include <iostream>
#include <algorithm>

namespace Inkspire
{
    static bool rectContains(const SDL_Rect &r, int x, int y)
    {
        return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h;
    }

    App::App() = default;

    App::~App()
    {
        shutdown();
    }

    bool App::init()
    {
        if (SDL_Init(SDL_INIT_VIDEO) != 0)
        {
            std::cerr << "SDL_Init failed: " << SDL_GetError() << '\n';
            return false;
        }

        window = SDL_CreateWindow("Inkspire", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                  windowW, windowH, SDL_WINDOW_RESIZABLE);
        if (!window)
        {
            std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << '\n';
            return false;
        }

        SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl"); // <-- add this line

        // Crisp, blocky pixels when zoomed in (nearest-neighbor sampling), the
        // expected default for a drawing app. Must be set before textures are
        // created so they inherit it.
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
        if (!renderer)
        {
            std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << '\n';
            return false;
        }

        SDL_RendererInfo info;
        SDL_GetRendererInfo(renderer, &info);
        if (!(info.flags & SDL_RENDERER_TARGETTEXTURE))
        {
            std::cerr << "Renderer does not support target textures\n";
            return false;
        }

        // Initialize SDL_ttf and load the font
        if (TTF_Init() != 0)
        {
            std::cerr << "TTF_Init failed: " << TTF_GetError() << '\n';
            return false;
        }

        font = TTF_OpenFont("assets/fonts/DejaVuSans.ttf", 14);
        if (!font)
        {
            std::cerr << "Failed to load font: " << TTF_GetError() << '\n';
            return false;
        }

        canvas = std::make_unique<Canvas>();
        if (!canvas->init(renderer, 800, 600))
        {
            std::cerr << "Canvas init failed\n";
            return false;
        }

        // Build the menu bar
        menuBar.init(font);
        Menu &file = menuBar.addMenu("File");
        file.addItem("New", [this]()
                     { canvas->reset();
                       toolState.resetSwatches();
                       currentFilePath.clear();
                       updateWindowTitle(); });
        file.addItem("Save", [this]()
                     { saveCurrent(); });
        file.addItem("Save As", [this]()
                     { saveAs(); });
        file.addItem("Open", [this]()
                     {
                         const char *filters[1] = { "*.inks" };
                         const char *path = tinyfd_openFileDialog(
                             "Open Project",
                             "",
                             1,
                             filters,
                             "Inkspire Project",
                             0); // 0 = single file
                         if (path)
                         {
                            std::vector<SDL_Color> loadedSwatches;
                            if (loadProject(*canvas, renderer, loadedSwatches, path))
                            {
                                toolState.setSwatches(loadedSwatches);
                                currentFilePath = path;
                                updateWindowTitle();
                            }
                            else
                                std::cerr << "Open failed\n";
                         } });
        file.addItem("Exit", [this]()
                     { quit = true; });

        Menu &edit = menuBar.addMenu("Edit");
        edit.addItem("Undo", [this]()
                     { canvas->undo(); });
        edit.addItem("Redo", [this]()
                     { canvas->redo(); });
        edit.addItem("Clear", [this]()
                     { canvas->pushUndo();
                       canvas->clear(); });

        Menu &view = menuBar.addMenu("View");
        view.addItem("Zoom In", [this]()
                     { zoomInAtCenter(); });
        view.addItem("Zoom Out", [this]()
                     { zoomOutAtCenter(); });
        view.addItem("Reset Zoom", [this]()
                     { resetZoom(); });
        view.addItem("Toggle Fullscreen", [this]()
                     { toggleFullscreen(); });

        toolState.init(font);
        toolsPanel.init(font, &toolState);
        toolOptionsPanel.init(font, &toolState);
        layersPanel.init(font);

        dock.buildDefault(); // tools left, layers right, canvas center

        // System cursors for divider hover/resize feedback
        curDefault_ = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
        curResizeH_ = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE);
        curResizeV_ = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS);

        bgPicker.init(font);

        recenterCanvas();

        updateWindowTitle();
        return true;
    }

    void App::shutdown()
    {
        canvas.reset();

        if (curDefault_)
        {
            SDL_FreeCursor(curDefault_);
            curDefault_ = nullptr;
        }
        if (curResizeH_)
        {
            SDL_FreeCursor(curResizeH_);
            curResizeH_ = nullptr;
        }
        if (curResizeV_)
        {
            SDL_FreeCursor(curResizeV_);
            curResizeV_ = nullptr;
        }

        if (font)
        {
            TTF_CloseFont(font);
            font = nullptr;
        }
        TTF_Quit();

        if (renderer)
        {
            SDL_DestroyRenderer(renderer);
            renderer = nullptr;
        }

        if (window)
        {
            SDL_DestroyWindow(window);
            window = nullptr;
        }

        SDL_Quit();
    }

    void App::saveAs()
    {
        const char *filters[1] = {"*.inks"};
        const char *path = tinyfd_saveFileDialog(
            "Save Project As", "untitled.inks", 1, filters, "Inkspire Project");
        if (path)
        {
            if (saveProject(*canvas, renderer, toolState.getSwatches(), path))
            {
                currentFilePath = path; // this file is now "the current file"
                updateWindowTitle();
            }
            else
                std::cerr << "Save failed\n";
        }
    }

    void App::saveCurrent()
    {
        if (currentFilePath.empty())
        {
            saveAs(); // no file yet — behave like Save As
            return;
        }
        // Quick-save: no dialog, reuse the known path
        if (!saveProject(*canvas, renderer, toolState.getSwatches(), currentFilePath))
            std::cerr << "Save failed\n";
    }

    void App::updateWindowTitle()
    {
        std::string title = "Inkspire — ";
        if (currentFilePath.empty())
        {
            title += "Untitled";
        }
        else
        {
            // Extract just the filename from the full path
            size_t slash = currentFilePath.find_last_of("/\\");
            std::string name = (slash == std::string::npos)
                                   ? currentFilePath
                                   : currentFilePath.substr(slash + 1);
            title += name;
        }
        SDL_SetWindowTitle(window, title.c_str());
    }

    int App::scaledCanvasW() const { return (int)(canvasW * zoom + 0.5f); }
    int App::scaledCanvasH() const { return (int)(canvasH * zoom + 0.5f); }

    void App::relayout()
    {
        // The dock tree lays out everything below the menu bar.
        int barH = menuBar.height();
        SDL_Rect area = {0, barH, windowW, windowH - barH};
        dock.compute(area);
    }

    Panel *App::panelById(const std::string &id)
    {
        // App is the panel registry: it owns the concrete Panel objects and
        // maps dock ids to them. (A map would scale better once there are many
        // panels; two panels don't warrant it yet.)
        if (id == "tools")
            return &toolsPanel;
        if (id == "tooloptions")
            return &toolOptionsPanel;
        if (id == "layers")
            return &layersPanel;
        return nullptr;
    }

    SDL_Rect App::panelRect(const std::string &id) const
    {
        for (const auto &p : dock.placements())
            if (p.id == id)
                return p.contentRect;
        return {0, 0, 0, 0};
    }

    PanelContext App::makeContext(const SDL_Rect &bounds)
    {
        PanelContext ctx;
        ctx.renderer = renderer;
        ctx.bounds = bounds;
        ctx.canvas = canvas.get();
        return ctx;
    }

    void App::drawText(const std::string &s, int x, int y, SDL_Color c)
    {
        SDL_Surface *surf = TTF_RenderText_Blended(font, s.c_str(), c);
        if (!surf)
            return;
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_Rect dst = {x, y, surf->w, surf->h};
        SDL_RenderCopy(renderer, tex, nullptr, &dst);
        SDL_DestroyTexture(tex);
        SDL_FreeSurface(surf);
    }

    SDL_Rect App::floatContentRect(const FloatingPanel &f) const
    {
        return {f.rect.x, f.rect.y + floatHeaderH_, f.rect.w, f.rect.h - floatHeaderH_};
    }

    int App::floatHeaderAt(int x, int y) const
    {
        // Topmost float first (drawn last = last in vector).
        for (int i = (int)floats_.size() - 1; i >= 0; --i)
        {
            SDL_Rect h = {floats_[i].rect.x, floats_[i].rect.y, floats_[i].rect.w, floatHeaderH_};
            if (rectContains(h, x, y))
                return i;
        }
        return -1;
    }

    int App::floatContentPanelAt(int x, int y) const
    {
        for (int i = (int)floats_.size() - 1; i >= 0; --i)
            if (rectContains(floats_[i].rect, x, y))
                return i;
        return -1;
    }

    void App::tearOff(const std::string &panelId, int atX, int atY)
    {
        // Center the new float's header roughly under the cursor.
        FloatingPanel f;
        f.id = panelId;
        f.rect = {atX - floatDefaultW_ / 2, atY - floatHeaderH_ / 2,
                  floatDefaultW_, floatDefaultH_};
        floats_.push_back(f);
    }

    void App::renderFloats()
    {
        SDL_Color text = {220, 220, 225, 255};
        for (auto &f : floats_)
        {
            Panel *p = panelById(f.id);

            // Frame
            SDL_SetRenderDrawColor(renderer, 40, 40, 44, 255);
            SDL_RenderFillRect(renderer, &f.rect);

            // Header
            SDL_Rect header = {f.rect.x, f.rect.y, f.rect.w, floatHeaderH_};
            SDL_SetRenderDrawColor(renderer, 30, 30, 34, 255);
            SDL_RenderFillRect(renderer, &header);
            drawText(p ? p->title() : f.id, header.x + 8,
                     header.y + (floatHeaderH_ - TTF_FontHeight(font)) / 2, text);

            // Content, clipped
            SDL_Rect content = floatContentRect(f);
            if (p)
            {
                SDL_RenderSetClipRect(renderer, &content);
                p->render(makeContext(content));
                SDL_RenderSetClipRect(renderer, nullptr);
            }

            // Border last, on top
            SDL_SetRenderDrawColor(renderer, 90, 90, 98, 255);
            SDL_RenderDrawRect(renderer, &f.rect);
        }
    }

    void App::renderPanel(const PanelPlacement &pl)
    {
        Panel *p = panelById(pl.id);
        SDL_Color text = {220, 220, 225, 255};

        // Which side of the window is this dock on? The collapse arrow points
        // toward the nearest edge (the way the panel tucks away); the expand
        // arrow points back inward. Left dock: collapse "<<", expand ">>".
        // Right dock: collapse ">>", expand "<<". (Inferred from position for
        // now; becomes an explicit dock side once panels can be dragged.)
        bool rightSide = (pl.fullRect.x + pl.fullRect.w / 2) > (windowW / 2);
        const char *collapseArrow = rightSide ? ">>" : "<<";
        const char *expandArrow = rightSide ? "<<" : ">>";

        if (pl.collapsed)
        {
            // Thin rail: fill, the expand glyph near the top, panel title drawn
            // vertically (one character per line) below it.
            SDL_SetRenderDrawColor(renderer, 44, 44, 48, 255);
            SDL_RenderFillRect(renderer, &pl.fullRect);
            SDL_SetRenderDrawColor(renderer, 70, 70, 76, 255);
            SDL_RenderDrawRect(renderer, &pl.fullRect);

            int cx = pl.fullRect.x + pl.fullRect.w / 2;
            drawText(expandArrow, cx - 8, pl.fullRect.y + 5, text);

            // Title, one letter per row, going down.
            const char *title = p ? p->title() : "";
            int ty = pl.fullRect.y + pl.fullRect.w + 6;
            for (const char *c = title; *c; ++c)
            {
                if (*c == ' ')
                {
                    ty += 6;
                    continue;
                }
                char buf[2] = {*c, 0};
                drawText(buf, cx - 4, ty, text);
                ty += 14;
            }
            return;
        }

        // Header bar
        SDL_SetRenderDrawColor(renderer, 34, 34, 38, 255);
        SDL_RenderFillRect(renderer, &pl.headerRect);
        drawText(p ? p->title() : "", pl.headerRect.x + 8,
                 pl.headerRect.y + (pl.headerRect.h - TTF_FontHeight(font)) / 2, text);

        // Collapse toggle, inset from the panel's border so it isn't clipped by
        // the divider/edge.
        SDL_Rect tog = pl.toggleRect;
        tog.x -= 3; // pull left, off the right border
        tog.y += 2; // small top inset
        tog.h -= 4;
        SDL_SetRenderDrawColor(renderer, 54, 54, 60, 255);
        SDL_RenderFillRect(renderer, &tog);
        drawText(collapseArrow, tog.x + 3,
                 tog.y + (tog.h - TTF_FontHeight(font)) / 2, text);

        // Tab strip, when this column holds multiple panels.
        if (pl.tabIds.size() > 1)
        {
            int n = (int)pl.tabIds.size();
            int tabW = pl.tabStripRect.w / n;
            for (int i = 0; i < n; ++i)
            {
                SDL_Rect tabRect = {pl.tabStripRect.x + i * tabW, pl.tabStripRect.y,
                                    tabW, pl.tabStripRect.h};
                bool activeTab = (i == pl.activeTab);
                SDL_SetRenderDrawColor(renderer, activeTab ? 48 : 30,
                                       activeTab ? 48 : 30, activeTab ? 54 : 34, 255);
                SDL_RenderFillRect(renderer, &tabRect);
                SDL_SetRenderDrawColor(renderer, 20, 20, 22, 255);
                SDL_RenderDrawRect(renderer, &tabRect);

                Panel *tp = panelById(pl.tabIds[i]);
                const char *tt = tp ? tp->title() : pl.tabIds[i].c_str();
                // Clip the label to its tab so long titles don't overrun.
                SDL_RenderSetClipRect(renderer, &tabRect);
                drawText(tt, tabRect.x + 5,
                         tabRect.y + (tabRect.h - TTF_FontHeight(font)) / 2, text);
                SDL_RenderSetClipRect(renderer, nullptr);
            }
        }

        // Panel content, clipped to its rectangle so nothing spills out even at
        // small sizes.
        if (p)
        {
            SDL_RenderSetClipRect(renderer, &pl.contentRect);
            p->render(makeContext(pl.contentRect));
            SDL_RenderSetClipRect(renderer, nullptr);
        }
    }

    void App::recenterCanvas()
    {
        // The dock layout provides the canvas center rectangle directly — the
        // tool options are now a real docked panel, so there's no overlay to
        // account for.
        relayout();
        SDL_Rect center = dock.canvasRect();

        int sw = scaledCanvasW();
        int sh = scaledCanvasH();

        if (center.w > sw)
            canvasX = center.x + (center.w - sw) / 2;
        else
            canvasX = center.x;

        if (center.h > sh)
            canvasY = center.y + (center.h - sh) / 2;
        else
            canvasY = center.y;
    }

    void App::zoomAt(int screenPivotX, int screenPivotY, float newZoom)
    {
        newZoom = std::clamp(newZoom, MIN_ZOOM, MAX_ZOOM);
        if (newZoom == zoom)
            return;

        // Keep the document point currently under (screenPivotX, screenPivotY)
        // under that same screen point after the zoom changes.
        float ratio = newZoom / zoom;
        canvasX = (int)(screenPivotX - (screenPivotX - canvasX) * ratio + 0.5f);
        canvasY = (int)(screenPivotY - (screenPivotY - canvasY) * ratio + 0.5f);
        zoom = newZoom;
    }

    void App::zoomInAtCenter() { zoomAt(windowW / 2, windowH / 2, zoom * 1.25f); }
    void App::zoomOutAtCenter() { zoomAt(windowW / 2, windowH / 2, zoom / 1.25f); }

    void App::resetZoom()
    {
        zoom = 1.0f;
        recenterCanvas();
    }

    void App::toggleFullscreen()
    {
        fullscreen = !fullscreen;
        SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
        // The resize event that follows will update windowW/H and recenter
    }

    bool App::screenToCanvas(int sx, int sy, int &cx, int &cy) const
    {
        // Screen → document: undo the pan offset, then undo the zoom scale.
        cx = (int)((sx - canvasX) / zoom);
        cy = (int)((sy - canvasY) / zoom);
        return cx >= 0 && cx < canvasW && cy >= 0 && cy < canvasH;
    }

    void App::handle_events(bool &running)
    {
        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
            {
                running = false;
                continue;
            }

            if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
            {
                windowW = e.window.data1;
                windowH = e.window.data2;
                if (canvas)
                    canvas->preserveAcrossResize();
                recenterCanvas();
                continue;
            }

            // Menu bar gets first look; if it consumes the event, don't draw
            if (menuBar.handleEvent(e))
                continue;

            // ---- Dock divider dragging (resize docks) ----
            // An active drag captures motion/up anywhere, so the divider keeps
            // tracking even if the cursor outruns the thin bar.
            if (dragDivider_)
            {
                if (e.type == SDL_MOUSEMOTION)
                {
                    dock.dragDivider(dragDivider_, e.motion.x, e.motion.y);
                    recenterCanvas(); // reflow (also recomputes the dock)
                    continue;
                }
                if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT)
                {
                    dragDivider_ = nullptr;
                    continue;
                }
            }

            // ---- Panel drag: redock, tear-off, and float movement ----
            // A header press (dock or float) arms a drag; motion past a
            // threshold promotes it. On release: over a dock zone → dock; over
            // the canvas interior → float. A float being dragged also moves in
            // real time so it tracks the cursor even before crossing a zone.
            if (pendingDragLeaf_ || panelDragging_ || dragFromFloat_)
            {
                if (e.type == SDL_MOUSEMOTION)
                {
                    int prevX = dragCurX_, prevY = dragCurY_;
                    dragCurX_ = e.motion.x;
                    dragCurY_ = e.motion.y;
                    if (!panelDragging_)
                    {
                        int dx = e.motion.x - dragStartX_;
                        int dy = e.motion.y - dragStartY_;
                        if (dx * dx + dy * dy >= 25) // ~5px threshold
                            panelDragging_ = true;
                    }
                    // While dragging a float, move its frame with the cursor.
                    if (dragFromFloat_ && dragFloatIndex_ >= 0 &&
                        dragFloatIndex_ < (int)floats_.size())
                    {
                        floats_[dragFloatIndex_].rect.x += dragCurX_ - prevX;
                        floats_[dragFloatIndex_].rect.y += dragCurY_ - prevY;
                    }
                    continue;
                }
                if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT)
                {
                    if (panelDragging_)
                    {
                        DropTarget tgt = dock.hitTestDropZone(e.button.x, e.button.y);
                        bool docked = false;
                        if (tgt.leaf && tgt.zone != DropZone::None)
                        {
                            if (dragFromFloat_)
                            {
                                // Re-dock a float: insert into the tree, then
                                // remove it from the floating layer. Source leaf
                                // is null (it isn't in the tree), so we use a
                                // tab/split that doesn't touch a source.
                                docked = dock.applyDropFromFloat(dragPanelId_, tgt);
                            }
                            else
                            {
                                docked = dock.applyDrop(pendingDragLeaf_, dragPanelId_, tgt);
                            }
                            if (docked)
                                recenterCanvas();
                        }

                        if (!docked)
                        {
                            // No dock zone under the cursor → float here.
                            if (dragFromFloat_)
                            {
                                // Already a float; movement already tracked it.
                            }
                            else
                            {
                                // Tear off from the dock: remove from the tree,
                                // then create a float at the drop point.
                                if (dock.detachPanel(pendingDragLeaf_, dragPanelId_))
                                {
                                    tearOff(dragPanelId_, e.button.x, e.button.y);
                                    recenterCanvas();
                                }
                            }
                        }
                        else if (dragFromFloat_ && dragFloatIndex_ >= 0 &&
                                 dragFloatIndex_ < (int)floats_.size())
                        {
                            // Successfully re-docked a float → remove it from the
                            // floating layer.
                            floats_.erase(floats_.begin() + dragFloatIndex_);
                        }
                    }
                    // Reset drag state whether it was a drag or just a click.
                    pendingDragLeaf_ = nullptr;
                    panelDragging_ = false;
                    dragFromFloat_ = false;
                    dragFloatIndex_ = -1;
                    dragPanelId_.clear();
                    dragPanelTitle_.clear();
                    continue;
                }
            }
            else if (e.type == SDL_MOUSEMOTION)
            {
                // Hover feedback: show a resize cursor over a divider.
                if (DockNode *d = dock.hitTestDivider(e.motion.x, e.motion.y))
                    SDL_SetCursor(d->dir == SplitDir::Vertical ? curResizeH_ : curResizeV_);
                else
                    SDL_SetCursor(curDefault_);
            }
            if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
            {
                // Floating panels are on top — check them first.
                int fh = floatHeaderAt(e.button.x, e.button.y);
                if (fh >= 0)
                {
                    // Bring to front (draw last) and arm a drag from this float.
                    FloatingPanel f = floats_[fh];
                    floats_.erase(floats_.begin() + fh);
                    floats_.push_back(f);
                    dragFloatIndex_ = (int)floats_.size() - 1;
                    dragFromFloat_ = true;
                    dragPanelId_ = f.id;
                    Panel *p = panelById(f.id);
                    dragPanelTitle_ = p ? p->title() : f.id;
                    dragStartX_ = dragCurX_ = e.button.x;
                    dragStartY_ = dragCurY_ = e.button.y;
                    continue; // wait for motion or release
                }
                // Click inside a float's content (not header): route to that
                // float and bring it to front, before any dock chrome sees it.
                int fc = floatContentPanelAt(e.button.x, e.button.y);
                if (fc >= 0)
                {
                    FloatingPanel f = floats_[fc];
                    floats_.erase(floats_.begin() + fc);
                    floats_.push_back(f);
                    Panel *p = panelById(f.id);
                    if (p)
                        p->handleEvent(e, makeContext(floatContentRect(floats_.back())));
                    continue;
                }

                if (DockNode *d = dock.hitTestDivider(e.button.x, e.button.y))
                {
                    dragDivider_ = d;
                    continue; // start dragging; don't let panels see this click
                }
                // Collapse / expand a column via its header toggle.
                if (DockNode *leaf = dock.hitTestCollapseToggle(e.button.x, e.button.y))
                {
                    leaf->collapsed = !leaf->collapsed;
                    recenterCanvas(); // reflow around the new rail width
                    continue;
                }
                // Tab click: switch the active panel in a tabbed column.
                int tabIdx = 0;
                if (DockNode *leaf = dock.hitTestTab(e.button.x, e.button.y, tabIdx))
                {
                    leaf->activeTab = tabIdx;
                    continue;
                }
                // Header press (not on the toggle) arms a potential redock drag.
                for (const auto &pl : dock.placements())
                {
                    if (pl.collapsed)
                        continue;
                    if (rectContains(pl.headerRect, e.button.x, e.button.y))
                    {
                        pendingDragLeaf_ = pl.leaf;
                        dragPanelId_ = pl.id;
                        Panel *p = panelById(pl.id);
                        dragPanelTitle_ = p ? p->title() : pl.id;
                        dragStartX_ = dragCurX_ = e.button.x;
                        dragStartY_ = dragCurY_ = e.button.y;
                        break;
                    }
                }
                if (pendingDragLeaf_)
                    continue; // header press consumed; wait for motion or release
            }

            // Floating panel content gets events before docked panels (floats
            // are visually on top). Route to the topmost float under the cursor;
            // an in-progress widget drag inside a float keeps its own state.
            {
                bool consumed = false;
                for (int i = (int)floats_.size() - 1; i >= 0; --i)
                {
                    Panel *p = panelById(floats_[i].id);
                    if (p && p->handleEvent(e, makeContext(floatContentRect(floats_[i]))))
                    {
                        consumed = true;
                        break;
                    }
                }
                if (consumed)
                    continue;
            }

            // Offer the event to docked panels, in placement order. A collapsed
            // column has no interactive content, so it's skipped. Panels receive
            // their content rectangle (below the header) as bounds.
            {
                bool consumed = false;
                for (const auto &pl : dock.placements())
                {
                    if (pl.collapsed)
                        continue;
                    Panel *p = panelById(pl.id);
                    if (p && p->handleEvent(e, makeContext(pl.contentRect)))
                    {
                        consumed = true;
                        break;
                    }
                }
                if (consumed)
                    continue;
            }

            if (bgPopupOpen && e.type != SDL_KEYDOWN)
            {
                // Snapshot once, on the mouse-down that starts the interaction,
                // so a click-drag across the value slider is a single undo entry
                // rather than one per motion event.
                if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
                    canvas->pushUndo();
                bgPicker.handleEvent(e);
                canvas->setBackgroundColor(bgPicker.color());
                continue;
            }

            // ---- View navigation: wheel zoom + space-drag pan ----
            // These are handled ahead of the drawing logic so a pan gesture
            // never leaks through as a stroke.
            if (e.type == SDL_MOUSEWHEEL)
            {
                int mx, my;
                SDL_GetMouseState(&mx, &my);
                float step = (e.wheel.y > 0) ? 1.25f : (1.0f / 1.25f);
                if (e.wheel.y != 0)
                    zoomAt(mx, my, zoom * step); // zoom toward the cursor
                continue;
            }

            if (spaceDown && e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
            {
                panning = true;
                panLastX = e.button.x;
                panLastY = e.button.y;
                continue;
            }
            if (panning && e.type == SDL_MOUSEMOTION)
            {
                canvasX += e.motion.x - panLastX;
                canvasY += e.motion.y - panLastY;
                panLastX = e.motion.x;
                panLastY = e.motion.y;
                continue;
            }
            if (panning && e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT)
            {
                panning = false;
                continue;
            }

            if (e.type == SDL_KEYDOWN)
            {
                SDL_Keycode k = e.key.keysym.sym;
                bool ctrl = (e.key.keysym.mod & KMOD_CTRL) != 0;
                bool shift = (e.key.keysym.mod & KMOD_SHIFT) != 0;

                // Ctrl+Z = undo, Ctrl+Y or Ctrl+Shift+Z = redo
                if (ctrl && k == SDLK_z && !shift)
                    canvas->undo();
                else if (ctrl && (k == SDLK_y || (k == SDLK_z && shift)))
                    canvas->redo();
                // Ctrl+= / Ctrl+- zoom (center); Ctrl+0 resets to 100%.
                // SDLK_EQUALS is the unshifted "=" key (the "+" key).
                else if (ctrl && (k == SDLK_EQUALS || k == SDLK_PLUS || k == SDLK_KP_PLUS))
                    zoomInAtCenter();
                else if (ctrl && (k == SDLK_MINUS || k == SDLK_KP_MINUS))
                    zoomOutAtCenter();
                else if (ctrl && k == SDLK_0)
                    resetZoom();
                else if (k == SDLK_SPACE)
                    spaceDown = true; // hold to pan-drag
                else if (k == SDLK_b)
                    currentTool = Tool::Brush;
                else if (k == SDLK_e)
                    currentTool = Tool::Eraser;
                else if (k == SDLK_F11)
                    toggleFullscreen();
                else if (k == SDLK_g)
                {
                    if (bgPopupOpen)
                        canvas->setBackgroundColor(bgPicker.color());
                    bgPopupOpen = !bgPopupOpen;
                }
                else if (k == SDLK_h)
                {
                    canvas->pushUndo();
                    canvas->toggleVisible(canvas->getActiveLayer());
                }
                else if (k >= SDLK_1 && k <= SDLK_9)
                    canvas->setActiveLayer(k - SDLK_1);
            }
            else if (e.type == SDL_KEYUP)
            {
                if (e.key.keysym.sym == SDLK_SPACE)
                {
                    spaceDown = false;
                    panning = false; // safety: releasing space ends any pan
                }
            }
            else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
            {
                int cx, cy;
                if (screenToCanvas(e.button.x, e.button.y, cx, cy))
                {
                    // One undo entry per stroke: snapshot before the first pixel
                    // changes. True for the brush (committed at mouse-up) and the
                    // eraser (drawn continuously), so it must happen here, once.
                    if (canvas->activeIsDrawable())
                        canvas->pushUndo();

                    drawing = true;
                    lastX = cx;
                    lastY = cy;
                    if (toolState.tool() == Tool::Brush)
                    {
                        canvas->beginStroke();
                        strokeActive = true;
                    }
                }
            }
            else if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT)
            {
                if (strokeActive)
                {
                    canvas->commitStroke(toolState.brushOpacity());
                    strokeActive = false;
                }
                drawing = false;
            }
            else if (e.type == SDL_MOUSEMOTION && drawing)
            {
                int cx, cy;
                screenToCanvas(e.motion.x, e.motion.y, cx, cy);
                if (toolState.tool() == Tool::Brush)
                    canvas->strokeLine(lastX, lastY, cx, cy, toolState.brushSize(), toolState.brushColor());
                else
                    canvas->drawLine(lastX, lastY, cx, cy, Tool::Eraser, toolState.brushSize());
                lastX = cx;
                lastY = cy;
            }
        }
    }

    void App::update()
    {
        // Per-frame time-based state goes here (animations, smooth pan/zoom).
        // Empty for now — all current behavior is event-driven.
    }

    void App::render()
    {
        // Workspace "desk"
        SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255); // #3C3C3C
        SDL_RenderClear(renderer);

        if (canvas)
        {
            SDL_Rect paper = {canvasX, canvasY, scaledCanvasW(), scaledCanvasH()};
            canvas->renderCheckerboard(renderer, paper);
            canvas->render(renderer, paper, toolState.brushOpacity(), strokeActive);
        }

        for (const auto &pl : dock.placements())
            renderPanel(pl);

        // Dock divider bars.
        SDL_SetRenderDrawColor(renderer, 28, 28, 30, 255);
        for (const auto &d : dock.dividers())
            SDL_RenderFillRect(renderer, &d);

        // Floating panels, on top of the docks.
        renderFloats();

        menuBar.render(renderer);

        // Background Color Picker
        if (bgPopupOpen)
        {
            int pw = 200, ph = 280;
            int px = windowW / 2 - pw / 2;
            int py = windowH / 2 - ph / 2;
            SDL_SetRenderDrawColor(renderer, 45, 45, 50, 255);
            SDL_Rect panel = {px, py, pw, ph};
            SDL_RenderFillRect(renderer, &panel);
            SDL_SetRenderDrawColor(renderer, 100, 100, 105, 255);
            SDL_RenderDrawRect(renderer, &panel);

            // Title
            SDL_Color white = {230, 230, 230, 255};
            SDL_Surface *surf = TTF_RenderText_Blended(font, "Background Color", white);
            if (surf)
            {
                SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
                SDL_Rect dst = {px + 12, py + 10, surf->w, surf->h};
                SDL_RenderCopy(renderer, tex, nullptr, &dst);
                SDL_DestroyTexture(tex);
                SDL_FreeSurface(surf);
            }

            bgPicker.render(renderer, px + 12, py + 60, pw - 24, true, false);
        }

        // ---- Panel drag overlay (drop highlight + title ghost) ----
        if (panelDragging_)
        {
            DropTarget tgt = dock.hitTestDropZone(dragCurX_, dragCurY_);
            if (tgt.leaf && tgt.zone != DropZone::None)
            {
                // Translucent fill over the region the panel would occupy.
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer, 90, 150, 240, 90);
                SDL_RenderFillRect(renderer, &tgt.highlight);
                SDL_SetRenderDrawColor(renderer, 120, 180, 255, 220);
                SDL_RenderDrawRect(renderer, &tgt.highlight);
            }

            // Title label following the cursor — only for dock-originated drags.
            // A floating panel being dragged is its own visible feedback.
            if (!dragFromFloat_)
            {
                SDL_Color ghost = {235, 235, 240, 255};
                SDL_Surface *surf = TTF_RenderText_Blended(font, dragPanelTitle_.c_str(), ghost);
                if (surf)
                {
                    SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
                    SDL_Rect bg = {dragCurX_ + 12, dragCurY_ + 8, surf->w + 12, surf->h + 6};
                    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                    SDL_SetRenderDrawColor(renderer, 40, 44, 52, 220);
                    SDL_RenderFillRect(renderer, &bg);
                    SDL_SetRenderDrawColor(renderer, 120, 140, 170, 255);
                    SDL_RenderDrawRect(renderer, &bg);
                    SDL_Rect dst = {bg.x + 6, bg.y + 3, surf->w, surf->h};
                    SDL_RenderCopy(renderer, tex, nullptr, &dst);
                    SDL_DestroyTexture(tex);
                    SDL_FreeSurface(surf);
                }
            }
        }

        SDL_RenderPresent(renderer);
    }

    int App::run()
    {
        if (!init())
            return 1;

        bool running = true;
        while (running && !quit)
        {
            handle_events(running);
            update();
            render();
            SDL_Delay(16);
        }

        return 0; // ~App() calls shutdown()
    }
}