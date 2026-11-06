#include "forge/algo/graphs.hpp"

#include <algorithm>
#include <cctype>
#include <deque>
#include <map>
#include <stdexcept>

#include "forge/algo/input.hpp"
#include "forge/algo/registry.hpp"
#include "forge/util/cat.hpp"
#include "forge/viz/scene.hpp"

namespace forge::algo {

using viz::GraphScene;
using viz::Mark;
using viz::Recorder;

namespace {

std::string trim(const std::string& s) {
    std::size_t a = 0, b = s.size();
    while (a < b && std::isspace(static_cast<unsigned char>(s[a]))) ++a;
    while (b > a && std::isspace(static_cast<unsigned char>(s[b - 1]))) --b;
    return s.substr(a, b - a);
}

// Layered layout: x = longest path from a source (only meaningful for DAGs).
GraphScene dagScene(const std::vector<std::string>& nodes,
                    const std::vector<std::pair<std::string, std::string>>& edges,
                    const std::map<std::string, int>& indegree, const std::vector<std::string>& done,
                    const std::deque<std::string>& queue, const std::string& active) {
    std::map<std::string, int> depth;
    for (const auto& n : nodes) depth[n] = 0;
    for (std::size_t pass = 0; pass < nodes.size(); ++pass)
        for (const auto& [a, b] : edges) depth[b] = std::max(depth[b], depth[a] + 1);
    std::map<int, int> perLayer;
    GraphScene g;
    for (const auto& n : nodes) {
        Mark m = Mark::None;
        if (std::find(done.begin(), done.end(), n) != done.end()) m = Mark::Sorted;
        if (std::find(queue.begin(), queue.end(), n) != queue.end()) m = Mark::Compare;
        if (n == active) m = Mark::Active;
        g.node(n, n, m);
        const int d = std::min(depth[n], static_cast<int>(nodes.size()));
        g.at(n, d, perLayer[d]++);
        auto it = indegree.find(n);
        if (it != indegree.end() && m != Mark::Sorted) g.sub(n, cat("in: ", it->second));
    }
    for (const auto& [a, b] : edges) {
        const bool used = std::find(done.begin(), done.end(), a) != done.end();
        g.edge(a, b, "", true, used ? Mark::Dim : Mark::None);
    }
    std::string order;
    for (const auto& n : done) order += (order.empty() ? "" : ", ") + n;
    g.var("order so far", order.empty() ? "(empty)" : order);
    return g;
}

}  // namespace

void parseEdges(const std::string& text, std::vector<std::string>& nodes,
                std::vector<std::pair<std::string, std::string>>& edges) {
    auto addNode = [&nodes](const std::string& n) {
        if (n.empty() || n.size() > 12) throw std::invalid_argument("names must be 1-12 characters");
        for (char c : n)
            if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_')
                throw std::invalid_argument("names may only use letters, digits and _");
        if (std::find(nodes.begin(), nodes.end(), n) == nodes.end()) nodes.push_back(n);
    };
    std::size_t start = 0;
    while (start <= text.size()) {
        const std::size_t comma = text.find(',', start);
        const std::string part = trim(text.substr(start, comma == std::string::npos ? std::string::npos : comma - start));
        if (!part.empty()) {
            const std::size_t gt = part.find('>');
            if (gt == std::string::npos) {
                addNode(part);  // a lone node with no edges
            } else {
                const std::string a = trim(part.substr(0, gt)), b = trim(part.substr(gt + 1));
                addNode(a);
                addNode(b);
                edges.emplace_back(a, b);
            }
        }
        if (comma == std::string::npos) break;
        start = comma + 1;
    }
    if (nodes.size() > 16) throw std::invalid_argument("at most 16 names");
}

// [snippet:graph.topo_sort]
// Kahn's algorithm: repeatedly take something that has no unmet
// prerequisites (in-degree 0), output it, and "cross it off" for others.
std::vector<std::string> topoSort(const std::vector<std::string>& nodes,
                                  const std::vector<std::pair<std::string, std::string>>& edges,
                                  Recorder& rec) {
    std::map<std::string, int> indegree;            // how many things must come first
    std::map<std::string, std::vector<std::string>> after;
    for (const auto& n : nodes) indegree[n] = 0;
    for (const auto& [a, b] : edges) {
        after[a].push_back(b);
        ++indegree[b];  // @line count
    }
    std::deque<std::string> ready;  // in-degree 0: can go right now
    for (const auto& n : nodes)
        if (indegree[n] == 0) ready.push_back(n);
    std::vector<std::string> order;
    rec.frame(dagScene(nodes, edges, indegree, order, ready, ""), "Count prerequisites (in-degree) for every node. Nodes with 0 are ready (yellow).", "count"); // @viz
    while (!ready.empty()) {
        const std::string n = ready.front();
        ready.pop_front();
        order.push_back(n);  // @line take
        rec.frame(dagScene(nodes, edges, indegree, order, ready, n), cat("Take ", n, ": nothing is blocking it. Add it to the order."), "take"); // @viz
        for (const auto& next : after[n]) {
            if (--indegree[next] == 0) ready.push_back(next);  // last blocker gone  // @line unlock
        }
        rec.frame(dagScene(nodes, edges, indegree, order, ready, ""), cat("Cross ", n, " off: everything after it has one prerequisite fewer."), "unlock"); // @viz
    }
    if (order.size() != nodes.size()) return {};  // leftovers wait on each other: a cycle  // @line cycle
    return order;
}
// [/snippet]

void registerGraphs(Registry& r) {
    r.add({"graph.topo_sort", "Topological sort (Kahn)", "Graphs", "graph.topo_sort", "O(V + E)", "O(V + E)",
           "Put tasks in an order that respects every \"A before B\" rule. Build systems, quest chains and skill trees all use it.",
           {{"edges", "text", "Rules like a>b, separated by commas"}},
           {{"edges", "socks>shoes, pants>shoes, pants>belt, shirt>belt, shirt>tie, tie>coat, belt>coat"}}},
          [](const Json& input, Recorder& rec) -> Json {
              const auto text = textValue(input, "edges", 300);
              std::vector<std::string> nodes;
              std::vector<std::pair<std::string, std::string>> edges;
              parseEdges(text, nodes, edges);
              if (nodes.empty()) throw std::invalid_argument("type at least one rule, like a>b");
              const auto order = topoSort(nodes, edges, rec);
              if (order.empty()) {
                  GraphScene g;
                  for (const auto& n : nodes) g.node(n, n, Mark::Error);
                  for (const auto& [a, b] : edges) g.edge(a, b, "", true, Mark::Error);
                  g.layoutCircle();
                  rec.frame(g, "Some nodes never reached in-degree 0: they wait on each other in a CYCLE. No valid order exists.", "cycle");
                  return {{"order", Json::array()}, {"cycle", true}};
              }
              return {{"order", order}, {"cycle", false}};
          });
}

}  // namespace forge::algo
