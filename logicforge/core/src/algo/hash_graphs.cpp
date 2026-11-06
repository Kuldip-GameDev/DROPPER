// Hash tables, BFS pathfinding on a grid, and Dijkstra's shortest paths.
#include <algorithm>
#include <climits>
#include <cmath>
#include <tuple>
#include <functional>
#include <map>
#include <queue>
#include <stdexcept>

#include "forge/algo/hash_graphs.hpp"
#include "forge/algo/input.hpp"
#include "forge/algo/registry.hpp"
#include "forge/util/cat.hpp"
#include "forge/viz/scene.hpp"

namespace forge::algo {

using viz::GraphScene;
using viz::GridScene;
using viz::Mark;
using viz::Recorder;

// [snippet:hash.chaining]
// A hash set with "separate chaining": key -> bucket index -> a short list.
std::size_t HashSet::index(int key) const {
    const auto m = static_cast<long long>(buckets_.size());
    return static_cast<std::size_t>(((key % m) + m) % m);  // modulo that is never negative  // @line hash
}

bool HashSet::contains(int key) const {
    for (int k : buckets_[index(key)])  // only ONE short list is searched  // @line find
        if (k == key) return true;
    return false;
}

bool HashSet::insert(int key) {
    if (contains(key)) return false;
    // Keep the load factor (keys / buckets) at most 0.75, so chains stay short.
    if ((size_ + 1) * 4 > buckets_.size() * 3) rehash(buckets_.size() * 2 + 1);  // @line grow
    buckets_[index(key)].push_back(key);  // @line insert
    ++size_;
    return true;
}

void HashSet::rehash(std::size_t bucketCount) {
    std::vector<std::vector<int>> old = std::move(buckets_);
    buckets_.assign(bucketCount, {});
    for (const auto& chain : old)
        for (int k : chain) buckets_[index(k)].push_back(k);  // every key moves to its NEW bucket  // @line rehash
}
// [/snippet]

// [snippet:graph.bfs_grid]
// Shortest path on a grid where every step costs 1 = breadth-first search.
// BFS explores in rings: everything 1 step away, then 2 steps, then 3...
std::vector<Cell> bfsPath(const std::vector<std::string>& grid, Cell start, Cell goal) {
    const int rows = static_cast<int>(grid.size()), cols = static_cast<int>(grid[0].size());
    std::vector<std::vector<int>> dist(rows, std::vector<int>(cols, -1));
    std::vector<std::vector<Cell>> parent(rows, std::vector<Cell>(cols, {-1, -1}));
    std::queue<Cell> todo;
    dist[start.first][start.second] = 0;
    todo.push(start);
    const int dr[4] = {-1, 1, 0, 0}, dc[4] = {0, 0, -1, 1};
    while (!todo.empty()) {
        const auto [r, c] = todo.front();  // @line visit
        todo.pop();
        if (Cell{r, c} == goal) break;  // the first time we reach it is the shortest way
        for (int d = 0; d < 4; ++d) {
            const int nr = r + dr[d], nc = c + dc[d];
            if (nr < 0 || nr >= rows || nc < 0 || nc >= cols) continue;
            if (grid[nr][nc] == '#' || dist[nr][nc] != -1) continue;  // wall, or already seen
            dist[nr][nc] = dist[r][c] + 1;
            parent[nr][nc] = {r, c};  // remember how we got here  // @line enqueue
            todo.push({nr, nc});
        }
    }
    std::vector<Cell> path;  // walk the parents back from the goal
    if (dist[goal.first][goal.second] == -1) return path;
    for (Cell at = goal; at != Cell{-1, -1}; at = parent[at.first][at.second]) path.push_back(at);  // @line path
    std::reverse(path.begin(), path.end());
    return path;
}
// [/snippet]

// [snippet:graph.dijkstra]
// Shortest paths when edges have different costs. Always settle the
// closest unsettled node next, using a min-priority-queue.
std::vector<long long> dijkstra(const std::vector<std::vector<std::pair<int, int>>>& adj,
                                int source) {
    const long long INF = LLONG_MAX;
    std::vector<long long> dist(adj.size(), INF);
    using Item = std::pair<long long, int>;  // (distance so far, node)
    std::priority_queue<Item, std::vector<Item>, std::greater<>> pq;  // smallest distance on top
    dist[source] = 0;
    pq.push({0, source});
    while (!pq.empty()) {
        const auto [d, u] = pq.top();
        pq.pop();
        if (d > dist[u]) continue;  // an old, longer entry: skip it  // @line stale
        for (const auto& [v, w] : adj[u]) {
            if (dist[u] + w < dist[v]) {  // found a shorter way to v  // @line relax
                dist[v] = dist[u] + w;
                pq.push({dist[v], v});
            }
        }
    }
    return dist;
}
// [/snippet]

namespace {

GridScene hashScene(const HashSet& h, int hotKey) {
    const auto& b = h.buckets();
    std::size_t longest = 1;
    for (const auto& c : b) longest = std::max(longest, c.size());
    GridScene g(static_cast<int>(b.size()), static_cast<int>(longest));
    for (std::size_t i = 0; i < b.size(); ++i) {
        g.rowHeader(static_cast<int>(i), cat("[", i, "]"));
        for (std::size_t j = 0; j < longest; ++j) {
            if (j < b[i].size()) {
                g.set(static_cast<int>(i), static_cast<int>(j), b[i][j]);
                if (b[i][j] == hotKey) g.mark(static_cast<int>(i), static_cast<int>(j), Mark::Active);
            } else {
                g.set(static_cast<int>(i), static_cast<int>(j), "·").mark(static_cast<int>(i), static_cast<int>(j), Mark::Dim);
            }
        }
    }
    g.var("size", h.size()).var("buckets", b.size()).var("load", cat(std::round(100.0 * static_cast<double>(h.size()) / static_cast<double>(b.size())) / 100));
    return g;
}

std::vector<std::string> parseMaze(const std::string& text, Cell& start, Cell& goal) {
    std::vector<std::string> rows;
    std::string cur;
    for (char ch : text) {
        if (ch == '/') { rows.push_back(cur); cur.clear(); }
        else if (ch != ' ') cur += ch;
    }
    rows.push_back(cur);
    if (rows.size() < 2 || rows.size() > 8) throw std::invalid_argument("the maze needs 2-8 rows separated by /");
    int s = 0, gl = 0;
    for (std::size_t r = 0; r < rows.size(); ++r) {
        if (rows[r].size() != rows[0].size() || rows[r].size() < 2 || rows[r].size() > 10) throw std::invalid_argument("every row needs the same length (2-10)");
        for (std::size_t c = 0; c < rows[r].size(); ++c) {
            const char ch = rows[r][c];
            if (ch == 'S') { start = {static_cast<int>(r), static_cast<int>(c)}; ++s; }
            else if (ch == 'G') { goal = {static_cast<int>(r), static_cast<int>(c)}; ++gl; }
            else if (ch != '.' && ch != '#') throw std::invalid_argument("use . for floor, # for wall, S start, G goal");
        }
    }
    if (s != 1 || gl != 1) throw std::invalid_argument("the maze needs exactly one S and one G");
    return rows;
}

GridScene mazeScene(const std::vector<std::string>& maze, const std::vector<std::vector<int>>& dist) {
    const int rows = static_cast<int>(maze.size()), cols = static_cast<int>(maze[0].size());
    GridScene g(rows, cols);
    for (int r = 0; r < rows; ++r)
        for (int c = 0; c < cols; ++c) {
            const char ch = maze[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)];
            const int d = dist[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)];
            if (ch == '#') g.set(r, c, "█").mark(r, c, Mark::Dim);
            else if (ch == 'S' || ch == 'G') g.set(r, c, std::string(1, ch)).mark(r, c, Mark::Pivot);
            else if (d >= 0) g.set(r, c, d).mark(r, c, Mark::Visited);
            else g.set(r, c, "");
        }
    return g;
}

}  // namespace

void registerHashGraphs(Registry& r) {
    r.add({"hash.chaining", "Hash table (chaining)", "Hashing", "hash.chaining", "O(1) average", "O(n)",
           "Turn a key into a bucket number, keep each bucket's list short, grow when it gets crowded.",
           {{"array", "int_array", "Keys to insert"}}, {{"array", {12, 5, 19, 26, 8, 33, 40, 7}}}},
          [](const Json& input, Recorder& rec) -> Json {
              HashSet h(5);
              rec.frame(hashScene(h, INT_MIN), "5 empty buckets. bucket = key % 5.", "");
              for (int k : intArray(input, "array", 16)) {
                  const std::size_t before = h.buckets().size();
                  const bool added = h.insert(k);
                  const bool grew = h.buckets().size() != before;
                  rec.frame(hashScene(h, k), !added ? cat(k, " is already in the set: nothing to do.")
                                           : grew ? cat("Adding ", k, " would push the load factor over 0.75, so the table grows to ", h.buckets().size(), " buckets and every key is re-hashed. Then ", k, " goes into bucket ", k % static_cast<int>(h.buckets().size()), ".")
                                                  : cat(k, " % ", h.buckets().size(), " = ", ((k % static_cast<int>(h.buckets().size())) + static_cast<int>(h.buckets().size())) % static_cast<int>(h.buckets().size()), ": append to that bucket's list."),
                            grew ? "grow" : "insert");
              }
              return {{"size", h.size()}, {"buckets", h.buckets().size()}};
          });

    r.add({"graph.bfs_grid", "BFS pathfinding on a grid", "Graphs", "graph.bfs_grid", "O(rows x cols)", "O(rows x cols)",
           "Find the shortest path through a maze by exploring in rings of equal distance.",
           {{"maze", "text", "Rows separated by / (. floor, # wall, S, G)"}},
           {{"maze", "S...#...../.##.#.###./...#...#../.#.#.#.#.#/.#...#...G"}}},
          [](const Json& input, Recorder& rec) -> Json {
              Cell start, goal;
              const auto maze = parseMaze(textValue(input, "maze", 120), start, goal);
              const int rows = static_cast<int>(maze.size()), cols = static_cast<int>(maze[0].size());
              std::vector<std::vector<int>> dist(static_cast<std::size_t>(rows), std::vector<int>(static_cast<std::size_t>(cols), -1));
              dist[static_cast<std::size_t>(start.first)][static_cast<std::size_t>(start.second)] = 0;
              rec.frame(mazeScene(maze, dist), "Find the shortest way from S to G. Each step (up/down/left/right) costs 1.", "");
              // Visible BFS, ring by ring (same algorithm as bfsPath).
              std::vector<Cell> ring = {start};
              int d = 0;
              bool reached = false;
              const int dr[4] = {-1, 1, 0, 0}, dc[4] = {0, 0, -1, 1};
              while (!ring.empty() && !reached) {
                  std::vector<Cell> next;
                  for (auto [rr, cc] : ring)
                      for (int k = 0; k < 4; ++k) {
                          const int nr = rr + dr[k], nc = cc + dc[k];
                          if (nr < 0 || nr >= rows || nc < 0 || nc >= cols) continue;
                          if (maze[static_cast<std::size_t>(nr)][static_cast<std::size_t>(nc)] == '#' || dist[static_cast<std::size_t>(nr)][static_cast<std::size_t>(nc)] != -1) continue;
                          dist[static_cast<std::size_t>(nr)][static_cast<std::size_t>(nc)] = d + 1;
                          next.push_back({nr, nc});
                          if (Cell{nr, nc} == goal) reached = true;
                      }
                  ++d;
                  if (!next.empty()) {
                      auto s = mazeScene(maze, dist);
                      for (auto [rr, cc] : next) s.mark(rr, cc, Mark::Compare);
                      rec.frame(s, reached ? cat("Ring ", d, " reaches G. BFS found it at distance ", d, ", and nothing shorter can exist.") : cat("Ring ", d, ": every floor cell exactly ", d, " steps from S."), "enqueue");
                  }
                  ring = next;
              }
              const auto path = bfsPath(maze, start, goal);
              auto s = mazeScene(maze, dist);
              for (auto [rr, cc] : path) s.mark(rr, cc, Mark::Found);
              if (path.size() >= 2) s.path(path, Mark::Found);
              rec.frame(s, path.empty() ? "G can't be reached: every reachable cell was explored." : cat("Follow the parents back from G: a shortest path of ", path.size() - 1, " steps. Games use this (and A*, a smarter cousin) for enemy pathfinding."), "path");
              return {{"steps", path.empty() ? -1 : static_cast<int>(path.size()) - 1}};
          });

    r.add({"graph.dijkstra", "Dijkstra's shortest paths", "Graphs", "graph.dijkstra", "O((V + E) log V)", "O(V + E)",
           "Shortest routes when roads have different lengths: always settle the closest town next.",
           {{"edges", "text", "Roads like A-B:4, comma separated"}},
           {{"edges", "A-B:4, A-C:2, B-C:1, B-D:5, C-D:8, C-E:10, D-E:2, D-F:6, E-F:2"}}},
          [](const Json& input, Recorder& rec) -> Json {
              const auto text = textValue(input, "edges", 300);
              std::vector<std::string> names;
              std::vector<std::tuple<int, int, int>> edges;
              auto idOf = [&names](const std::string& n) {
                  if (n.empty() || n.size() > 6) throw std::invalid_argument("town names must be 1-6 characters");
                  auto it = std::find(names.begin(), names.end(), n);
                  if (it != names.end()) return static_cast<int>(it - names.begin());
                  names.push_back(n);
                  return static_cast<int>(names.size()) - 1;
              };
              std::size_t pos = 0;
              while (pos < text.size()) {
                  const std::size_t comma = std::min(text.find(',', pos), text.size());
                  std::string part = text.substr(pos, comma - pos);
                  part.erase(std::remove(part.begin(), part.end(), ' '), part.end());
                  pos = comma + 1;
                  if (part.empty()) continue;
                  const auto dash = part.find('-'), colon = part.find(':');
                  if (dash == std::string::npos || colon == std::string::npos || colon < dash) throw std::invalid_argument("write roads like A-B:4");
                  const int w = std::stoi(part.substr(colon + 1));
                  if (w < 0 || w > 999) throw std::invalid_argument("road lengths must be 0-999");
                  // Two statements on purpose: inside one call, C++ may evaluate the arguments in
                  // either order, which would number the towns differently (see "Hard-to-read C++").
                  const int from = idOf(part.substr(0, dash));
                  const int to = idOf(part.substr(dash + 1, colon - dash - 1));
                  edges.emplace_back(from, to, w);
              }
              if (names.empty() || names.size() > 12) throw std::invalid_argument("use 1-12 towns");
              std::vector<std::vector<std::pair<int, int>>> adj(names.size());
              for (auto [a, b, w] : edges) {
                  adj[static_cast<std::size_t>(a)].push_back({b, w});
                  adj[static_cast<std::size_t>(b)].push_back({a, w});
              }
              const long long INF = LLONG_MAX;
              std::vector<long long> dist(names.size(), INF);
              std::vector<bool> settled(names.size(), false);
              auto scene = [&](int active) {
                  GraphScene g;
                  for (std::size_t i = 0; i < names.size(); ++i) {
                      g.node(names[i], names[i], static_cast<int>(i) == active ? Mark::Active : settled[i] ? Mark::Sorted : dist[i] < INF ? Mark::Compare : Mark::None);
                      g.sub(names[i], dist[i] == INF ? "d = ∞" : cat("d = ", dist[i]));
                  }
                  for (auto [a, b, w] : edges) g.edge(names[static_cast<std::size_t>(a)], names[static_cast<std::size_t>(b)], std::to_string(w), false);
                  g.layoutCircle();
                  return g;
              };
              dist[0] = 0;
              using Item = std::pair<long long, int>;
              std::priority_queue<Item, std::vector<Item>, std::greater<>> pq;
              pq.push({0, 0});
              rec.frame(scene(-1), cat("Start at ", names[0], " (distance 0). Every other town starts at ∞."), "");
              while (!pq.empty()) {
                  const auto [d, u] = pq.top();
                  pq.pop();
                  if (d > dist[static_cast<std::size_t>(u)] || settled[static_cast<std::size_t>(u)]) continue;
                  settled[static_cast<std::size_t>(u)] = true;
                  std::string improved;
                  for (const auto& [v, w] : adj[static_cast<std::size_t>(u)]) {
                      if (dist[static_cast<std::size_t>(u)] + w < dist[static_cast<std::size_t>(v)]) {
                          dist[static_cast<std::size_t>(v)] = dist[static_cast<std::size_t>(u)] + w;
                          pq.push({dist[static_cast<std::size_t>(v)], v});
                          improved += (improved.empty() ? "" : ", ") + names[static_cast<std::size_t>(v)] + " = " + std::to_string(dist[static_cast<std::size_t>(v)]);
                      }
                  }
                  rec.frame(scene(u), cat(names[static_cast<std::size_t>(u)], " is the closest unsettled town (", d, "): its distance is final. ", improved.empty() ? "No neighbour improves." : "Shorter routes found: " + improved + "."), "relax");
              }
              const auto check = dijkstra(adj, 0);
              Json out = Json::object();
              for (std::size_t i = 0; i < names.size(); ++i) out[names[i]] = check[i] == INF ? Json(nullptr) : Json(check[i]);
              return out;
          });
}

}  // namespace forge::algo
