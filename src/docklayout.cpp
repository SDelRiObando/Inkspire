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

    // The minimum primary-axis size a subtree wants. For a leaf it's its
    // minPixels; for a split it's the min of the child on the shared side plus
    // structure. We keep it simple: use the immediate child's minPixels if it's
    // a leaf, else a small floor.
    static int sideMin(DockNode *n)
    {
        if (!n)
            return 40;
        if (n->isLeaf)
            return n->collapsed ? 0 : n->minPixels;
        return 40; // nested splits: modest floor (refined later if needed)
    }

    void DockLayout::dragDivider(DockNode *node, int mx, int my)
    {
        if (!node || node->isLeaf)
            return;

        const SDL_Rect &r = node->rect; // the node's full area from last compute
        const int t = dividerThickness_;
        int minA = sideMin(node->a.get());
        int minB = sideMin(node->b.get());

        if (node->dir == SplitDir::Vertical)
        {
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
        for (const auto &pl : placements_)
            if (rectContains(pl.toggleRect, x, y))
                return pl.leaf;
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
        newLeaf->minPixels = 120;

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
        //          [tooloptions]     Split(V, layers = 200px on right)
        //                            /                                \
        //                        Canvas center                    [layers]
        //
        // Every dock is a fixed-width strip; the canvas center is the leftover.

        auto toolsLeaf = DockNode::makeLeaf({"tools"});
        auto optionsLeaf = DockNode::makeLeaf({"tooloptions"});
        auto layersLeaf = DockNode::makeLeaf({"layers"});
        auto canvas = DockNode::makeCanvas();

        // Minimum widths: content panels need room for sliders/wheel; the tools
        // strip is happy thin.
        toolsLeaf->minPixels = 48;
        optionsLeaf->minPixels = 120;
        layersLeaf->minPixels = 120;

        // canvas | layers (layers fixed 200px)
        auto layersSplit = DockNode::makeSplit(SplitDir::Vertical, 0.5f,
                                               std::move(canvas), std::move(layersLeaf));
        layersSplit->fixedBPixels = 200;

        // tooloptions | (canvas|layers)  — options fixed 180px on the left
        auto optionsSplit = DockNode::makeSplit(SplitDir::Vertical, 0.5f,
                                                std::move(optionsLeaf), std::move(layersSplit));
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

    // A collapsed leaf occupies only the rail width, regardless of its stored
    // size. Returns true and the forced extent if `n` is a collapsed leaf.
    static bool collapsedExtent(DockNode *n, int rail, int &out)
    {
        if (n && n->isLeaf && n->collapsed && !n->isCanvasCenter)
        {
            out = rail;
            return true;
        }
        return false;
    }

    void DockLayout::computeNode(DockNode *node, const SDL_Rect &rect)
    {
        node->rect = rect;

        if (node->isLeaf)
        {
            if (node->isCanvasCenter)
            {
                canvasRect_ = rect;
                canvasLeaf_ = node;
                return;
            }

            // A panel column, possibly with multiple tabbed panels.
            PanelPlacement pl;
            pl.leaf = node;
            pl.fullRect = rect;
            pl.collapsed = node->collapsed;
            pl.tabIds = node->panelIds;

            // Clamp the active tab and pick the visible panel.
            int active = node->activeTab;
            if (active < 0 || active >= (int)node->panelIds.size())
                active = 0;
            node->activeTab = active;
            pl.activeTab = active;
            pl.id = node->panelIds.empty() ? std::string() : node->panelIds[active];

            if (node->collapsed)
            {
                pl.headerRect = rect;                             // the rail acts as its own header strip
                pl.toggleRect = {rect.x, rect.y, rect.w, rect.w}; // square expand button at top
                pl.contentRect = {0, 0, 0, 0};
            }
            else
            {
                pl.headerRect = {rect.x, rect.y, rect.w, headerH_};
                int tb = headerH_; // square toggle button at header's right
                pl.toggleRect = {rect.x + rect.w - tb, rect.y, tb, headerH_};

                int contentTop = rect.y + headerH_;
                // If tabbed, reserve a strip below the header for the tab buttons.
                if (node->panelIds.size() > 1)
                {
                    pl.tabStripRect = {rect.x, contentTop, rect.w, tabH_};
                    contentTop += tabH_;
                }
                pl.contentRect = {rect.x, contentTop, rect.w, rect.y + rect.h - contentTop};
                if (pl.contentRect.h < 0)
                    pl.contentRect.h = 0;
            }

            placements_.push_back(pl);
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
            if (collapsedExtent(node->a.get(), railWidth_, forced))
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
            if (collapsedExtent(node->a.get(), railWidth_, forced))
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