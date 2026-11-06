// Exercise 04: sliding window.
//
// Count how many windows of exactly k consecutive values have a sum >= target.
//
//   a = {2, 1, 5, 1, 3}, k = 2, target = 6  ->  windows: 3, 6, 6, 4  ->  answer 2
//
// Hint: build the first window's sum, then for each step add the value that
// enters and subtract the value that leaves. Never re-add the whole window.
// That turns O(n*k) into O(n).
#include <doctest/doctest.h>

#include <vector>

int countWindowsAtLeast(const std::vector<int>& a, int k, int target) {
    // TODO
    (void)a;
    (void)k;
    (void)target;
    return -1;
}

TEST_CASE("example") {
    CHECK(countWindowsAtLeast({2, 1, 5, 1, 3}, 2, 6) == 2);
}

TEST_CASE("window as wide as the array") {
    CHECK(countWindowsAtLeast({1, 2, 3}, 3, 6) == 1);
    CHECK(countWindowsAtLeast({1, 2, 3}, 3, 7) == 0);
}

TEST_CASE("negative numbers") {
    CHECK(countWindowsAtLeast({-1, -2, 4, -1}, 2, 1) == 2);  // sums: -3, 2, 3
}
