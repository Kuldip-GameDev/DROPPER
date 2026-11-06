#include "forge/viz/scene.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <numbers>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace forge::viz {

std::string_view toString(Mark m) {
    switch (m) {
        case Mark::None: return "none";
        case Mark::Active: return "active";
        case Mark::Compare: return "compare";
        case Mark::Swap: return "swap";
        case Mark::Sorted: return "sorted";
        case Mark::Found: return "found";
        case Mark::Visited: return "visited";
        case Mark::Dim: return "dim";
        case Mark::Window: return "window";
        case Mark::Pivot: return "pivot";
        case Mark::Error: return "error";
        case Mark::Ok: return "ok";
    }
    return "none";
}

namespace {
Json varsToJson(const std::vector<std::pair<std::string, Json>>& vars) {
    Json out = Json::array();
    for (const auto& [name, value] : vars) out.push_back({{"name", name}, {"value", value}});
    return out;
}
}  // namespace

// ---------------------------------------------------------------- ArrayScene

ArrayScene::ArrayScene(std::vector<int> values) : values_(std::move(values)) {}

ArrayScene ArrayScene::ofLabels(std::vector<std::string> labels) {
    ArrayScene s;
    s.labels_ = std::move(labels);
    return s;
}

ArrayScene& ArrayScene::title(std::string t) {
    title_ = std::move(t);
    return *this;
}

ArrayScene& ArrayScene::layout(Layout l) {
    layout_ = l;
    return *this;
}

ArrayScene& ArrayScene::mark(int index, Mark m) {
    marks_[index] = m;
    return *this;
}

ArrayScene& ArrayScene::markRange(int lo, int hiExclusive, Mark m) {
    for (int i = lo; i < hiExclusive; ++i) marks_[i] = m;
    return *this;
}

ArrayScene& ArrayScene::sortedFrom(int from) {
    const int n = static_cast<int>(labels_.empty() ? values_.size() : labels_.size());
    for (int i = std::max(from, 0); i < n; ++i) {
        // Do not hide a more interesting mark (e.g. a swap that just landed).
        if (!marks_.contains(i)) marks_[i] = Mark::Sorted;
    }
    return *this;
}

ArrayScene& ArrayScene::sortedUpTo(int toExclusive) {
    for (int i = 0; i < toExclusive; ++i) {
        if (!marks_.contains(i)) marks_[i] = Mark::Sorted;
    }
    return *this;
}

ArrayScene& ArrayScene::pointer(std::string name, int index) {
    pointers_.emplace_back(std::move(name), index);
    return *this;
}

ArrayScene& ArrayScene::window(int lo, int hiExclusive) {
    window_ = std::make_pair(lo, hiExclusive);
    return *this;
}

ArrayScene& ArrayScene::var(std::string name, Json value) {
    vars_.emplace_back(std::move(name), std::move(value));
    return *this;
}

Json ArrayScene::toJson() const {
    Json j;
    j["kind"] = "array";
    j["title"] = title_;
    switch (layout_) {
        case Layout::Row: j["layout"] = "row"; break;
        case Layout::Bars: j["layout"] = "bars"; break;
        case Layout::Stack: j["layout"] = "stack"; break;
    }
    if (labels_.empty()) {
        j["values"] = values_;
        Json labels = Json::array();
        for (int v : values_) labels.push_back(std::to_string(v));
        j["labels"] = labels;
    } else {
        j["values"] = Json::array();
        j["labels"] = labels_;
    }
    Json marks = Json::object();
    for (const auto& [i, m] : marks_) {
        if (m != Mark::None) marks[std::to_string(i)] = std::string(toString(m));
    }
    j["marks"] = marks;
    Json ptrs = Json::array();
    for (const auto& [name, idx] : pointers_) ptrs.push_back({{"name", name}, {"index", idx}});
    j["pointers"] = ptrs;
    if (window_) {
        j["window"] = {window_->first, window_->second};
    } else {
        j["window"] = nullptr;
    }
    j["vars"] = varsToJson(vars_);
    return j;
}

// ---------------------------------------------------------------- GraphScene

GraphScene& GraphScene::title(std::string t) {
    title_ = std::move(t);
    return *this;
}

GraphScene::Node* GraphScene::find(const std::string& id) {
    for (auto& n : nodes_) {
        if (n.id == id) return &n;
    }
    return nullptr;
}

bool GraphScene::hasNode(const std::string& id) const {
    return std::any_of(nodes_.begin(), nodes_.end(), [&](const Node& n) { return n.id == id; });
}

GraphScene& GraphScene::node(std::string id, std::string label, Mark m) {
    if (Node* existing = find(id)) {
        existing->label = std::move(label);
        existing->mark = m;
    } else {
        nodes_.push_back(Node{std::move(id), std::move(label), "", m, 0, 0});
    }
    return *this;
}

GraphScene& GraphScene::sub(const std::string& id, std::string text) {
    if (Node* n = find(id)) n->sub = std::move(text);
    return *this;
}

GraphScene& GraphScene::mark(const std::string& id, Mark m) {
    if (Node* n = find(id)) n->mark = m;
    return *this;
}

GraphScene& GraphScene::edge(std::string from, std::string to, std::string label, bool directed,
                             Mark m) {
    edges_.push_back(Edge{std::move(from), std::move(to), std::move(label), m, directed});
    return *this;
}

GraphScene& GraphScene::at(const std::string& id, double x, double y) {
    if (Node* n = find(id)) {
        n->x = x;
        n->y = y;
    }
    return *this;
}

GraphScene& GraphScene::pointer(std::string name, const std::string& nodeId) {
    pointers_.emplace_back(std::move(name), nodeId);
    return *this;
}

GraphScene& GraphScene::var(std::string name, Json value) {
    vars_.emplace_back(std::move(name), std::move(value));
    return *this;
}

GraphScene& GraphScene::layoutRow() {
    for (std::size_t i = 0; i < nodes_.size(); ++i) {
        nodes_[i].x = static_cast<double>(i);
        nodes_[i].y = 0;
    }
    return *this;
}

GraphScene& GraphScene::layoutCircle() {
    const double n = static_cast<double>(nodes_.size());
    const double radius = std::max(1.0, n / 3.0);
    for (std::size_t i = 0; i < nodes_.size(); ++i) {
        const double angle = 2 * std::numbers::pi * static_cast<double>(i) / n - std::numbers::pi / 2;
        nodes_[i].x = radius + radius * std::cos(angle);
        nodes_[i].y = radius + radius * std::sin(angle);
    }
    return *this;
}

// Tidy-ish tree layout:
//   * y = depth
//   * leaves get consecutive x slots (left to right)
//   * a parent sits in the middle of its first and last child
//   * for binary trees, an only child labelled "L"/"R" gets an empty
//     slot on the other side, so left and right children never look alike.
GraphScene& GraphScene::layoutTree(const std::string& root) {
    std::unordered_map<std::string, std::vector<const Edge*>> children;
    for (const auto& e : edges_) children[e.from].push_back(&e);

    double nextSlot = 0;
    std::unordered_set<std::string> seen;  // protects against cycles

    // A recursive lambda: std::function lets the lambda call itself.
    std::function<double(const std::string&, int)> place = [&](const std::string& id, int depth) {
        Node* n = find(id);
        if (n == nullptr || seen.contains(id)) return nextSlot;
        seen.insert(id);
        n->y = depth;
        static const std::vector<const Edge*> kNoChildren;
        const auto it = children.find(id);
        const auto& kids = it == children.end() ? kNoChildren : it->second;
        if (kids.empty()) {
            n->x = nextSlot++;
            return n->x;
        }
        const bool onlyRight = kids.size() == 1 && kids[0]->label == "R";
        const bool onlyLeft = kids.size() == 1 && kids[0]->label == "L";
        if (onlyRight) nextSlot += 1;  // empty slot where the left child would be
        double first = 0, last = 0;
        for (std::size_t i = 0; i < kids.size(); ++i) {
            const double x = place(kids[i]->to, depth + 1);
            if (i == 0) first = x;
            last = x;
        }
        if (onlyLeft) nextSlot += 1;  // empty slot where the right child would be
        if (onlyRight) first -= 1;
        if (onlyLeft) last += 1;
        n->x = (first + last) / 2;
        return n->x;
    };
    place(root, 0);
    return *this;
}

Json GraphScene::toJson() const {
    Json j;
    j["kind"] = "graph";
    j["title"] = title_;
    Json nodes = Json::array();
    for (const auto& n : nodes_) {
        nodes.push_back({{"id", n.id},
                         {"label", n.label},
                         {"sub", n.sub},
                         {"mark", std::string(toString(n.mark))},
                         {"x", n.x},
                         {"y", n.y}});
    }
    j["nodes"] = nodes;
    Json edges = Json::array();
    for (const auto& e : edges_) {
        edges.push_back({{"from", e.from},
                         {"to", e.to},
                         {"label", e.label},
                         {"mark", std::string(toString(e.mark))},
                         {"directed", e.directed}});
    }
    j["edges"] = edges;
    Json ptrs = Json::array();
    for (const auto& [name, id] : pointers_) ptrs.push_back({{"name", name}, {"node", id}});
    j["pointers"] = ptrs;
    j["vars"] = varsToJson(vars_);
    return j;
}

// ----------------------------------------------------------------- GridScene

GridScene::GridScene(int rows, int cols)
    : rows_(rows),
      cols_(cols),
      cells_(static_cast<std::size_t>(std::max(rows, 0) * std::max(cols, 0))),
      rowHeaders_(static_cast<std::size_t>(std::max(rows, 0))),
      colHeaders_(static_cast<std::size_t>(std::max(cols, 0))) {
    if (rows < 0 || cols < 0) throw std::invalid_argument("GridScene: negative size");
}

int GridScene::index(int r, int c) const {
    if (r < 0 || r >= rows_ || c < 0 || c >= cols_) throw std::out_of_range("GridScene: bad cell");
    return r * cols_ + c;
}

GridScene& GridScene::title(std::string t) {
    title_ = std::move(t);
    return *this;
}

GridScene& GridScene::set(int r, int c, std::string text) {
    cells_[static_cast<std::size_t>(index(r, c))] = std::move(text);
    return *this;
}

GridScene& GridScene::set(int r, int c, int value) { return set(r, c, std::to_string(value)); }

GridScene& GridScene::mark(int r, int c, Mark m) {
    marks_[index(r, c)] = m;
    return *this;
}

GridScene& GridScene::rowHeader(int r, std::string text) {
    rowHeaders_.at(static_cast<std::size_t>(r)) = std::move(text);
    return *this;
}

GridScene& GridScene::colHeader(int c, std::string text) {
    colHeaders_.at(static_cast<std::size_t>(c)) = std::move(text);
    return *this;
}

GridScene& GridScene::path(std::vector<std::pair<int, int>> cells, Mark m) {
    for (const auto& [r, c] : cells) index(r, c);  // validates
    paths_.emplace_back(std::move(cells), m);
    return *this;
}

GridScene& GridScene::var(std::string name, Json value) {
    vars_.emplace_back(std::move(name), std::move(value));
    return *this;
}

Json GridScene::toJson() const {
    Json j;
    j["kind"] = "grid";
    j["title"] = title_;
    j["rows"] = rows_;
    j["cols"] = cols_;
    j["cells"] = cells_;
    Json marks = Json::object();
    for (const auto& [i, m] : marks_) {
        if (m != Mark::None) marks[std::to_string(i)] = std::string(toString(m));
    }
    j["marks"] = marks;
    const bool anyRow = std::any_of(rowHeaders_.begin(), rowHeaders_.end(),
                                    [](const std::string& s) { return !s.empty(); });
    const bool anyCol = std::any_of(colHeaders_.begin(), colHeaders_.end(),
                                    [](const std::string& s) { return !s.empty(); });
    j["rowHeaders"] = anyRow ? Json(rowHeaders_) : Json::array();
    j["colHeaders"] = anyCol ? Json(colHeaders_) : Json::array();
    Json paths = Json::array();
    for (const auto& [cells, m] : paths_) {
        Json pts = Json::array();
        for (const auto& [r, c] : cells) pts.push_back({r, c});
        paths.push_back({{"cells", pts}, {"mark", std::string(toString(m))}});
    }
    j["paths"] = paths;
    j["vars"] = varsToJson(vars_);
    return j;
}

}  // namespace forge::viz
