// Exercise 08: weighted symbol picks (how a virtual reel chooses a symbol).
//
// 1. runningTotals({40, 30, 20, 10}) -> {40, 70, 90, 100}
// 2. pickWeighted(totals, r): r is a random number in [0, total).
//    Return the FIRST index whose running total is GREATER than r.
//        totals {40, 70, 90, 100}:  r = 0..39 -> 0,  40..69 -> 1,  70..89 -> 2,  90..99 -> 3
//    Use binary search (O(log n)), not a loop over everything.
//
// Hint: this is exactly std::upper_bound. Write it by hand first, then compare.
#include <doctest/doctest.h>

#include <cstdint>
#include <random>
#include <vector>

std::vector<std::uint64_t> runningTotals(const std::vector<std::uint32_t>& weights) {
    // TODO
    (void)weights;
    return {};
}

std::size_t pickWeighted(const std::vector<std::uint64_t>& totals, std::uint64_t r) {
    // TODO
    (void)totals;
    (void)r;
    return 999;
}

TEST_CASE("running totals") {
    CHECK(runningTotals({40, 30, 20, 10}) == std::vector<std::uint64_t>{40, 70, 90, 100});
    CHECK(runningTotals({}).empty());
}

TEST_CASE("boundaries of each slice") {
    const std::vector<std::uint64_t> t = {40, 70, 90, 100};
    CHECK(pickWeighted(t, 0) == 0);
    CHECK(pickWeighted(t, 39) == 0);
    CHECK(pickWeighted(t, 40) == 1);
    CHECK(pickWeighted(t, 69) == 1);
    CHECK(pickWeighted(t, 70) == 2);
    CHECK(pickWeighted(t, 99) == 3);
}

TEST_CASE("every value of r maps to the right symbol exactly weight times") {
    std::mt19937 gen(8);
    for (int trial = 0; trial < 20; ++trial) {
        std::vector<std::uint32_t> w(1 + gen() % 7);
        for (auto& x : w) x = 1 + gen() % 30;
        const auto t = runningTotals(w);
        REQUIRE(t.size() == w.size());
        std::vector<std::uint32_t> seen(w.size(), 0);
        for (std::uint64_t r = 0; r < t.back(); ++r) ++seen.at(pickWeighted(t, r));
        CHECK(seen == w);
    }
}
