#include <doctest/doctest.h>

#include <algorithm>
#include <climits>
#include <functional>
#include <random>
#include <set>

#include "forge/algo/dp_backtracking.hpp"
#include "forge/algo/hash_graphs.hpp"
#include "forge/algo/trees_heaps.hpp"

using namespace forge;
using namespace forge::algo;

TEST_CASE("BST: in-order is sorted, find works, level order is by depth") {
    std::mt19937 gen(21);
    for (int trial = 0; trial < 100; ++trial) {
        std::vector<int> v(gen() % 15);
        for (int& x : v) x = static_cast<int>(gen() % 30);
        const auto root = buildBstFrom(v);
        std::vector<int> in;
        inorder(root.get(), in);
        auto sorted = v;
        std::sort(sorted.begin(), sorted.end());
        CHECK(in == sorted);
        for (int t = 0; t < 30; ++t) {
            const bool present = std::find(v.begin(), v.end(), t) != v.end();
            CHECK((bstFind(root.get(), t) != nullptr) == present);
        }
    }
    const auto root = buildBstFrom({8, 3, 10, 1, 6, 14});
    CHECK(levelOrder(root.get()) == std::vector<int>{8, 3, 10, 1, 6, 14});
    std::vector<int> pre;
    preorder(root.get(), pre);
    CHECK(pre == std::vector<int>{8, 3, 1, 6, 10, 14});
}

TEST_CASE("binary heap pops in sorted order") {
    std::mt19937 gen(22);
    for (int trial = 0; trial < 100; ++trial) {
        std::vector<int> v(1 + gen() % 20), h;
        for (int& x : v) x = static_cast<int>(gen() % 50) - 25;
        for (int x : v) heapPush(h, x);
        std::vector<int> out;
        while (!h.empty()) out.push_back(heapPop(h));
        std::sort(v.begin(), v.end());
        CHECK(out == v);
    }
}

TEST_CASE("hash set behaves like std::set and keeps the load factor <= 0.75") {
    std::mt19937 gen(23);
    HashSet h(5);
    std::set<int> ref;
    for (int i = 0; i < 500; ++i) {
        const int k = static_cast<int>(gen() % 400) - 200;  // negatives too
        CHECK(h.insert(k) == ref.insert(k).second);
        CHECK(h.size() * 4 <= h.buckets().size() * 3);
    }
    for (int k = -200; k < 200; ++k) CHECK(h.contains(k) == (ref.count(k) == 1));
}

TEST_CASE("BFS path is valid and shortest") {
    std::mt19937 gen(24);
    for (int trial = 0; trial < 60; ++trial) {
        const int rows = 3 + static_cast<int>(gen() % 5), cols = 3 + static_cast<int>(gen() % 6);
        std::vector<std::string> g(static_cast<std::size_t>(rows), std::string(static_cast<std::size_t>(cols), '.'));
        for (auto& row : g) for (char& c : row) if (gen() % 4 == 0) c = '#';
        g[0][0] = '.';
        g[static_cast<std::size_t>(rows - 1)][static_cast<std::size_t>(cols - 1)] = '.';
        // Reference: relax distances until nothing changes.
        std::vector<std::vector<int>> d(static_cast<std::size_t>(rows), std::vector<int>(static_cast<std::size_t>(cols), INT_MAX));
        d[0][0] = 0;
        for (bool changed = true; changed;) {
            changed = false;
            for (int r = 0; r < rows; ++r)
                for (int c = 0; c < cols; ++c) {
                    if (g[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)] == '#' || d[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)] == INT_MAX) continue;
                    const int dr[4] = {-1, 1, 0, 0}, dc[4] = {0, 0, -1, 1};
                    for (int k = 0; k < 4; ++k) {
                        const int nr = r + dr[k], nc = c + dc[k];
                        if (nr < 0 || nr >= rows || nc < 0 || nc >= cols || g[static_cast<std::size_t>(nr)][static_cast<std::size_t>(nc)] == '#') continue;
                        if (d[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)] + 1 < d[static_cast<std::size_t>(nr)][static_cast<std::size_t>(nc)]) {
                            d[static_cast<std::size_t>(nr)][static_cast<std::size_t>(nc)] = d[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)] + 1;
                            changed = true;
                        }
                    }
                }
        }
        const auto path = bfsPath(g, {0, 0}, {rows - 1, cols - 1});
        const int expected = d[static_cast<std::size_t>(rows - 1)][static_cast<std::size_t>(cols - 1)];
        if (expected == INT_MAX) {
            CHECK(path.empty());
            continue;
        }
        REQUIRE(static_cast<int>(path.size()) == expected + 1);
        for (std::size_t i = 1; i < path.size(); ++i) {
            CHECK(std::abs(path[i].first - path[i - 1].first) + std::abs(path[i].second - path[i - 1].second) == 1);
            CHECK(g[static_cast<std::size_t>(path[i].first)][static_cast<std::size_t>(path[i].second)] != '#');
        }
    }
}

TEST_CASE("Dijkstra matches Bellman-Ford") {
    std::mt19937 gen(25);
    for (int trial = 0; trial < 100; ++trial) {
        const int n = 2 + static_cast<int>(gen() % 8);
        std::vector<std::vector<std::pair<int, int>>> adj(static_cast<std::size_t>(n));
        std::vector<std::tuple<int, int, int>> edges;
        for (int e = 0; e < n * 2; ++e) {
            const int a = static_cast<int>(gen() % static_cast<unsigned>(n)), b = static_cast<int>(gen() % static_cast<unsigned>(n)), w = static_cast<int>(gen() % 20);
            adj[static_cast<std::size_t>(a)].push_back({b, w});
            edges.emplace_back(a, b, w);
        }
        std::vector<long long> bf(static_cast<std::size_t>(n), LLONG_MAX);
        bf[0] = 0;
        for (int i = 0; i < n; ++i)
            for (auto [a, b, w] : edges)
                if (bf[static_cast<std::size_t>(a)] != LLONG_MAX && bf[static_cast<std::size_t>(a)] + w < bf[static_cast<std::size_t>(b)]) bf[static_cast<std::size_t>(b)] = bf[static_cast<std::size_t>(a)] + w;
        CHECK(dijkstra(adj, 0) == bf);
    }
}

TEST_CASE("coin change and LCS match brute force") {
    std::function<int(const std::vector<int>&, int)> brute = [&](const std::vector<int>& coins, int amount) {
        if (amount == 0) return 0;
        int best = -1;
        for (int c : coins)
            if (c <= amount) {
                const int sub = brute(coins, amount - c);
                if (sub >= 0 && (best < 0 || sub + 1 < best)) best = sub + 1;
            }
        return best;
    };
    for (int amount = 0; amount <= 18; ++amount) {
        CHECK(coinChange({1, 3, 4}, amount) == brute({1, 3, 4}, amount));
        CHECK(coinChange({5, 7}, amount) == brute({5, 7}, amount));
    }
    std::function<int(const std::string&, const std::string&)> lcsBrute = [&](const std::string& a, const std::string& b) {
        if (a.empty() || b.empty()) return 0;
        if (a.back() == b.back()) return 1 + lcsBrute(a.substr(0, a.size() - 1), b.substr(0, b.size() - 1));
        return std::max(lcsBrute(a.substr(0, a.size() - 1), b), lcsBrute(a, b.substr(0, b.size() - 1)));
    };
    std::mt19937 gen(26);
    for (int trial = 0; trial < 100; ++trial) {
        std::string a(gen() % 7, 'a'), b(gen() % 7, 'a');
        for (char& c : a) c = static_cast<char>('a' + gen() % 3);
        for (char& c : b) c = static_cast<char>('a' + gen() % 3);
        CHECK(lcsLength(a, b) == lcsBrute(a, b));
    }
}

TEST_CASE("N-Queens: known solution counts and valid boards") {
    const int counts[] = {0, 0, 0, 0, 2, 10, 4, 40, 92};
    for (int n = 4; n <= 8; ++n) {
        CHECK(countQueens(n) == counts[n]);
        std::vector<int> col;
        REQUIRE(solveQueens(n, col));
        for (int r1 = 0; r1 < n; ++r1)
            for (int r2 = r1 + 1; r2 < n; ++r2) {
                CHECK(col[static_cast<std::size_t>(r1)] != col[static_cast<std::size_t>(r2)]);
                CHECK(std::abs(col[static_cast<std::size_t>(r1)] - col[static_cast<std::size_t>(r2)]) != r2 - r1);
            }
    }
    std::vector<int> col;
    CHECK_FALSE(solveQueens(3, col));
}
