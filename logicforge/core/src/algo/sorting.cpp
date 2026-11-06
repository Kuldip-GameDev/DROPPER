// Sorting algorithms, instrumented for the visualizer.
//
// How to read this file:
//   * Code between  // [snippet:id]  and  // [/snippet]  is shown in the app.
//   * Lines ending in  // @viz  only record pictures. They are removed from
//     the displayed code, so the learner sees the clean algorithm.
//   * // @line <label>  marks a line the visual can highlight.
#include "forge/algo/sorting.hpp"

#include <algorithm>
#include <utility>

#include "forge/algo/input.hpp"
#include "forge/algo/registry.hpp"
#include "forge/util/cat.hpp"
#include "forge/viz/scene.hpp"

namespace forge::algo {

using viz::ArrayScene;
using viz::Mark;
using viz::Recorder;

namespace {
// Bars are the most intuitive picture for sorting: taller = bigger.
ArrayScene bars(const std::vector<int>& a) {
    return ArrayScene(a).layout(ArrayScene::Layout::Bars);
}
int sz(const std::vector<int>& a) { return static_cast<int>(a.size()); }
}  // namespace

// ------------------------------------------------------------------ bubble
// [snippet:sort.bubble]
void bubbleSort(std::vector<int>& a, Recorder& rec) {
    const std::size_t n = a.size();
    for (std::size_t pass = 0; pass + 1 < n; ++pass) {  // @line pass
        bool swapped = false;
        for (std::size_t j = 0; j + 1 < n - pass; ++j) {
            rec.frame(bars(a).compare(j, j + 1).sortedFrom(n - pass), cat("Is ", a[j], " bigger than ", a[j + 1], "? ", a[j] > a[j + 1] ? "Yes, so swap them." : "No, leave them."), "compare"); // @viz
            if (a[j] > a[j + 1]) {  // @line compare
                std::swap(a[j], a[j + 1]);  // @line swap
                rec.frame(bars(a).swapped(j, j + 1).sortedFrom(n - pass), cat("Swapped. The bigger value ", a[j + 1], " bubbles one step right."), "swap"); // @viz
                swapped = true;
            }
        }
        rec.frame(bars(a).sortedFrom(n - pass - 1), cat("End of pass ", pass + 1, ": the biggest unsorted value is now in its final place."), "pass"); // @viz
        if (!swapped) break;  // nothing moved: already sorted  // @line early_exit
    }
}
// [/snippet]

// --------------------------------------------------------------- selection
// [snippet:sort.selection]
void selectionSort(std::vector<int>& a, Recorder& rec) {
    const std::size_t n = a.size();
    for (std::size_t i = 0; i + 1 < n; ++i) {
        std::size_t minIdx = i;  // best candidate so far  // @line pick
        rec.frame(bars(a).mark(i, Mark::Active).sortedUpTo(i).pointer("i", i).pointer("min", minIdx), cat("Find the smallest value from position ", i, " onwards. Start by guessing ", a[i], "."), "pick"); // @viz
        for (std::size_t j = i + 1; j < n; ++j) {
            rec.frame(bars(a).compare(j, minIdx).sortedUpTo(i).pointer("min", minIdx).pointer("j", j), cat("Is ", a[j], " smaller than ", a[minIdx], "?"), "compare"); // @viz
            if (a[j] < a[minIdx]) {  // @line compare
                minIdx = j;  // @line new_min
                rec.frame(bars(a).mark(minIdx, Mark::Pivot).sortedUpTo(i).pointer("min", minIdx), cat("Yes. New smallest: ", a[minIdx], "."), "new_min"); // @viz
            }
        }
        std::swap(a[i], a[minIdx]);  // put the smallest in place  // @line swap
        rec.frame(bars(a).swapped(i, minIdx).sortedUpTo(i + 1), cat("Swap it into position ", i, ". Everything left of here is sorted."), "swap"); // @viz
    }
}
// [/snippet]

// --------------------------------------------------------------- insertion
// [snippet:sort.insertion]
void insertionSort(std::vector<int>& a, Recorder& rec) {
    for (std::size_t i = 1; i < a.size(); ++i) {
        int key = a[i];  // the card we are inserting  // @line take
        std::size_t j = i;
        rec.frame(bars(a).mark(i, Mark::Active).pointer("i", i).var("key", key), cat("Pick up ", key, ". Slide it left until it fits, like sorting cards in your hand."), "take"); // @viz
        while (j > 0 && a[j - 1] > key) {  // @line compare
            a[j] = a[j - 1];  // shift the bigger one right  // @line shift
            --j;
            rec.frame(bars(a).mark(j + 1, Mark::Swap).pointer("hole", j).var("key", key), cat(a[j + 1], " is bigger than ", key, ", so it shifts right."), "shift"); // @viz
        }
        a[j] = key;  // drop the card into the gap  // @line insert
        rec.frame(bars(a).mark(j, Mark::Found).var("key", key), cat("Drop ", key, " into position ", j, ". The first ", i + 1, " values are sorted."), "insert"); // @viz
    }
}
// [/snippet]

// ------------------------------------------------------------------- merge
// [snippet:sort.merge]
// Sorts a[lo, hi)  ("hi" is one past the end, like end() in the STL).
void mergeSort(std::vector<int>& a, std::size_t lo, std::size_t hi, std::vector<int>& tmp,
               Recorder& rec) {
    if (hi - lo < 2) return;  // 0 or 1 items: already sorted  // @line base
    std::size_t mid = lo + (hi - lo) / 2;  // @line split
    rec.frame(bars(a).window(lo, hi).pointer("mid", mid), cat("Split [", lo, ", ", hi, ") into two halves at ", mid, "."), "split"); // @viz
    mergeSort(a, lo, mid, tmp, rec);  // sort the left half
    mergeSort(a, mid, hi, tmp, rec);  // sort the right half

    std::size_t i = lo, j = mid, k = lo;  // merge two sorted halves
    while (i < mid && j < hi) {
        rec.frame(bars(a).window(lo, hi).compare(i, j).pointer("i", i).pointer("j", j), cat("Take the smaller of ", a[i], " and ", a[j], "."), a[i] <= a[j] ? "take_left" : "take_right"); // @viz
        if (a[i] <= a[j]) tmp[k++] = a[i++];  // @line take_left
        else tmp[k++] = a[j++];  // @line take_right
    }
    while (i < mid) tmp[k++] = a[i++];  // copy leftovers
    while (j < hi) tmp[k++] = a[j++];
    std::copy(tmp.begin() + lo, tmp.begin() + hi, a.begin() + lo);  // @line copy_back
    rec.frame(bars(a).window(lo, hi).markRange(lo, hi, Mark::Ok), cat("Merged: positions ", lo, " to ", hi - 1, " are now in order."), "copy_back"); // @viz
}

void mergeSort(std::vector<int>& a, Recorder& rec) {
    std::vector<int> tmp(a.size());  // one buffer, reused by every merge
    mergeSort(a, 0, a.size(), tmp, rec);
}
// [/snippet]

// ------------------------------------------------------------------- quick
// [snippet:sort.quick]
// Lomuto partition: the last element is the pivot.
int partition(std::vector<int>& a, int lo, int hi, Recorder& rec) {
    int pivot = a[hi];  // @line pivot
    int store = lo;     // next slot for a value smaller than the pivot
    rec.frame(bars(a).window(lo, hi + 1).mark(hi, Mark::Pivot).pointer("store", store), cat("Pivot = ", pivot, ". Move everything smaller than it to the left."), "pivot"); // @viz
    for (int j = lo; j < hi; ++j) {
        rec.frame(bars(a).window(lo, hi + 1).mark(hi, Mark::Pivot).mark(j, Mark::Compare).pointer("store", store).pointer("j", j), cat("Is ", a[j], " smaller than the pivot ", pivot, "?"), "compare"); // @viz
        if (a[j] < pivot) {  // @line compare
            std::swap(a[store], a[j]);  // @line swap
            rec.frame(bars(a).window(lo, hi + 1).mark(hi, Mark::Pivot).swapped(store, j).pointer("store", store), cat("Yes: move ", a[store], " into the small side."), "swap"); // @viz
            ++store;
        }
    }
    std::swap(a[store], a[hi]);  // pivot lands in its final place  // @line place_pivot
    rec.frame(bars(a).window(lo, hi + 1).mark(store, Mark::Sorted), cat("Pivot ", pivot, " is now in its final position ", store, "."), "place_pivot"); // @viz
    return store;
}

void quickSort(std::vector<int>& a, int lo, int hi, Recorder& rec) {
    if (lo >= hi) return;  // @line base
    int p = partition(a, lo, hi, rec);
    quickSort(a, lo, p - 1, rec);  // left of the pivot  // @line recurse
    quickSort(a, p + 1, hi, rec);  // right of the pivot
}

void quickSort(std::vector<int>& a, Recorder& rec) {
    quickSort(a, 0, static_cast<int>(a.size()) - 1, rec);
}
// [/snippet]

// -------------------------------------------------------------------- heap
// [snippet:sort.heap]
// Push a[root] down until both children are smaller (max-heap rule).
void siftDown(std::vector<int>& a, std::size_t root, std::size_t size, Recorder& rec) {
    while (true) {
        std::size_t largest = root;
        std::size_t left = 2 * root + 1, right = 2 * root + 2;  // @line children
        if (left < size && a[left] > a[largest]) largest = left;
        if (right < size && a[right] > a[largest]) largest = right;
        rec.frame(bars(a).mark(root, Mark::Active).mark(largest, Mark::Compare).sortedFrom(size), cat("Node ", a[root], ": is a child bigger? ", largest == root ? "No, heap rule holds." : cat("Yes, ", a[largest], ".")), largest == root ? "heap_ok" : "swap"); // @viz
        if (largest == root) return;  // @line heap_ok
        std::swap(a[root], a[largest]);  // @line swap
        root = largest;
    }
}

void heapSort(std::vector<int>& a, Recorder& rec) {
    const std::size_t n = a.size();
    for (std::size_t i = n / 2; i-- > 0;) siftDown(a, i, n, rec);  // @line build_heap
    for (std::size_t end = n; end-- > 1;) {
        std::swap(a[0], a[end]);  // biggest goes to the back  // @line extract_max
        rec.frame(bars(a).swapped(0, end).sortedFrom(end), cat("The biggest value ", a[end], " moves to the back."), "extract_max"); // @viz
        siftDown(a, 0, end, rec);
    }
}
// [/snippet]

// ---------------------------------------------------------- registration

void registerSorting(Registry& r) {
    const std::vector<Field> fields = {{"array", "int_array", "Numbers"}};
    const Json defaults = {{"array", {5, 1, 4, 2, 8, 3}}};

    // Every sort shares the same wrapper. A lambda that *returns* a lambda:
    // `wrap(bubbleSort)` captures the function pointer and gives back an AlgoFn.
    auto wrap = [](void (*sortFn)(std::vector<int>&, Recorder&)) {
        return [sortFn](const Json& input, Recorder& rec) -> Json {
            auto a = intArray(input, "array");
            rec.frame(bars(a), "Here is the unsorted input.", "");
            sortFn(a, rec);
            rec.frame(bars(a).markRange(0, sz(a), Mark::Sorted), "Done! Every value is in order.", "");
            return {{"sorted", a}};
        };
    };

    r.add({"sort.bubble", "Bubble sort", "Sorting", "sort.bubble", "O(n²)", "O(1)",
           "Walk the list, swapping neighbours that are in the wrong order. Big values bubble to the end.",
           fields, defaults},
          wrap(bubbleSort));
    r.add({"sort.selection", "Selection sort", "Sorting", "sort.selection", "O(n²)", "O(1)",
           "Find the smallest remaining value and put it next in line.", fields, defaults},
          wrap(selectionSort));
    r.add({"sort.insertion", "Insertion sort", "Sorting", "sort.insertion", "O(n²), O(n) if nearly sorted",
           "O(1)", "Take one value at a time and slide it left into place, like sorting playing cards.",
           fields, defaults},
          wrap(insertionSort));
    r.add({"sort.merge", "Merge sort", "Sorting", "sort.merge", "O(n log n)", "O(n)",
           "Split in half, sort each half, then merge the two sorted halves.", fields, defaults},
          wrap(static_cast<void (*)(std::vector<int>&, Recorder&)>(mergeSort)));
    r.add({"sort.quick", "Quick sort", "Sorting", "sort.quick", "O(n log n) average, O(n²) worst",
           "O(log n)", "Pick a pivot, put smaller values left and bigger right, repeat on each side.",
           fields, defaults},
          wrap(static_cast<void (*)(std::vector<int>&, Recorder&)>(quickSort)));
    r.add({"sort.heap", "Heap sort", "Sorting", "sort.heap", "O(n log n)", "O(1)",
           "Arrange values as a max-heap, then repeatedly move the biggest to the back.", fields,
           defaults},
          wrap(heapSort));
}

}  // namespace forge::algo
