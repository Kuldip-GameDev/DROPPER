#include <doctest/doctest.h>

#include <algorithm>
#include <random>
#include <vector>

#include "forge/algo/array_patterns.hpp"

using namespace forge;

TEST_CASE("two pointers matches brute force on sorted arrays") {
    std::mt19937 rng(99);
    for (int trial = 0; trial < 300; ++trial) {
        std::vector<int> a(static_cast<std::size_t>(rng() % 12));
        for (int& x : a) x = static_cast<int>(rng() % 40) - 20;
        std::sort(a.begin(), a.end());
        const int target = static_cast<int>(rng() % 40) - 20;
        bool exists = false;
        for (std::size_t i = 0; i < a.size(); ++i)
            for (std::size_t j = i + 1; j < a.size(); ++j)
                if (a[i] + a[j] == target) exists = true;
        viz::Recorder rec;
        const auto [l, r] = algo::pairWithSum(a, target, rec);
        if (exists) {
            REQUIRE(l >= 0);
            CHECK(l < r);
            CHECK(a[static_cast<std::size_t>(l)] + a[static_cast<std::size_t>(r)] == target);
        } else {
            CHECK(l == -1);
            CHECK(r == -1);
        }
    }
}

TEST_CASE("sliding window matches brute force") {
    std::mt19937 rng(5);
    for (int trial = 0; trial < 300; ++trial) {
        const int n = 1 + static_cast<int>(rng() % 15);
        std::vector<int> a(static_cast<std::size_t>(n));
        for (int& x : a) x = static_cast<int>(rng() % 21) - 10;
        const int k = 1 + static_cast<int>(rng() % static_cast<unsigned>(n));
        int best = -1'000'000;
        for (int i = 0; i + k <= n; ++i) {
            int s = 0;
            for (int j = i; j < i + k; ++j) s += a[static_cast<std::size_t>(j)];
            best = std::max(best, s);
        }
        viz::Recorder rec;
        CHECK(algo::maxWindowSum(a, k, rec) == best);
    }
}

TEST_CASE("prefix sums answer range queries") {
    const std::vector<int> a = {3, 1, 4, 1, 5, 9};
    viz::Recorder rec;
    const auto prefix = algo::prefixSums(a, rec);
    CHECK(prefix == std::vector<int>{0, 3, 4, 8, 9, 14, 23});
    for (int l = 0; l < 6; ++l) {
        for (int r = l; r < 6; ++r) {
            int s = 0;
            for (int i = l; i <= r; ++i) s += a[static_cast<std::size_t>(i)];
            CHECK(algo::rangeSum(prefix, l, r) == s);
        }
    }
}
