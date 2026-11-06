#include <doctest/doctest.h>

#include <algorithm>
#include <random>
#include <vector>

#include "forge/algo/searching.hpp"

using namespace forge;

TEST_CASE("binary search finds every present value and rejects absent ones") {
    std::mt19937 rng(7);
    for (int trial = 0; trial < 30; ++trial) {
        std::vector<int> a(static_cast<std::size_t>(rng() % 20));
        for (int& x : a) x = static_cast<int>(rng() % 60) - 30;
        std::sort(a.begin(), a.end());
        a.erase(std::unique(a.begin(), a.end()), a.end());
        for (int target = -32; target <= 32; ++target) {
            viz::Recorder rec;
            const int idx = algo::binarySearch(a, target, rec);
            const auto it = std::find(a.begin(), a.end(), target);
            if (it == a.end()) {
                CHECK(idx == -1);
            } else {
                CHECK(idx == static_cast<int>(it - a.begin()));
            }
        }
    }
}

TEST_CASE("binary search takes O(log n) steps") {
    std::vector<int> a(32);
    for (int i = 0; i < 32; ++i) a[static_cast<std::size_t>(i)] = i * 2;
    viz::Recorder rec;
    CHECK(algo::binarySearch(a, -1, rec) == -1);
    // Each loop iteration records 2 frames; log2(32) + 1 = 6 iterations max.
    CHECK(rec.frames().size() <= 2 * 6 + 1);
}

TEST_CASE("linear search returns the first match") {
    viz::Recorder rec;
    CHECK(algo::linearSearch({4, 2, 7, 2}, 2, rec) == 1);
    CHECK(algo::linearSearch({4, 2, 7, 2}, 9, rec) == -1);
    CHECK(algo::linearSearch({}, 1, rec) == -1);
}
