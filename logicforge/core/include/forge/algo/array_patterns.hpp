#pragma once
// Three patterns that solve a huge share of array interview problems.
#include <utility>
#include <vector>

#include "forge/viz/recorder.hpp"

namespace forge::algo {

// Two pointers: indices {i, j} with a[i] + a[j] == target in a SORTED
// array, or {-1, -1}.
std::pair<int, int> pairWithSum(const std::vector<int>& a, int target, viz::Recorder& rec);

// Sliding window: the largest sum of any k consecutive values.
// Requires 1 <= k <= a.size().
int maxWindowSum(const std::vector<int>& a, int k, viz::Recorder& rec);

// Prefix sums: prefix[i] = a[0] + ... + a[i-1], so prefix has size n + 1.
std::vector<int> prefixSums(const std::vector<int>& a, viz::Recorder& rec);
// Sum of a[l..r] (inclusive) in O(1) using the prefix array.
int rangeSum(const std::vector<int>& prefix, int l, int r);

}  // namespace forge::algo
