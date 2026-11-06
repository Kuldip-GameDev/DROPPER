#pragma once
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace forge::algo {

class HashSet {
public:
    explicit HashSet(std::size_t buckets = 7) : buckets_(buckets) {}
    bool insert(int key);  // false if already present
    bool contains(int key) const;
    std::size_t size() const { return size_; }
    const std::vector<std::vector<int>>& buckets() const { return buckets_; }

private:
    std::size_t index(int key) const;
    void rehash(std::size_t bucketCount);
    std::vector<std::vector<int>> buckets_;
    std::size_t size_ = 0;
};

using Cell = std::pair<int, int>;  // {row, col}
// '#' = wall. Returns the cells from start to goal, or empty if unreachable.
std::vector<Cell> bfsPath(const std::vector<std::string>& grid, Cell start, Cell goal);

// adj[u] = {(v, weight)...}, weights >= 0. Unreachable nodes get LLONG_MAX.
std::vector<long long> dijkstra(const std::vector<std::vector<std::pair<int, int>>>& adj, int source);

}  // namespace forge::algo
