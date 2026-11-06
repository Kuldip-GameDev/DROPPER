#pragma once
// Scenes describe WHAT to draw. They never draw anything themselves.
//
// The Android app has one renderer per scene kind. The terminal tool
// (forge_cli) has a text renderer for each kind. Adding a new visual
// means adding a new scene kind here, not touching every algorithm.
//
// Each scene is built with a small "fluent builder": every setter
// returns *this, so calls can be chained:
//
//     ArrayScene(values).compare(2, 3).pointer("i", 2).title("Pass 1")
//
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "forge/json.hpp"

namespace forge::viz {

// How a cell / node should be highlighted. The UI maps each to a colour.
enum class Mark {
    None,
    Active,   // the thing we are looking at right now
    Compare,  // two things being compared
    Swap,     // two things being swapped / moved
    Sorted,   // finished, will not change again
    Found,    // the answer
    Visited,  // already processed (graphs, flood fill)
    Dim,      // out of the current range, ignore
    Window,   // inside the current window / range
    Pivot,    // quicksort pivot, special element
    Error,    // something went wrong (dangling pointer, mismatch)
    Ok,       // check passed
};

std::string_view toString(Mark m);

// ---------------------------------------------------------------------------
// ArrayScene: a row of boxes (or bars, or a vertical stack).
// ---------------------------------------------------------------------------
class ArrayScene {
public:
    enum class Layout { Row, Bars, Stack };

    explicit ArrayScene(std::vector<int> values);
    // Text cells (e.g. characters on a stack). Values are not drawn as bars.
    static ArrayScene ofLabels(std::vector<std::string> labels);

    ArrayScene& title(std::string t);
    ArrayScene& layout(Layout l);
    ArrayScene& mark(int index, Mark m);
    ArrayScene& markRange(int lo, int hiExclusive, Mark m);
    ArrayScene& compare(int a, int b) { return mark(a, Mark::Compare).mark(b, Mark::Compare); }
    ArrayScene& swapped(int a, int b) { return mark(a, Mark::Swap).mark(b, Mark::Swap); }
    // Marks [from, size) as sorted. Handy for algorithms that grow a sorted tail.
    ArrayScene& sortedFrom(int from);
    ArrayScene& sortedUpTo(int toExclusive);
    // A named arrow under a cell, e.g. "i", "lo", "mid". index may be -1 or size().
    ArrayScene& pointer(std::string name, int index);
    // Visually group [lo, hiExclusive) as the current window / sub-range.
    ArrayScene& window(int lo, int hiExclusive);
    // A small "variable chip" shown beside the scene, e.g. sum = 12.
    ArrayScene& var(std::string name, Json value);

    Json toJson() const;

private:
    ArrayScene() = default;
    std::vector<int> values_;
    std::vector<std::string> labels_;
    std::string title_;
    Layout layout_ = Layout::Row;
    std::map<int, Mark> marks_;
    std::vector<std::pair<std::string, int>> pointers_;
    std::optional<std::pair<int, int>> window_;
    std::vector<std::pair<std::string, Json>> vars_;
};

// ---------------------------------------------------------------------------
// GraphScene: nodes and arrows. Used for linked lists, trees, graphs and
// class diagrams. Positions are in abstract units; the renderer scales
// them to fit the screen.
// ---------------------------------------------------------------------------
class GraphScene {
public:
    struct Node {
        std::string id;
        std::string label;
        std::string sub;  // small text under the node, e.g. "next = null"
        Mark mark = Mark::None;
        double x = 0;
        double y = 0;
    };
    struct Edge {
        std::string from;
        std::string to;
        std::string label;
        Mark mark = Mark::None;
        bool directed = true;
    };

    GraphScene& title(std::string t);
    GraphScene& node(std::string id, std::string label, Mark m = Mark::None);
    GraphScene& sub(const std::string& id, std::string text);
    GraphScene& mark(const std::string& id, Mark m);
    GraphScene& edge(std::string from, std::string to, std::string label = "",
                     bool directed = true, Mark m = Mark::None);
    GraphScene& at(const std::string& id, double x, double y);
    // A named label pointing at a node, e.g. "head", "slow", "fast".
    GraphScene& pointer(std::string name, const std::string& nodeId);
    GraphScene& var(std::string name, Json value);

    // Layout helpers (they only set x/y).
    GraphScene& layoutRow();                         // left to right, insertion order
    GraphScene& layoutTree(const std::string& root); // tidy layered tree (children = edge order)
    GraphScene& layoutCircle();

    bool hasNode(const std::string& id) const;
    Json toJson() const;

private:
    Node* find(const std::string& id);
    std::vector<Node> nodes_;
    std::vector<Edge> edges_;
    std::vector<std::pair<std::string, std::string>> pointers_;
    std::vector<std::pair<std::string, Json>> vars_;
    std::string title_;
};

// ---------------------------------------------------------------------------
// GridScene: rows x columns of text cells. Used for slot reels, DP tables,
// flood fill, N-Queens and two-row comparisons (array vs prefix sums).
// ---------------------------------------------------------------------------
class GridScene {
public:
    GridScene(int rows, int cols);

    GridScene& title(std::string t);
    GridScene& set(int r, int c, std::string text);
    GridScene& set(int r, int c, int value);
    GridScene& mark(int r, int c, Mark m);
    GridScene& rowHeader(int r, std::string text);
    GridScene& colHeader(int c, std::string text);
    // A polyline through cells, e.g. a slot payline. Each point is {row, col}.
    GridScene& path(std::vector<std::pair<int, int>> cells, Mark m = Mark::Active);
    GridScene& var(std::string name, Json value);

    int rows() const { return rows_; }
    int cols() const { return cols_; }
    Json toJson() const;

private:
    int index(int r, int c) const;
    int rows_;
    int cols_;
    std::vector<std::string> cells_;
    std::map<int, Mark> marks_;
    std::vector<std::string> rowHeaders_;
    std::vector<std::string> colHeaders_;
    std::vector<std::pair<std::vector<std::pair<int, int>>, Mark>> paths_;
    std::vector<std::pair<std::string, Json>> vars_;
    std::string title_;
};

// ---------------------------------------------------------------------------
// MemoryScene: the Pointer & Lambda Lab picture. Stack frames on the left,
// heap blocks on the right, arrows from pointers to what they point at.
// Addresses are "friendly" (0x7ff0, 0x5000...) but come from REAL pointer
// values at runtime, see memlab/address_book.hpp.
// ---------------------------------------------------------------------------
class MemoryScene {
public:
    struct Cell {
        std::string name;
        std::string type;
        std::string value;
        std::string addr;
        Mark mark = Mark::None;
    };
    struct Frame {
        std::string function;
        std::vector<Cell> locals;
    };
    struct Block {
        std::string addr;
        std::string label;
        std::vector<Cell> cells;
        Mark mark = Mark::None;
        bool freed = false;
    };
    struct Arrow {
        std::string from;  // address of the pointer itself
        std::string to;    // address it points at
        Mark mark = Mark::None;
        std::string label;
    };

    MemoryScene& title(std::string t);
    // Starts a new stack frame; following local() calls go into it.
    MemoryScene& frame(std::string function);
    MemoryScene& local(std::string name, std::string type, std::string value, std::string addr,
                       Mark m = Mark::None);
    // Starts a new heap block; following cell() calls go into it.
    MemoryScene& block(std::string addr, std::string label, Mark m = Mark::None, bool freed = false);
    MemoryScene& cell(std::string name, std::string type, std::string value, std::string addr,
                      Mark m = Mark::None);
    MemoryScene& arrow(std::string from, std::string to, Mark m = Mark::None, std::string label = "");
    MemoryScene& output(std::vector<std::string> lines);
    MemoryScene& var(std::string name, Json value);

    Json toJson() const;

private:
    std::string title_;
    std::vector<Frame> stack_;
    std::vector<Block> heap_;
    std::vector<Arrow> arrows_;
    std::vector<std::string> output_;
    std::vector<std::pair<std::string, Json>> vars_;
};

// ---------------------------------------------------------------------------
// PlotScene: bar charts and line charts (RNG histograms, RTP convergence,
// training loss). Values are plain doubles; the renderer scales them.
// ---------------------------------------------------------------------------
class PlotScene {
public:
    PlotScene& title(std::string t);
    PlotScene& axes(std::string xLabel, std::string yLabel);
    PlotScene& bar(std::string label, double value, Mark m = Mark::None);
    PlotScene& line(std::string name, std::vector<std::pair<double, double>> points, Mark m = Mark::Active);
    // A horizontal reference line, e.g. the expected value or the exact RTP.
    PlotScene& reference(double y, std::string label);
    PlotScene& var(std::string name, Json value);

    Json toJson() const;

private:
    struct Bar {
        std::string label;
        double value;
        Mark mark;
    };
    struct Line {
        std::string name;
        std::vector<std::pair<double, double>> points;
        Mark mark;
    };
    std::string title_, xLabel_, yLabel_;
    std::vector<Bar> bars_;
    std::vector<Line> lines_;
    std::vector<std::pair<double, std::string>> refs_;
    std::vector<std::pair<std::string, Json>> vars_;
};

}  // namespace forge::viz
