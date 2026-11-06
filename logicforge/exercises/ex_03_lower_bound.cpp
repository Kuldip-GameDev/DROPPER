// Exercise 03: lowerBound, the most useful binary search.
//
// Return the FIRST index i such that a[i] >= x, or a.size() if there is none.
// `a` is sorted. This is exactly what std::lower_bound does.
//
//   a = {1, 3, 3, 5},  x = 3  ->  1
//   a = {1, 3, 3, 5},  x = 4  ->  3
//   a = {1, 3, 3, 5},  x = 9  ->  4
//
// Hint: keep the invariant
//   "everything before lo is < x, everything from hi onwards is >= x"
// and loop while lo < hi. Use mid = lo + (hi - lo) / 2.
#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <random>
#include <vector>

std::size_t lowerBound(const std::vector<int>& a, int x) {
    // TODO
    (void)a;
    (void)x;
    return 12345;
}

TEST_CASE("examples from the description") {
    const std::vector<int> a = {1, 3, 3, 5};
    CHECK(lowerBound(a, 3) == 1);
    CHECK(lowerBound(a, 4) == 3);
    CHECK(lowerBound(a, 9) == 4);
    CHECK(lowerBound(a, 0) == 0);
    CHECK(lowerBound({}, 1) == 0);
}

TEST_CASE("agrees with std::lower_bound") {
    std::mt19937 rng(3);
    for (int trial = 0; trial < 200; ++trial) {
        std::vector<int> a(rng() % 12);
        for (int& v : a) v = static_cast<int>(rng() % 10);
        std::sort(a.begin(), a.end());
        for (int x = -1; x <= 10; ++x) {
            const auto expected = static_cast<std::size_t>(std::lower_bound(a.begin(), a.end(), x) - a.begin());
            CHECK(lowerBound(a, x) == expected);
        }
    }
}
