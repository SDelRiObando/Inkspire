#pragma once

#include <SDL.h>
#include <memory>
#include <string>
#include <vector>

namespace Inkspire
{
    // A dock layout is a binary tree of splits. Each node is either:
    //   - a Split: divides its rectangle into two children (left|right for a
    //     vertical divider, top/bottom for a horizontal one) at a ratio, or
    //   - a Leaf: holds panel identities (by string id) and fills its rectangle.
    //     A leaf with no panel ids is the canvas center (the drawing area).
    //
    // The tree stores panel *ids*, not Panel* — matching how Photoshop/CSP work.
    // App owns the actual Panel objects and resolves ids to them. This keeps the
    // tree decoupled from panel lifetimes and makes the layout serializable
    // (a memory address can't be written to disk; "layers" can).

    enum class SplitDir
    {
        Vertical,  // divider is a vertical bar; children are left | right
        Horizontal // divider is a horizontal bar; children are top / bottom
    };

    struct DockNode
    {
        // --- Split fields (used when !isLeaf) ---
        bool isLeaf = true;
        SplitDir dir = SplitDir::Vertical;
        float ratio = 0.5f;          // fraction of the usable span given to child a
        std::unique_ptr<DockNode> a; // left / top
        std::unique_ptr<DockNode> b; // right / bottom

        // A dock may prefer a fixed pixel size for a child rather than a pure
        // ratio (e.g. the tools strip is naturally 64px, the layers dock ~200px,
        // not "some % of window"). When > 0 these override the ratio for that
        // child's primary-axis extent. At most one side is typically fixed.
        int fixedAPixels = 0; // child a wants this many px (0 = use ratio)
        int fixedBPixels = 0; // child b wants this many px (0 = use ratio)

        // --- Leaf fields (used when isLeaf) ---
        std::vector<std::string> panelIds; // panels stacked as tabs (one for now)
        bool isCanvasCenter = false;       // true = the drawing area, not a panel

        // Minimum primary-axis size (px) this node may be shrunk to by a divider
        // drag. Content panels (tool options, layers) want ~120; the tools strip
        // is happy much thinner. Applied to whichever side owns it.
        int minPixels = 40;

        // Collapsed columns render as a thin rail (name sideways + >>). When
        // collapsed, the layout gives this leaf a fixed thin width regardless of
        // its normal size, and the freed space flows to the canvas.
        bool collapsed = false;

        // When a leaf holds multiple panels (tabs), which one is shown.
        int activeTab = 0;

        // Rectangle assigned during the last computeLayout pass (for hit-testing
        // dividers and, later, drag targeting).
        SDL_Rect rect{0, 0, 0, 0};

        // Divider rectangle for a split (empty for leaves), captured each layout.
        SDL_Rect divider{0, 0, 0, 0};

        static std::unique_ptr<DockNode> makeLeaf(std::vector<std::string> ids)
        {
            auto n = std::make_unique<DockNode>();
            n->isLeaf = true;
            n->panelIds = std::move(ids);
            return n;
        }

        static std::unique_ptr<DockNode> makeCanvas()
        {
            auto n = std::make_unique<DockNode>();
            n->isLeaf = true;
            n->isCanvasCenter = true;
            return n;
        }

        static std::unique_ptr<DockNode> makeSplit(SplitDir dir, float ratio,
                                                   std::unique_ptr<DockNode> a,
                                                   std::unique_ptr<DockNode> b)
        {
            auto n = std::make_unique<DockNode>();
            n->isLeaf = false;
            n->dir = dir;
            n->ratio = ratio;
            n->a = std::move(a);
            n->b = std::move(b);
            return n;
        }
    };

    // Result of a layout pass: where each panel goes, and where the canvas goes.
    // Where a panel column has been placed and how to draw it. A column is a
    // leaf that may hold one or more panels; for now one panel per leaf.
    struct PanelPlacement
    {
        std::string id;
        DockNode *leaf = nullptr; // the column this panel lives in (for toggling)

        SDL_Rect fullRect;    // the whole leaf rectangle (header + content)
        SDL_Rect headerRect;  // the title/collapse bar along the top
        SDL_Rect toggleRect;  // the << / >> collapse button (top-right of header)
        SDL_Rect contentRect; // where the panel body draws (below the header)

        bool collapsed = false; // when true, render as a thin rail instead

        // Tabs: all panel ids in this column and which is active. When there's
        // more than one, App draws a tab strip and `id` is the active tab.
        std::vector<std::string> tabIds;
        int activeTab = 0;
        SDL_Rect tabStripRect{0, 0, 0, 0}; // where the tab buttons sit
    };

    // Where a dragged panel would land when dropped over a target leaf.
    enum class DropZone
    {
        None,
        Left,   // split target: new panel on the left half
        Right,  // split target: new panel on the right half
        Top,    // split target: new panel on the top half
        Bottom, // split target: new panel on the bottom half
        Center  // tab into the target leaf
    };

    struct DropTarget
    {
        DockNode *leaf = nullptr; // the panel column under the cursor
        DropZone zone = DropZone::None;
        SDL_Rect highlight{0, 0, 0, 0}; // the region the panel would occupy
    };

    class DockLayout
    {
    public:
        // Build the default arrangement that reproduces the current UI:
        // tools strip on the left, layers on the right, canvas in the middle.
        void buildDefault();

        // Recompute every node's rectangle from the given available area
        // (typically the window minus the menu bar). Populates placements and
        // the canvas rect, and records divider rects for drawing/hit-testing.
        void compute(const SDL_Rect &area);

        const std::vector<PanelPlacement> &placements() const { return placements_; }
        const SDL_Rect &canvasRect() const { return canvasRect_; }
        DockNode *canvasLeaf() const { return canvasLeaf_; }

        // Divider rectangles captured during the last compute (for rendering the
        // bars now, and for drag hit-testing in step 3).
        const std::vector<SDL_Rect> &dividers() const { return dividers_; }

        int dividerThickness() const { return dividerThickness_; }
        int railWidth() const { return railWidth_; }
        // If (x,y) is on a tabbed leaf's tab strip, return the leaf and set
        // `tabIndex` to the clicked tab; else nullptr.
        DockNode *hitTestTab(int x, int y, int &tabIndex);

        int headerHeight() const { return headerH_; }

        DockNode *root() { return root_.get(); }

        // Return the split node whose divider bar contains (x, y), or nullptr.
        // Used to start a divider drag.
        DockNode *hitTestDivider(int x, int y);

        // Return the leaf whose collapse toggle contains (x, y), or nullptr.
        DockNode *hitTestCollapseToggle(int x, int y);

        // Given a cursor position over the docks, return which panel column it's
        // over and which of the 5 drop zones (edges = split, center = tab),
        // along with the highlight rectangle for that zone. Used while dragging
        // a panel to show where it would land. Skips the canvas center.
        DropTarget hitTestDropZone(int x, int y);

        // Apply a drag-to-redock drop: move `panelId` out of `sourceLeaf` and
        // into `target` (split for an edge zone, tab for center). Handles orphan
        // collapse of the source and silently rejects no-op drops. Returns true
        // if the tree actually changed (caller should recompute). Does NOT call
        // compute() itself.
        bool applyDrop(DockNode *sourceLeaf, const std::string &panelId, const DropTarget &target);

        // Dock a panel that isn't currently in the tree (e.g. a floating panel
        // being re-docked). Inserts at the target; there is no source to remove.
        bool applyDropFromFloat(const std::string &panelId, const DropTarget &target);

        // Remove `panelId` from `sourceLeaf` and collapse the leaf if it becomes
        // empty. Used to tear a panel off the dock into a float. Returns true if
        // the panel was found and removed.
        bool detachPanel(DockNode *sourceLeaf, const std::string &panelId);

        // Resize the split `node` so its divider tracks the mouse coordinate
        // (mx for a vertical divider, my for a horizontal one). Updates the
        // node's fixed-pixel size if it uses one, otherwise its ratio. Clamps so
        // neither child shrinks below its minPixels. Caller recomputes after.
        void dragDivider(DockNode *node, int mx, int my);

    private:
        void computeNode(DockNode *node, const SDL_Rect &rect);

        // Tree-surgery helpers for applyDrop.
        // Find the split node whose child (a or b) is `child`; nullptr if `child`
        // is the root or not found. Also reports which side via `isChildA`.
        DockNode *findParent(DockNode *node, DockNode *child, bool &isChildA);
        DockNode *findParent(DockNode *child, bool &isChildA);
        // Remove a leaf that has become empty: replace its parent split with the
        // leaf's sibling (promoting the sibling up one level).
        void collapseOrphan(DockNode *emptyLeaf);
        // Insert `panelId` at the target (tab for Center, split for an edge).
        // Shared by applyDrop and applyDropFromFloat. Does not touch any source.
        bool insertPanelAt(const std::string &panelId, const DropTarget &target);

        std::unique_ptr<DockNode> root_;
        std::vector<PanelPlacement> placements_;
        std::vector<SDL_Rect> dividers_;
        SDL_Rect canvasRect_{0, 0, 0, 0};
        DockNode *canvasLeaf_ = nullptr; // the canvas-center leaf, for edge drops
        int dividerThickness_ = 4;
        int railWidth_ = 26; // width of a collapsed column's rail
        int headerH_ = 22;   // panel header bar height
        int tabH_ = 20;      // tab strip height for multi-panel leaves
    };
}