#include <doctest/doctest.h>

#include <algorithm>
#include <random>
#include <vector>

#include "forge/algo/registry.hpp"
#include "forge/algo/sorting.hpp"

using namespace forge;

namespace {
using SortFn = void (*)(std::vector<int>&, viz::Recorder&);

void checkAgainstStdSort(SortFn sortFn) {
    std::mt19937 rng(12345);  // fixed seed: tests must be repeatable
    std::uniform_int_distribution<int> len(0, 16), val(-50, 50);
    for (int trial = 0; trial < 80; ++trial) {
        std::vector<int> a(static_cast<std::size_t>(len(rng)));
        for (int& x : a) x = val(rng);
        auto expected = a;
        std::sort(expected.begin(), expected.end());
        viz::Recorder rec;
        sortFn(a, rec);
        REQUIRE(a == expected);
    }
}
}  // namespace

TEST_CASE("every sort agrees with std::sort on random input") {
    checkAgainstStdSort(algo::bubbleSort);
    checkAgainstStdSort(algo::selectionSort);
    checkAgainstStdSort(algo::insertionSort);
    checkAgainstStdSort(static_cast<SortFn>(algo::mergeSort));
    checkAgainstStdSort(static_cast<SortFn>(algo::quickSort));
    checkAgainstStdSort(algo::heapSort);
}

TEST_CASE("sort edge cases: empty, single, duplicates, already sorted, reversed") {
    const std::vector<std::vector<int>> cases = {
        {}, {7}, {2, 2, 2}, {1, 2, 3, 4}, {4, 3, 2, 1}, {-1, 0, -1, 0}};
    for (SortFn f : {static_cast<SortFn>(algo::bubbleSort), static_cast<SortFn>(algo::selectionSort),
                     static_cast<SortFn>(algo::insertionSort), static_cast<SortFn>(algo::mergeSort),
                     static_cast<SortFn>(algo::quickSort), static_cast<SortFn>(algo::heapSort)}) {
        for (auto a : cases) {
            auto expected = a;
            std::sort(expected.begin(), expected.end());
            viz::Recorder rec;
            f(a, rec);
            CHECK(a == expected);
        }
    }
}

TEST_CASE("sort traces end with a fully sorted frame") {
    const auto& reg = algo::Registry::builtin();
    for (const char* id : {"sort.bubble", "sort.selection", "sort.insertion", "sort.merge", "sort.quick",
                           "sort.heap"}) {
        CAPTURE(id);
        const auto* e = reg.find(id);
        REQUIRE(e != nullptr);
        viz::Recorder rec;
        const Json result = e->run(e->info.defaults, rec);
        REQUIRE(rec.frames().size() >= 3);
        const Json& last = rec.frames().back().scene;
        const auto values = last["values"].get<std::vector<int>>();
        CHECK(std::is_sorted(values.begin(), values.end()));
        CHECK(result["sorted"].get<std::vector<int>>() == values);
        // every cell in the final frame is marked sorted
        CHECK(last["marks"].size() == values.size());
    }
}

TEST_CASE("bubble sort stops early on sorted input") {
    std::vector<int> a = {1, 2, 3, 4, 5, 6};
    viz::Recorder rec;
    algo::bubbleSort(a, rec);
    // One pass of 5 comparisons + 1 end-of-pass frame, then early exit.
    CHECK(rec.frames().size() == 6);
}

TEST_CASE("traces are capped instead of growing forever") {
    // Reversed input is bubble sort's worst case: 60*59/2 = 1770 compares,
    // each recorded with a swap frame, which is more than kMaxFrames.
    std::vector<int> a(60);
    for (int i = 0; i < 60; ++i) a[static_cast<std::size_t>(i)] = 60 - i;
    viz::Recorder rec;
    algo::bubbleSort(a, rec);
    CHECK(std::is_sorted(a.begin(), a.end()));  // the algorithm still finishes
    CHECK(rec.frames().size() == viz::Recorder::kMaxFrames);
    CHECK(rec.truncated());
}
