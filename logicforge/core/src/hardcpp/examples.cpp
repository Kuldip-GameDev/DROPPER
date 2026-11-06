#include <functional>
#include <ranges>

#include "forge/hardcpp.hpp"

// These examples deliberately use precedence the compiler warns about.
// The warning is the lesson: always build with warnings on.
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic ignored "-Wparentheses"
#endif

namespace forge::hardcpp {

// [snippet:hard.declarations]
// Read a declaration from the NAME outwards: right first, then left.
int declarationsDemo() {
    int value = 7;
    int* p = &value;               // p: pointer to int
    int** pp = &p;                 // pp: pointer to (pointer to int)
    const int* readOnly = &value;  // pointer to const int: *readOnly = 1 won't compile
    int* const fixed = &value;     // const pointer to int: fixed = p won't compile
    int arr[3] = {1, 2, 3};
    int (*pArr)[3] = &arr;         // pointer to an ARRAY of 3 ints (the parentheses matter)
    int* ptrs[3] = {p, p, p};      // array of 3 POINTERS to int
    int (*fn)(int) = [](int x) { return x * 2; };  // pointer to function int -> int
    using Doubler = int (*)(int);  // the same type, readable
    Doubler same = fn;
    *fixed += 1;                   // value = 8
    return **pp + *readOnly + (*pArr)[2] + *ptrs[1] + same(1);  // 8 + 8 + 3 + 8 + 2 = 29
}
// [/snippet]

// [snippet:hard.precedence]
int precedenceDemo() {
    int a[] = {10, 20, 30};
    int* p = a;
    int x = *p++;    // *(p++): read a[0] = 10, THEN move p to a[1]
    int y = ++*p;    // ++(*p): a[1] becomes 21, y = 21
    int z = (*p)++;  // read 21 into z, then a[1] becomes 22
    bool shiftOk = 1 << 2 + 1 == 8;  // + before <<, << before ==: (1 << 3) == 8 is true
    int masked = 6 & 3 == 2;         // == before &: 6 & (3 == 2) = 6 & 0 = 0. A classic bug!
    return x + y + z + shiftOk + masked + a[1];  // 10 + 21 + 21 + 1 + 0 + 22 = 75
}
// [/snippet]

// [snippet:hard.evaluation_order]
// In f(g(), h()) C++ does NOT promise whether g or h runs first.
std::string evaluationOrderDemo() {
    std::string log;
    auto g = [&log] { log += "g"; return 1; };
    auto h = [&log] { log += "h"; return 2; };
    auto add = [](int a, int b) { return a + b; };
    add(g(), h());  // log is "gh" or "hg": both are correct C++
    return log;
}
// [/snippet]

// [snippet:hard.stl_pipeline]
// "The total of the three biggest wins over 100", as a pipeline.
long long topWins(std::vector<int> wins) {
    std::sort(wins.begin(), wins.end(), std::greater<>{});  // biggest first
    auto big = wins | std::views::filter([](int w) { return w > 100; }) | std::views::take(3);
    long long total = 0;
    for (int w : big) total += w;  // views are lazy: work happens here, element by element
    return total;
}
// [/snippet]

}  // namespace forge::hardcpp
