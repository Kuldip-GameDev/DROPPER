#pragma once
#include <utility>
#include <vector>

#include "forge/viz/recorder.hpp"

namespace forge::algo {

// Return the index of `target`, or -1 if it is not there.
int linearSearch(const std::vector<int>& a, int target, viz::Recorder& rec);
// `a` must be sorted ascending.
int binarySearch(const std::vector<int>& a, int target, viz::Recorder& rec);

}  // namespace forge::algo
