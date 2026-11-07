// Binary search trees, traversals and binary heaps.
#include <algorithm>
#include <memory>
#include <queue>
#include <stdexcept>
#include <vector>

#include "forge/algo/input.hpp"
#include "forge/algo/registry.hpp"
#include "forge/algo/trees_heaps.hpp"
#include "forge/util/cat.hpp"
#include "forge/viz/scene.hpp"

namespace forge::algo {

using viz::ArrayScene;
using viz::GraphScene;
using viz::Mark;
using viz::Recorder;

namespace {
// Node ids by pre-order position: stable within one picture, independent of addresses.
void addTree(GraphScene& g, const TreeNode* n, int& next, const std::vector<const TreeNode*>& hot,
             const std::vector<const TreeNode*>& done, const std::string& parent, const char* side) {
    if (n == nullptr) return;
    const std::string id = "t" + std::to_string(next++);
    Mark m = Mark::None;
    if (std::find(done.begin(), done.end(), n) != done.end()) m = Mark::Visited;
    if (std::find(hot.begin(), hot.end(), n) != hot.end()) m = Mark::Active;
    g.node(id, std::to_string(n->value), m);
    if (!parent.empty()) g.edge(parent, id, side);
    addTree(g, n->left.get(), next, hot, done, id, "L");
    addTree(g, n->right.get(), next, hot, done, id, "R");
}

GraphScene treeScene(const TreeNode* root, std::vector<const TreeNode*> hot = {}, std::vector<const TreeNode*> done = {}) {
    GraphScene g;
    int next = 0;
    addTree(g, root, next, hot, done, "", "");
    if (root) g.layoutTree("t0");
    return g;
}
}  // namespace

// [snippet:tree.bst]
// Each node OWNS its children (unique_ptr), so deleting the root frees the whole tree.
// insert takes a reference to the owning pointer, so it can fill an empty spot in place.
void bstInsert(std::unique_ptr<TreeNode>& node, int value, const TreeNode* root, Recorder& rec) {
    if (!node) {
        node = std::make_unique<TreeNode>(TreeNode{value, nullptr, nullptr});  // empty spot  // @line place
        rec.frame(treeScene(root ? root : node.get(), {node.get()}), cat("Empty spot found: ", value, " goes here."), "place"); // @viz
        return;
    }
    rec.frame(treeScene(root, {node.get()}), cat(value, value < node->value ? " < " : " >= ", node->value, value < node->value ? ": go left." : ": go right."), "compare"); // @viz
    if (value < node->value) bstInsert(node->left, value, root, rec);  // smaller: left subtree  // @line compare
    else bstInsert(node->right, value, root, rec);  // bigger or equal: right subtree
}

const TreeNode* bstFind(const TreeNode* node, int target) {
    while (node != nullptr && node->value != target) {
        node = target < node->value ? node->left.get() : node->right.get();  // @line descend
    }
    return node;  // nullptr if the value isn't in the tree  // @line found
}
// [/snippet]

// [snippet:tree.traversals]
// Depth-first, recursively. The only difference is WHEN we visit the node.
void inorder(const TreeNode* n, std::vector<int>& out) {
    if (!n) return;
    inorder(n->left.get(), out);
    out.push_back(n->value);  // between the children: sorted order for a BST  // @line visit_in
    inorder(n->right.get(), out);
}

void preorder(const TreeNode* n, std::vector<int>& out) {
    if (!n) return;
    out.push_back(n->value);  // before the children: copy/serialise a tree  // @line visit_pre
    preorder(n->left.get(), out);
    preorder(n->right.get(), out);
}

// Breadth-first: a queue visits the tree level by level.
std::vector<int> levelOrder(const TreeNode* root) {
    std::vector<int> out;
    std::queue<const TreeNode*> todo;
    if (root) todo.push(root);
    while (!todo.empty()) {
        const TreeNode* n = todo.front();
        todo.pop();
        out.push_back(n->value);  // @line visit_level
        if (n->left) todo.push(n->left.get());
        if (n->right) todo.push(n->right.get());
    }
    return out;
}
// [/snippet]

// [snippet:heap.binary_heap]
// A min-heap stored in a plain array: the children of i are 2i+1 and 2i+2,
// the parent of i is (i-1)/2. Rule: every parent <= its children.
void heapPush(std::vector<int>& heap, int value) {
    heap.push_back(value);  // add at the end...
    std::size_t i = heap.size() - 1;
    while (i > 0 && heap[(i - 1) / 2] > heap[i]) {  // ...then bubble up  // @line sift_up
        std::swap(heap[(i - 1) / 2], heap[i]);
        i = (i - 1) / 2;
    }
}

int heapPop(std::vector<int>& heap) {
    const int top = heap.front();
    heap.front() = heap.back();  // move the last leaf to the root...
    heap.pop_back();
    std::size_t i = 0;
    while (true) {  // ...then sink it down
        const std::size_t l = 2 * i + 1, r = 2 * i + 2;
        std::size_t smallest = i;
        if (l < heap.size() && heap[l] < heap[smallest]) smallest = l;
        if (r < heap.size() && heap[r] < heap[smallest]) smallest = r;
        if (smallest == i) break;  // @line sift_down
        std::swap(heap[i], heap[smallest]);
        i = smallest;
    }
    return top;
}
// [/snippet]

namespace {

GraphScene heapScene(const std::vector<int>& h, int hot = -1, int hot2 = -1) {
    GraphScene g;
    for (std::size_t i = 0; i < h.size(); ++i) {
        const auto id = "h" + std::to_string(i);
        g.node(id, std::to_string(h[i]), static_cast<int>(i) == hot ? Mark::Active : static_cast<int>(i) == hot2 ? Mark::Compare : Mark::None);
        g.sub(id, cat("[", i, "]"));
        if (i > 0) g.edge("h" + std::to_string((i - 1) / 2), id, (i % 2 == 1) ? "L" : "R", false);
    }
    if (!h.empty()) g.layoutTree("h0");
    std::string arr;
    for (int v : h) arr += (arr.empty() ? "" : " ") + std::to_string(v);
    g.var("array", "[" + arr + "]");
    return g;
}

std::unique_ptr<TreeNode> buildBst(const std::vector<int>& values) {
    std::unique_ptr<TreeNode> root;
    Recorder discard;
    for (int v : values) bstInsert(root, v, root.get(), discard);
    return root;
}

}  // namespace

std::unique_ptr<TreeNode> buildBstFrom(const std::vector<int>& values) { return buildBst(values); }
viz::GraphScene drawTree(const TreeNode* root) { return treeScene(root); }
viz::GraphScene drawHeap(const std::vector<int>& heap) { return heapScene(heap); }

void registerTreesHeaps(Registry& r) {
    r.add({"tree.bst", "Binary search tree", "Trees & heaps", "tree.bst", "O(log n) average, O(n) worst", "O(n)",
           "Smaller values go left, bigger go right, so searching skips half the tree at every step (if it's balanced).",
           {{"array", "int_array", "Insert in this order"}, {"target", "int", "Then search for"}},
           {{"array", {8, 3, 10, 1, 6, 14, 4, 7}}, {"target", 7}}},
          [](const Json& input, Recorder& rec) -> Json {
              const auto values = intArray(input, "array", 15);
              const int target = intValue(input, "target", -kMaxAbsValue, kMaxAbsValue);
              std::unique_ptr<TreeNode> root;
              for (int v : values) {
                  rec.frame(treeScene(root.get()), cat("Insert ", v, ": start at the root."), "");
                  bstInsert(root, v, root.get(), rec);
              }
              // Search, drawn step by step.
              const TreeNode* n = root.get();
              std::vector<const TreeNode*> path;
              while (n != nullptr && n->value != target) {
                  path.push_back(n);
                  rec.frame(treeScene(root.get(), {n}, path), cat("Search ", target, ": ", target < n->value ? "smaller than " : "bigger than ", n->value, ", go ", target < n->value ? "left." : "right."), "descend");
                  n = target < n->value ? n->left.get() : n->right.get();
              }
              rec.frame(treeScene(root.get(), n ? std::vector<const TreeNode*>{n} : std::vector<const TreeNode*>{}, path),
                        n ? cat("Found ", target, " after ", path.size() + 1, " comparisons.") : cat(target, " is not in the tree (we fell off at an empty spot)."), "found");
              std::vector<int> sorted;
              inorder(root.get(), sorted);
              return {{"found", n != nullptr}, {"inorder", sorted}};
          });

    r.add({"tree.traversals", "Tree traversals", "Trees & heaps", "tree.traversals", "O(n)", "O(height)",
           "In-order, pre-order and level-order visit the same nodes in different orders, each useful for something.",
           {{"array", "int_array", "Build a BST from"}}, {{"array", {8, 3, 10, 1, 6, 14, 4, 7}}}},
          [](const Json& input, Recorder& rec) -> Json {
              const auto root = buildBst(intArray(input, "array", 15));
              rec.frame(treeScene(root.get()), "The tree. Watch the visiting order of each traversal.", "");
              auto play = [&](const std::vector<int>& order, const char* name, const char* line, const char* why) {
                  std::vector<const TreeNode*> done;
                  std::vector<const TreeNode*> all;
                  std::queue<const TreeNode*> q;
                  if (root) q.push(root.get());
                  while (!q.empty()) { all.push_back(q.front()); if (q.front()->left) q.push(q.front()->left.get()); if (q.front()->right) q.push(q.front()->right.get()); q.pop(); }
                  std::string seq;
                  for (int v : order) {
                      for (const auto* n : all) if (n->value == v && std::find(done.begin(), done.end(), n) == done.end()) { done.push_back(n); break; }
                      seq += (seq.empty() ? "" : " ") + std::to_string(v);
                      rec.frame(treeScene(root.get(), {done.back()}, done).var(name, seq), cat(name, ": visit ", v, "."), line);
                  }
                  rec.frame(treeScene(root.get(), {}, done).var(name, seq), cat(name, " done: ", seq, ". ", why), line);
              };
              std::vector<int> in, pre;
              inorder(root.get(), in);
              preorder(root.get(), pre);
              play(in, "in-order", "visit_in", "For a BST that's sorted order: a free sort!");
              play(pre, "pre-order", "visit_pre", "Root first: inserting in this order rebuilds the same tree, which is how you save one to a file.");
              play(levelOrder(root.get()), "level-order", "visit_level", "Level by level with a queue: BFS on a tree.");
              return {{"inorder", in}, {"preorder", pre}, {"levelorder", levelOrder(root.get())}};
          });

    r.add({"heap.binary_heap", "Binary heap (priority queue)", "Trees & heaps", "heap.binary_heap", "O(log n) push/pop", "O(n)",
           "A tree stored in an array that always knows its smallest element. std::priority_queue is one.",
           {{"array", "int_array", "Push these, then pop all"}}, {{"array", {5, 9, 3, 7, 1, 8}}}},
          [](const Json& input, Recorder& rec) -> Json {
              const auto values = intArray(input, "array", 15);
              std::vector<int> h;
              for (int v : values) {
                  h.push_back(v);
                  std::size_t i = h.size() - 1;
                  rec.frame(heapScene(h, static_cast<int>(i)), cat("Push ", v, ": add it at the end of the array (the next free leaf)."), "sift_up");
                  while (i > 0 && h[(i - 1) / 2] > h[i]) {
                      const std::size_t p = (i - 1) / 2;
                      rec.frame(heapScene(h, static_cast<int>(i), static_cast<int>(p)), cat(h[i], " < its parent ", h[p], ": swap them."), "sift_up");
                      std::swap(h[p], h[i]);
                      i = p;
                  }
              }
              rec.frame(heapScene(h, 0), cat("The smallest value, ", h.empty() ? 0 : h[0], ", is always at index 0."), "");
              std::vector<int> popped;
              while (!h.empty()) {
                  const int top = heapPop(h);
                  popped.push_back(top);
                  rec.frame(heapScene(h, h.empty() ? -1 : 0).var("popped", popped), cat("Pop ", top, ": the last leaf moves to the root and sinks down until the heap rule holds."), "sift_down");
              }
              return {{"popped", popped}};
          });
}

}  // namespace forge::algo
