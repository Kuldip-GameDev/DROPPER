#include "forge/algo/searching.hpp"

#include <algorithm>

#include "forge/algo/input.hpp"
#include "forge/algo/registry.hpp"
#include "forge/util/cat.hpp"
#include "forge/viz/scene.hpp"

namespace forge::algo {

using viz::ArrayScene;
using viz::Mark;
using viz::Recorder;

// [snippet:search.linear]
int linearSearch(const std::vector<int>& a, int target, Recorder& rec) {
    for (std::size_t i = 0; i < a.size(); ++i) {
        rec.frame(ArrayScene(a).mark(i, a[i] == target ? Mark::Found : Mark::Active).markRange(0, i, Mark::Dim).pointer("i", i), cat("Is a[", i, "] = ", a[i], " equal to ", target, "? ", a[i] == target ? "Yes!" : "No, keep walking."), "check"); // @viz
        if (a[i] == target) return static_cast<int>(i);  // @line check
    }
    rec.frame(ArrayScene(a).markRange(0, static_cast<int>(a.size()), Mark::Dim), cat("Checked every value. ", target, " is not here."), "not_found"); // @viz
    return -1;  // @line not_found
}
// [/snippet]

// [snippet:search.binary]
int binarySearch(const std::vector<int>& a, int target, Recorder& rec) {
    int lo = 0;
    int hi = static_cast<int>(a.size()) - 1;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;  // same as (lo+hi)/2 but cannot overflow  // @line mid
        rec.frame(ArrayScene(a).markRange(0, lo, Mark::Dim).markRange(hi + 1, static_cast<int>(a.size()), Mark::Dim).window(lo, hi + 1).mark(mid, Mark::Active).pointer("lo", lo).pointer("mid", mid).pointer("hi", hi), cat("Look in the middle: a[", mid, "] = ", a[mid], "."), "mid"); // @viz
        if (a[mid] == target) {  // @line found
            rec.frame(ArrayScene(a).mark(mid, Mark::Found).pointer("mid", mid), cat("Found ", target, " at index ", mid, "."), "found"); // @viz
            return mid;
        }
        if (a[mid] < target) {
            lo = mid + 1;  // target is bigger: throw away the left half  // @line go_right
            rec.frame(ArrayScene(a).markRange(0, lo, Mark::Dim).pointer("lo", lo).pointer("hi", hi), cat(a[mid], " < ", target, ", so the answer can only be on the right."), "go_right"); // @viz
        } else {
            hi = mid - 1;  // target is smaller: throw away the right half  // @line go_left
            rec.frame(ArrayScene(a).markRange(hi + 1, static_cast<int>(a.size()), Mark::Dim).pointer("lo", lo).pointer("hi", hi), cat(a[mid], " > ", target, ", so the answer can only be on the left."), "go_left"); // @viz
        }
    }
    rec.frame(ArrayScene(a).markRange(0, static_cast<int>(a.size()), Mark::Dim), cat("lo passed hi: nothing left to search. ", target, " is not here."), "not_found"); // @viz
    return -1;  // @line not_found
}
// [/snippet]

void registerSearching(Registry& r) {
    const std::vector<Field> fields = {{"array", "int_array", "Numbers"},
                                       {"target", "int", "Find this number"}};

    r.add({"search.linear", "Linear search", "Searching", "search.linear", "O(n)", "O(1)",
           "Check every value from left to right until you find it.", fields,
           {{"array", {7, 3, 9, 1, 5}}, {"target", 1}}},
          [](const Json& input, Recorder& rec) -> Json {
              auto a = intArray(input, "array");
              const int target = intValue(input, "target", -kMaxAbsValue, kMaxAbsValue);
              return {{"index", linearSearch(a, target, rec)}};
          });

    r.add({"search.binary", "Binary search", "Searching", "search.binary", "O(log n)", "O(1)",
           "In a sorted list, look at the middle and throw away the half that can't contain the answer.",
           fields, {{"array", {1, 3, 4, 6, 8, 9, 11, 14}}, {"target", 9}}},
          [](const Json& input, Recorder& rec) -> Json {
              auto a = intArray(input, "array");
              const int target = intValue(input, "target", -kMaxAbsValue, kMaxAbsValue);
              if (!std::is_sorted(a.begin(), a.end())) {
                  std::sort(a.begin(), a.end());
                  rec.frame(ArrayScene(a), "Binary search only works on sorted data, so we sorted it first.", "");
              }
              return {{"index", binarySearch(a, target, rec)}, {"array", a}};
          });
}

}  // namespace forge::algo
