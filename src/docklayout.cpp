#include "inkspire/docklayout.hpp"
#include <algorithm>
#include <functional>

namespace Inkspire
{
    static bool rectContains(const SDL_Rect &r, int x, int y)
    {
        return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h;
    }

    DockNode *DockLayout::hitTestDivider(int x, int y)
    {
        // Walk the tree; the divider rects were captured during the last compute.
        // A small recursive lambda avoids adding another member function.
        DockNode *found = nullptr;
        std::function<void(DockNode *)> walk = [&](DockNode *n)
        {
            if (!n || n->isLeaf || found)
                return;
            if (rectContains(n->divider, x, y))
            {
                found = n;
                return;
            }
            walk(n->a.get());
            walk(n->b.get());
        };
        walk(root_.get());
        return found;
    }

    // Minimum extents of a subtree along each axis, honoring split direction so
    // a column can't be dragged narrower than its widest panel nor shorter than
    // the sum of its stacked panels. A collapsed column or the canvas imposes no
    // floor (they flex to a rail / fill freely).
    int DockLayout::minWidthOf(DockNode *node) const
    {
        if (!node || node->isCanvasCenter)
            return 0;
        if (node->collapsed)
            return railWidth_;
        if (node->isLeaf)
            return node->minWidth;
        int a = minWidthOf(node->a.get());
        int b = minWidthOf(node->b.get());
        if (node->dir == SplitDir::Vertical)
            return a + b + dividerThickness_; // side by side: widths add
        return std::max(a, b);                // stacked: take the wider
    }

    int DockLayout::minHeightOf(DockNode *node) const
    {
        if (!node || node->isCanvasCenter)
            return 0;
        if (node->collapsed)
            return 0; // a collapsed rail spans whatever height it's given
        if (node->isLeaf)
            return node->minHeight;
        int a = minHeightOf(node->a.get());
        int b = minHeightOf(node->b.get());
        if (node->dir == SplitDir::Horizontal)
            return a + b + dividerThickness_; // stacked: heights add
        return std::max(a, b);                // side by side: take the taller
    }

    void DockLayout::dragDivider(DockNode *node, int mx, int my)
    {
        if (!node || node->isLeaf)
            return;

        const SDL_Rect &r = node->rect; // the node's full area from last compute
        const int t = dividerThickness_;

        if (node->dir == SplitDir::Vertical)
        {
            int minA = minWidthOf(node->a.get());
            int minB = minWidthOf(node->b.get());
            int usable = r.w - t;
            if (usable <= 0)
                return;
            int aW = mx - r.x;
            int lo = minA;
            int hi = usable - minB;
            if (lo > hi)
                lo = hi = usable / 2; // degenerate; split evenly
            aW = std::clamp(aW, lo, hi);

            if (node->fixedBPixels > 0)
                node->fixedBPixels = usable - aW; // b is the fixed side
            else if (node->fixedAPixels > 0)
                node->fixedAPixels = aW;
            else
                node->ratio = (float)aW / (float)usable;
        }
        else // Horizontal
        {
            int minA = minHeightOf(node->a.get());
            int minB = minHeightOf(node->b.get());
            int usable = r.h - t;
            if (usable <= 0)
                return;
            int aH = my - r.y;
            int lo = minA;
            int hi = usable - minB;
            if (lo > hi)
                lo = hi = usable / 2;
            aH = std::clamp(aH, lo, hi);

            if (node->fixedBPixels > 0)
                node->fixedBPixels = usable - aH;
            else if (node->fixedAPixels > 0)
                node->fixedAPixels = aH;
            else
                node->ratio = (float)aH / (float)usable;
        }
    }

    DockNode *DockLayout::hitTestCollapseToggle(int x, int y)
    {
        // The single collapse toggle lives on the column's top placement; return
        // the COLUMN ROOT so the caller flips collapse for the whole column.
        for (const auto &pl : placements_)
            if ((pl.isColumnTop || pl.collapsed) && rectContains(pl.toggleRect, x, y))
                return pl.columnRoot ? pl.columnRoot : pl.leaf;
        return nullptr;
    }

    DropTarget DockLayout::hitTestDropZone(int x, int y)
    {
        DropTarget out;
        for (const auto &pl : placements_)
        {
            const SDL_Rect &r = pl.fullRect;
            if (!rectContains(r, x, y))
                continue;

            out.leaf = pl.leaf;

            // Center box: inner 40% region → tab.
            int cw = r.w * 4 / 10;
            int ch = r.h * 4 / 10;
            SDL_Rect center = {r.x + (r.w - cw) / 2, r.y + (r.h - ch) / 2, cw, ch};
            if (rectContains(center, x, y))
            {
                out.zone = DropZone::Center;
                out.highlight = r; // tabbing occupies the whole leaf
                return out;
            }

            // Otherwise pick the nearest edge by comparing distances to each side.
            int dl = x - r.x;
            int dr = (r.x + r.w) - x;
            int dt = y - r.y;
            int db = (r.y + r.h) - y;
            int m = std::min(std::min(dl, dr), std::min(dt, db));

            if (m == dl)
            {
                out.zone = DropZone::Left;
                out.highlight = {r.x, r.y, r.w / 2, r.h};
            }
            else if (m == dr)
            {
                out.zone = DropZone::Right;
                out.highlight = {r.x + r.w / 2, r.y, r.w / 2, r.h};
            }
            else if (m == dt)
            {
                out.zone = DropZone::Top;
                out.highlight = {r.x, r.y, r.w, r.h / 2};
            }
            else
            {
                out.zone = DropZone::Bottom;
                out.highlight = {r.x, r.y + r.h / 2, r.w, r.h / 2};
            }
            return out;
        }

        // The canvas center is always an edge drop target (no center/tab), so a
        // side can never become permanently unreachable — this is how Photoshop
        // and CSP keep the workspace edges dockable even when empty. A dropped
        // panel previews (and lands) as a fixed-width column on that edge.
        if (canvasLeaf_ && rectContains(canvasRect_, x, y))
        {
            const SDL_Rect &r = canvasRect_;
            const int band = 200;    // preview/target width of the new dock
            const int edgeZone = 80; // how close to a canvas edge counts as "dock"

            int dl = x - r.x;
            int dr = (r.x + r.w) - x;
            int dt = y - r.y;
            int db = (r.y + r.h) - y;
            int m = std::min(std::min(dl, dr), std::min(dt, db));

            // Interior of the canvas (away from all edges) is not a dock zone —
            // a drop there floats the panel instead. Only the edge bands dock.
            if (m > edgeZone)
                return out; // None → caller floats the panel

            out.leaf = canvasLeaf_;
            if (m == dl)
            {
                out.zone = DropZone::Left;
                out.highlight = {r.x, r.y, std::min(band, r.w), r.h};
            }
            else if (m == dr)
            {
                out.zone = DropZone::Right;
                int w = std::min(band, r.w);
                out.highlight = {r.x + r.w - w, r.y, w, r.h};
            }
            else if (m == dt)
            {
                out.zone = DropZone::Top;
                out.highlight = {r.x, r.y, r.w, std::min(band, r.h)};
            }
            else
            {
                out.zone = DropZone::Bottom;
                int h = std::min(band, r.h);
                out.highlight = {r.x, r.y + r.h - h, r.w, h};
            }
            return out;
        }

        return out; // None
    }

    // ---- Tree surgery for drag-to-redock ----

    DockNode *DockLayout::findParent(DockNode *node, DockNode *child, bool &isChildA)
    {
        if (!node || node->isLeaf)
            return nullptr;
        if (node->a.get() == child)
        {
            isChildA = true;
            return node;
        }
        if (node->b.get() == child)
        {
            isChildA = false;
            return node;
        }
        if (DockNode *r = findParent(node->a.get(), child, isChildA))
            return r;
        return findParent(node->b.get(), child, isChildA);
    }

    DockNode *DockLayout::findParent(DockNode *child, bool &isChildA)
    {
        return findParent(root_.get(), child, isChildA);
    }

    void DockLayout::collapseOrphan(DockNode *emptyLeaf)
    {
        // Replace emptyLeaf's parent split with emptyLeaf's sibling, so the
        // sibling takes over the parent's slot in the tree.
        bool orphanIsA = false;
        DockNode *parent = findParent(emptyLeaf, orphanIsA);
        if (!parent)
            return; // emptyLeaf is the root (shouldn't happen with a canvas center)

        // The sibling is the other child of `parent`.
        std::unique_ptr<DockNode> sibling =
            orphanIsA ? std::move(parent->b) : std::move(parent->a);
        if (!sibling)
            return;

        // Find parent-of-parent to graft the sibling into the grandparent slot.
        bool parentIsA = false;
        DockNode *grand = findParent(parent, parentIsA);
        if (!grand)
        {
            // parent was the root → sibling becomes the new root.
            root_ = std::move(sibling);
            return;
        }
        if (parentIsA)
            grand->a = std::move(sibling);
        else
            grand->b = std::move(sibling);
        // The old parent split (and the empty leaf) are freed as the unique_ptrs
        // that held them are overwritten.
    }

    bool DockLayout::insertPanelAt(const std::string &panelId, const DropTarget &target)
    {
        if (!target.leaf || target.zone == DropZone::None)
            return false;
        DockNode *tgt = target.leaf;

        if (target.zone == DropZone::Center)
        {
            // Tab into the target.
            tgt->panelIds.push_back(panelId);
            return true;
        }

        // --- Edge zone: split the target leaf ---
        auto newLeaf = DockNode::makeLeaf({panelId});
        newLeaf->minWidth = 120;
        newLeaf->minHeight = 70;

        SplitDir dir = (target.zone == DropZone::Left || target.zone == DropZone::Right)
                           ? SplitDir::Vertical
                           : SplitDir::Horizontal;
        bool newIsA = (target.zone == DropZone::Left || target.zone == DropZone::Top);

        bool tgtIsA = false;
        DockNode *parent = findParent(tgt, tgtIsA);

        std::unique_ptr<DockNode> tgtOwned;
        if (!parent)
            tgtOwned = std::move(root_);
        else
            tgtOwned = tgtIsA ? std::move(parent->a) : std::move(parent->b);

        bool tgtIsCanvas = tgt->isCanvasCenter;
        const int band = 200;

        std::unique_ptr<DockNode> childA, childB;
        if (newIsA)
        {
            childA = std::move(newLeaf);
            childB = std::move(tgtOwned);
        }
        else
        {
            childA = std::move(tgtOwned);
            childB = std::move(newLeaf);
        }
        auto split = DockNode::makeSplit(dir, 0.5f, std::move(childA), std::move(childB));
        if (tgtIsCanvas)
        {
            if (newIsA)
                split->fixedAPixels = band;
            else
                split->fixedBPixels = band;
        }

        if (!parent)
            root_ = std::move(split);
        else if (tgtIsA)
            parent->a = std::move(split);
        else
            parent->b = std::move(split);

        return true;
    }

    bool DockLayout::detachPanel(DockNode *sourceLeaf, const std::string &panelId)
    {
        if (!sourceLeaf)
            return false;
        auto &ids = sourceLeaf->panelIds;
        bool found = false;
        for (auto it = ids.begin(); it != ids.end(); ++it)
            if (*it == panelId)
            {
                ids.erase(it);
                found = true;
                break;
            }
        if (found)
        {
            // Keep the active tab in range.
            if (sourceLeaf->activeTab >= (int)ids.size())
                sourceLeaf->activeTab = (int)ids.size() - 1;
            if (sourceLeaf->activeTab < 0)
                sourceLeaf->activeTab = 0;
            if (ids.empty())
                collapseOrphan(sourceLeaf);
        }
        return found;
    }

    bool DockLayout::applyDrop(DockNode *sourceLeaf, const std::string &panelId, const DropTarget &target)
    {
        if (!sourceLeaf || !target.leaf || target.zone == DropZone::None)
            return false;
        DockNode *tgt = target.leaf;

        // --- No-op guards ---
        if (tgt == sourceLeaf && target.zone == DropZone::Center)
            return false;
        if (tgt == sourceLeaf && sourceLeaf->panelIds.size() <= 1)
            return false;

        // Insert at the target first, then remove from the source. If
        // sourceLeaf == tgt, the source object is moved (as a unique_ptr) but the
        // object itself is unchanged, so the pointer stays valid for the detach.
        if (!insertPanelAt(panelId, target))
            return false;
        detachPanel(sourceLeaf, panelId);
        return true;
    }

    bool DockLayout::applyDropFromFloat(const std::string &panelId, const DropTarget &target)
    {
        // A floating panel has no source leaf in the tree; just insert it.
        return insertPanelAt(panelId, target);
    }

    DockNode *DockLayout::hitTestTab(int x, int y, int &tabIndex)
    {
        for (const auto &pl : placements_)
        {
            if (pl.collapsed || pl.tabIds.size() <= 1)
                continue;
            if (!rectContains(pl.tabStripRect, x, y))
                continue;
            int n = (int)pl.tabIds.size();
            int tabW = pl.tabStripRect.w / n;
            if (tabW <= 0)
                return nullptr;
            int idx = (x - pl.tabStripRect.x) / tabW;
            if (idx < 0)
                idx = 0;
            if (idx >= n)
                idx = n - 1;
            tabIndex = idx;
            return pl.leaf;
        }
        return nullptr;
    }

    void DockLayout::buildDefault()
    {
        // Reproduce the layout as a tree, now with tool options as a real dock:
        //
        //   Split(V, tools = 64px)
        //   /                     \
        // [tools]      Split(V, options = 180px)
        //              /                        \
        //   Tool Options column      Split(V, layers = 200px on right)
        //   (4 panels stacked         /                                \
        //    as rows)             Canvas center                    [layers]
        //
        // Every dock is a fixed-width strip; the canvas center is the leftover.

        // The old single "tooloptions" panel is now four category-siblings:
        // brush size, brush settings, color wheel, and the swatch palette. They
        // default to a single column, stacked vertically as rows (each its own
        // leaf joined by horizontal splits) rather than tabs — so all four are
        // visible at once from the start, the way the tool options looked before
        // the split. Each stays a distinct panel the user can drag out and dock
        // wherever; a horizontal divider sits between each pair so the rows are
        // individually resizable. Their headers all read "Tool Options" (their
        // shared category), matching the column header when they're recombined.
        auto toolsLeaf = DockNode::makeLeaf({"tools"});
        auto sizeLeaf = DockNode::makeLeaf({"brushsize"});
        auto settingsLeaf = DockNode::makeLeaf({"brushsettings"});
        auto colorLeaf = DockNode::makeLeaf({"brushcolor"});
        auto paletteLeaf = DockNode::makeLeaf({"colorpalette"});
        auto layersLeaf = DockNode::makeLeaf({"layers"});
        auto canvas = DockNode::makeCanvas();

        // Per-panel minimums, sized to what each panel's content needs before it
        // clips. Heights are generous enough for the header plus a usable slice
        // (the rows scroll, so these are the "still usable" floor, not the full
        // content height). Widths are the hard floor — there's no horizontal
        // scroll, so the color wheel in particular needs real width.
        toolsLeaf->minWidth = 48;
        toolsLeaf->minHeight = 96; // at least the two tool buttons

        sizeLeaf->minWidth = 130;     // 4-column dot grid stays legible
        sizeLeaf->minHeight = 70;     // header + one row of dots
        settingsLeaf->minWidth = 130; // sliders need width to be draggable
        settingsLeaf->minHeight = 70; // header + size slider + label
        colorLeaf->minWidth = 150;    // wheel + value slider side by side
        colorLeaf->minHeight = 90;    // header + a usable wheel
        paletteLeaf->minWidth = 120;  // at least ~4 swatches across
        paletteLeaf->minHeight = 70;  // header + preview + one swatch row
        layersLeaf->minWidth = 140;
        layersLeaf->minHeight = 100;

        // Stack the four option panels top-to-bottom. Each split fixes the top
        // child's height (a natural starting size for its content) and lets the
        // bottom child take the rest, so the palette at the very bottom absorbs
        // leftover space. Built bottom-up so the fixed heights read naturally.
        //
        //   size (fixed) / [ settings (fixed) / [ color (fixed) / palette ] ]
        auto colorPalette = DockNode::makeSplit(SplitDir::Horizontal, 0.5f,
                                                std::move(colorLeaf), std::move(paletteLeaf));
        colorPalette->fixedAPixels = 210; // wheel + slider + preview

        auto settingsRest = DockNode::makeSplit(SplitDir::Horizontal, 0.5f,
                                                std::move(settingsLeaf), std::move(colorPalette));
        settingsRest->fixedAPixels = 150; // size + opacity sliders + labels

        auto optionsColumn = DockNode::makeSplit(SplitDir::Horizontal, 0.5f,
                                                 std::move(sizeLeaf), std::move(settingsRest));
        optionsColumn->fixedAPixels = 190; // preset-dot grid

        // canvas | layers (layers fixed 200px)
        auto layersSplit = DockNode::makeSplit(SplitDir::Vertical, 0.5f,
                                               std::move(canvas), std::move(layersLeaf));
        layersSplit->fixedBPixels = 200;

        // toolOptions | (canvas|layers)  — options column fixed 180px on the left
        auto optionsSplit = DockNode::makeSplit(SplitDir::Vertical, 0.5f,
                                                std::move(optionsColumn), std::move(layersSplit));
        optionsSplit->fixedAPixels = 180;

        // tools | (options|...)  — tools fixed 64px on the left
        auto rootSplit = DockNode::makeSplit(SplitDir::Vertical, 0.5f,
                                             std::move(toolsLeaf), std::move(optionsSplit));
        rootSplit->fixedAPixels = 64;

        root_ = std::move(rootSplit);
    }

    void DockLayout::compute(const SDL_Rect &area)
    {
        placements_.clear();
        dividers_.clear();
        canvasRect_ = {0, 0, 0, 0};
        canvasLeaf_ = nullptr;
        if (root_)
            computeNode(root_.get(), area);
    }

    // Clamp child-a's primary-axis extent so neither child drops below its
    // subtree minimum — but only when the container can actually fit both. When
    // it can't (window too small), fall back to proportional splitting so one
    // side isn't starved to zero. `aExtent` is the proposed size for child a,
    // `usable` the span shared by both children (container minus divider).
    static int clampSplitExtent(int aExtent, int usable, int minA, int minB)
    {
        if (usable <= 0)
            return 0;
        if (minA + minB <= usable)
        {
            // Room for both: keep a in [minA, usable-minB].
            int lo = minA;
            int hi = usable - minB;
            if (lo > hi)
                lo = hi = usable / 2;
            return std::clamp(aExtent, lo, hi);
        }
        // Not enough room for both minimums: split in proportion to them so each
        // keeps a fair, non-zero slice (content scrolls / clips gracefully).
        int total = minA + minB;
        int a = total > 0 ? (int)((long long)usable * minA / total) : usable / 2;
        return std::clamp(a, 0, usable);
    }

    // A collapsed COLUMN occupies only the rail width, regardless of its stored
    // size. A column root may be a lone leaf or a horizontal-split subtree, so
    // this checks the collapse flag on the node itself (the column root carries
    // it) rather than requiring a leaf. Returns true + the forced extent.
    static bool collapsedExtent(DockNode *n, int rail, int &out)
    {
        if (n && n->collapsed && !n->isCanvasCenter)
        {
            out = rail;
            return true;
        }
        return false;
    }

    bool DockLayout::isPanelStack(DockNode *node)
    {
        if (!node)
            return false;
        if (node->isLeaf)
            return !node->isCanvasCenter; // a panel leaf (not the canvas)
        // A split is part of a column only if it stacks vertically (Horizontal
        // divider = top/bottom) and both sides are themselves panel stacks.
        if (node->dir != SplitDir::Horizontal)
            return false;
        return isPanelStack(node->a.get()) && isPanelStack(node->b.get());
    }

    void DockLayout::collectColumnLeaves(DockNode *node, std::vector<DockNode *> &out)
    {
        if (!node)
            return;
        if (node->isLeaf)
        {
            if (!node->isCanvasCenter)
                out.push_back(node);
            return;
        }
        collectColumnLeaves(node->a.get(), out);
        collectColumnLeaves(node->b.get(), out);
    }

    bool DockLayout::isColumnRoot(DockNode *node) const
    {
        // Must itself be a panel stack, and must not be nested inside a larger
        // panel stack — i.e. its parent is null or a vertical split. That makes
        // it the topmost node of its vertical strip.
        if (!isPanelStack(node))
            return false;
        bool isA = false;
        DockNode *parent = const_cast<DockLayout *>(this)->findParent(const_cast<DockNode *>(node), isA);
        if (!parent)
            return true; // node is the root
        // If the parent is also a panel stack (a horizontal split of panels),
        // then `node` is an interior leaf of a bigger column, not its root.
        return !(parent->dir == SplitDir::Horizontal && isPanelStack(parent));
    }

    void DockLayout::computeCollapsedColumn(DockNode *columnRoot, const SDL_Rect &rect)
    {
        // One rail for the whole column. Emit a single placement carrying every
        // panel id in the column so App can label the rail with the category.
        std::vector<DockNode *> leaves;
        collectColumnLeaves(columnRoot, leaves);

        PanelPlacement pl;
        pl.columnRoot = columnRoot;
        pl.leaf = leaves.empty() ? columnRoot : leaves.front();
        pl.collapsed = true;
        pl.isColumnTop = true;
        pl.fullRect = rect;
        pl.columnHeaderRect = rect;                       // the rail is its own strip
        pl.toggleRect = {rect.x, rect.y, rect.w, rect.w}; // square expand button at top
        pl.contentRect = {0, 0, 0, 0};
        for (DockNode *lf : leaves)
            for (const auto &id : lf->panelIds)
                pl.columnPanelIds.push_back(id);
        pl.id = pl.columnPanelIds.empty() ? std::string() : pl.columnPanelIds.front();
        placements_.push_back(pl);
    }

    void DockLayout::computeColumn(DockNode *columnRoot, const SDL_Rect &rect)
    {
        // A category header spans the top of the column and carries the single
        // collapse toggle. The stacked panels share the remaining height.
        SDL_Rect header = {rect.x, rect.y, rect.w, columnHeaderH_};
        int tb = columnHeaderH_;
        SDL_Rect toggle = {rect.x + rect.w - tb, rect.y, tb, columnHeaderH_};

        SDL_Rect body = {rect.x, rect.y + columnHeaderH_, rect.w, rect.h - columnHeaderH_};
        if (body.h < 0)
            body.h = 0;

        // Remember which leaf is the top one so it can carry the column header.
        std::vector<DockNode *> leaves;
        collectColumnLeaves(columnRoot, leaves);
        DockNode *topLeaf = leaves.empty() ? nullptr : leaves.front();

        // Recurse through the column's internal horizontal splits to place each
        // stacked leaf, but with a small local walker so we can tag the top leaf
        // and share the column root — computeNode would re-enter column logic.
        std::function<void(DockNode *, const SDL_Rect &)> place =
            [&](DockNode *n, const SDL_Rect &r)
        {
            n->rect = r;
            if (n->isLeaf)
            {
                PanelPlacement pl;
                pl.leaf = n;
                pl.columnRoot = columnRoot;
                pl.fullRect = r;
                pl.collapsed = false;
                pl.tabIds = n->panelIds;

                int active = n->activeTab;
                if (active < 0 || active >= (int)n->panelIds.size())
                    active = 0;
                n->activeTab = active;
                pl.activeTab = active;
                pl.id = n->panelIds.empty() ? std::string() : n->panelIds[active];

                // The top leaf also owns the column's category header + toggle.
                if (n == topLeaf)
                {
                    pl.isColumnTop = true;
                    pl.columnHeaderRect = header;
                    pl.toggleRect = toggle;
                    for (DockNode *lf : leaves)
                        for (const auto &id : lf->panelIds)
                            pl.columnPanelIds.push_back(id);
                }

                // Each panel gets its OWN title header directly below wherever it
                // sits (below the column header for the top leaf; at the leaf top
                // otherwise).
                int hy = r.y;
                pl.headerRect = {r.x, hy, r.w, headerH_};
                int contentTop = hy + headerH_;
                if (n->panelIds.size() > 1)
                {
                    pl.tabStripRect = {r.x, contentTop, r.w, tabH_};
                    contentTop += tabH_;
                }
                pl.contentRect = {r.x, contentTop, r.w, r.y + r.h - contentTop};
                if (pl.contentRect.h < 0)
                    pl.contentRect.h = 0;

                placements_.push_back(pl);
                return;
            }

            // Horizontal split inside the column: carve a divider, recurse.
            const int t = dividerThickness_;
            int usable = r.h - t;
            if (usable < 0)
                usable = 0;
            int aH;
            if (n->fixedAPixels > 0)
                aH = std::min(n->fixedAPixels, usable);
            else if (n->fixedBPixels > 0)
                aH = usable - std::min(n->fixedBPixels, usable);
            else
                aH = (int)(usable * n->ratio + 0.5f);
            aH = std::clamp(aH, 0, usable);
            // Keep each stacked panel at/above its height minimum so its header
            // and a usable content slice always fit (rows scroll beyond that).
            aH = clampSplitExtent(aH, usable,
                                  minHeightOf(n->a.get()), minHeightOf(n->b.get()));
            int bH = usable - aH;

            SDL_Rect ra = {r.x, r.y, r.w, aH};
            SDL_Rect div = {r.x, r.y + aH, r.w, t};
            SDL_Rect rb = {r.x, r.y + aH + t, r.w, bH};
            n->divider = div;
            dividers_.push_back(div);
            place(n->a.get(), ra);
            place(n->b.get(), rb);
        };
        place(columnRoot, body);
    }

    void DockLayout::computeNode(DockNode *node, const SDL_Rect &rect)
    {
        node->rect = rect;

        // Column interception: if this node is the root of a vertical strip of
        // stacked panels (a lone panel leaf, or a horizontal-split subtree of
        // panels), lay it out as a single column — one category header + one
        // collapse toggle up top, panels stacked below — rather than recursing
        // as independent leaves. The canvas and vertical splits fall through.
        if (isColumnRoot(node))
        {
            if (node->collapsed)
                computeCollapsedColumn(node, rect);
            else
                computeColumn(node, rect);
            return;
        }

        if (node->isLeaf)
        {
            if (node->isCanvasCenter)
            {
                canvasRect_ = rect;
                canvasLeaf_ = node;
                return;
            }

            // Any panel leaf that reaches here wasn't caught as a column root
            // above (it should have been). Treat it defensively as a lone
            // column so a panel is never dropped from the layout.
            if (node->collapsed)
                computeCollapsedColumn(node, rect);
            else
                computeColumn(node, rect);
            return;
        }

        // Split node: carve out the divider, then split the remaining span.
        const int t = dividerThickness_;

        if (node->dir == SplitDir::Vertical)
        {
            int usable = rect.w - t;
            if (usable < 0)
                usable = 0;

            int aW;
            int forced;
            bool aCollapsed = collapsedExtent(node->a.get(), railWidth_, forced);
            if (aCollapsed)
                aW = std::min(forced, usable);
            else if (collapsedExtent(node->b.get(), railWidth_, forced))
                aW = usable - std::min(forced, usable); // b is the rail
            else if (node->fixedAPixels > 0)
                aW = std::min(node->fixedAPixels, usable);
            else if (node->fixedBPixels > 0)
                aW = usable - std::min(node->fixedBPixels, usable);
            else
                aW = (int)(usable * node->ratio + 0.5f);
            aW = std::clamp(aW, 0, usable);
            // Enforce width minimums so a panel is never squeezed until its
            // assets spill sideways (there's no horizontal scroll). Skip when a
            // side is a collapsed rail — its width is intentionally fixed thin.
            if (!aCollapsed && !(node->b && node->b->collapsed))
                aW = clampSplitExtent(aW, usable,
                                      minWidthOf(node->a.get()), minWidthOf(node->b.get()));
            int bW = usable - aW;

            SDL_Rect ra = {rect.x, rect.y, aW, rect.h};
            SDL_Rect divider = {rect.x + aW, rect.y, t, rect.h};
            SDL_Rect rb = {rect.x + aW + t, rect.y, bW, rect.h};

            node->divider = divider;
            dividers_.push_back(divider);

            if (node->a)
                computeNode(node->a.get(), ra);
            if (node->b)
                computeNode(node->b.get(), rb);
        }
        else // Horizontal
        {
            int usable = rect.h - t;
            if (usable < 0)
                usable = 0;

            int aH;
            int forced;
            bool aCol = collapsedExtent(node->a.get(), railWidth_, forced);
            if (aCol)
                aH = std::min(forced, usable);
            else if (collapsedExtent(node->b.get(), railWidth_, forced))
                aH = usable - std::min(forced, usable);
            else if (node->fixedAPixels > 0)
                aH = std::min(node->fixedAPixels, usable);
            else if (node->fixedBPixels > 0)
                aH = usable - std::min(node->fixedBPixels, usable);
            else
                aH = (int)(usable * node->ratio + 0.5f);
            aH = std::clamp(aH, 0, usable);
            if (!aCol && !(node->b && node->b->collapsed))
                aH = clampSplitExtent(aH, usable,
                                      minHeightOf(node->a.get()), minHeightOf(node->b.get()));
            int bH = usable - aH;

            SDL_Rect ra = {rect.x, rect.y, rect.w, aH};
            SDL_Rect divider = {rect.x, rect.y + aH, rect.w, t};
            SDL_Rect rb = {rect.x, rect.y + aH + t, rect.w, bH};

            node->divider = divider;
            dividers_.push_back(divider);

            if (node->a)
                computeNode(node->a.get(), ra);
            if (node->b)
                computeNode(node->b.get(), rb);
        }
    }
}