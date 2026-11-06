#include "forge/algo/array_patterns.hpp"

#include <algorithm>
#include <stdexcept>

#include "forge/algo/input.hpp"
#include "forge/algo/registry.hpp"
#include "forge/util/cat.hpp"
#include "forge/viz/scene.hpp"

namespace forge::algo {

using viz::ArrayScene;
using viz::GridScene;
using viz::Mark;
using viz::Recorder;

// [snippet:pattern.two_pointers]
std::pair<int, int> pairWithSum(const std::vector<int>& a, int target, Recorder& rec) {
    int left = 0;
    int right = static_cast<int>(a.size()) - 1;
    while (left < right) {
        int sum = a[left] + a[right];  // @line sum
        rec.frame(ArrayScene(a).compare(left, right).markRange(0, left, Mark::Dim).markRange(right + 1, static_cast<int>(a.size()), Mark::Dim).pointer("left", left).pointer("right", right).var("sum", sum).var("target", target), cat(a[left], " + ", a[right], " = ", sum, sum == target ? ". That's the target!" : sum < target ? ". Too small: move left up to get a bigger number." : ". Too big: move right down to get a smaller number."), sum == target ? "found" : sum < target ? "move_left" : "move_right"); // @viz
        if (sum == target) return {left, right};  // @line found
        if (sum < target) ++left;  // need a bigger sum  // @line move_left
        else --right;              // need a smaller sum  // @line move_right
    }
    rec.frame(ArrayScene(a).markRange(0, static_cast<int>(a.size()), Mark::Dim), "The pointers met. No pair adds up to the target.", "none"); // @viz
    return {-1, -1};  // @line none
}
// [/snippet]

// [snippet:pattern.sliding_window]
int maxWindowSum(const std::vector<int>& a, int k, Recorder& rec) {
    int windowSum = 0;
    for (int i = 0; i < k; ++i) windowSum += a[i];  // the first window  // @line first_window
    int best = windowSum;
    rec.frame(ArrayScene(a).window(0, k).var("window sum", windowSum).var("best", best), cat("First window: the first ", k, " values add up to ", windowSum, "."), "first_window"); // @viz
    for (int right = k; right < static_cast<int>(a.size()); ++right) {
        windowSum += a[right] - a[right - k];  // add the new value, drop the old one  // @line slide
        rec.frame(ArrayScene(a).window(right - k + 1, right + 1).mark(right, Mark::Active).mark(right - k, Mark::Dim).var("window sum", windowSum).var("best", std::max(best, windowSum)), cat("Slide right: add ", a[right], ", drop ", a[right - k], ". Sum = ", windowSum, ". No need to re-add the whole window."), "slide"); // @viz
        best = std::max(best, windowSum);  // @line best
    }
    return best;
}
// [/snippet]

namespace {
GridScene prefixGrid(const std::vector<int>& a, const std::vector<int>& prefix, int filled) {
    const int n = static_cast<int>(a.size());
    GridScene g(2, n + 1);
    g.rowHeader(0, "a").rowHeader(1, "prefix");
    for (int i = 0; i < n; ++i) g.set(0, i, a[static_cast<std::size_t>(i)]).colHeader(i, std::to_string(i));
    g.colHeader(n, std::to_string(n));
    for (int i = 0; i <= filled && i <= n; ++i) g.set(1, i, prefix[static_cast<std::size_t>(i)]);
    return g;
}
}  // namespace

// [snippet:pattern.prefix_sums]
std::vector<int> prefixSums(const std::vector<int>& a, Recorder& rec) {
    std::vector<int> prefix(a.size() + 1, 0);  // prefix[i] = sum of the first i values
    for (std::size_t i = 0; i < a.size(); ++i) {
        prefix[i + 1] = prefix[i] + a[i];  // @line build
        rec.frame(prefixGrid(a, prefix, static_cast<int>(i) + 1).mark(0, static_cast<int>(i), Mark::Active).mark(1, static_cast<int>(i), Mark::Compare).mark(1, static_cast<int>(i) + 1, Mark::Found), cat("prefix[", i + 1, "] = prefix[", i, "] + a[", i, "] = ", prefix[i], " + ", a[i], " = ", prefix[i + 1]), "build"); // @viz
    }
    return prefix;
}

// Sum of a[l..r] = (sum of first r+1 values) - (sum of first l values).
int rangeSum(const std::vector<int>& prefix, int l, int r) {
    return prefix[r + 1] - prefix[l];  // O(1), no loop!  // @line query
}
// [/snippet]

void registerArrayPatterns(Registry& r) {
    r.add({"pattern.two_pointers", "Two pointers: pair sum", "Array patterns", "pattern.two_pointers",
           "O(n)", "O(1)",
           "In a sorted list, start at both ends and move the pointer that brings you closer to the target.",
           {{"array", "int_array", "Numbers (will be sorted)"}, {"target", "int", "Target sum"}},
           {{"array", {1, 2, 4, 7, 11, 15}}, {"target", 15}}},
          [](const Json& input, Recorder& rec) -> Json {
              auto a = intArray(input, "array");
              const int target = intValue(input, "target", -2 * kMaxAbsValue, 2 * kMaxAbsValue);
              if (!std::is_sorted(a.begin(), a.end())) {
                  std::sort(a.begin(), a.end());
                  rec.frame(ArrayScene(a), "Two pointers needs sorted data, so we sorted it first.", "");
              }
              auto [i, j] = pairWithSum(a, target, rec);  // structured binding
              return {{"left", i}, {"right", j}, {"array", a}};
          });

    r.add({"pattern.sliding_window", "Sliding window: best k in a row", "Array patterns",
           "pattern.sliding_window", "O(n)", "O(1)",
           "Keep a running total of a fixed-size window: add the value entering, subtract the one leaving.",
           {{"array", "int_array", "Numbers"}, {"k", "int", "Window size"}},
           {{"array", {2, 1, 5, 1, 3, 2, 9, 1}}, {"k", 3}}},
          [](const Json& input, Recorder& rec) -> Json {
              auto a = intArray(input, "array");
              if (a.empty()) throw std::invalid_argument("'array' needs at least one number");
              const int k = intValue(input, "k", 1, static_cast<int>(a.size()));
              return {{"best", maxWindowSum(a, k, rec)}};
          });

    r.add({"pattern.prefix_sums", "Prefix sums", "Array patterns", "pattern.prefix_sums",
           "O(n) to build, O(1) per query", "O(n)",
           "Store running totals once, then answer any range-sum question with one subtraction.",
           {{"array", "int_array", "Numbers"}, {"l", "int", "Range start"}, {"r", "int", "Range end"}},
           {{"array", {3, 1, 4, 1, 5, 9}}, {"l", 1}, {"r", 4}}},
          [](const Json& input, Recorder& rec) -> Json {
              auto a = intArray(input, "array");
              if (a.empty()) throw std::invalid_argument("'array' needs at least one number");
              const int n = static_cast<int>(a.size());
              const int l = intValue(input, "l", 0, n - 1);
              const int rr = intValue(input, "r", l, n - 1);
              auto prefix = prefixSums(a, rec);
              const int sum = rangeSum(prefix, l, rr);
              auto g = prefixGrid(a, prefix, n);
              for (int i = l; i <= rr; ++i) g.mark(0, i, Mark::Window);
              g.mark(1, rr + 1, Mark::Found).mark(1, l, Mark::Compare);
              rec.frame(g, cat("Sum of a[", l, "..", rr, "] = prefix[", rr + 1, "] - prefix[", l, "] = ",
                               prefix[static_cast<std::size_t>(rr) + 1], " - ",
                               prefix[static_cast<std::size_t>(l)], " = ", sum),
                        "query");
              return {{"prefix", prefix}, {"sum", sum}};
          });
}

}  // namespace forge::algo
