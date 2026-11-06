// Exercise 02: reverse a vector in place with two pointers.
//
// Put `left` at the first index and `right` at the last.
// Swap, then walk them towards each other until they meet.
//
// Hint: be careful with an EMPTY vector. a.size() - 1 is a huge number when
// size() is 0, because size_t cannot be negative. Use int indices, or
// check for empty first.
#include <doctest/doctest.h>

#include <vector>

void reverseInPlace(std::vector<int>& a) {
    // TODO
    (void)a;
}

TEST_CASE("reverse odd and even lengths") {
    std::vector<int> odd = {1, 2, 3, 4, 5};
    reverseInPlace(odd);
    CHECK(odd == std::vector<int>{5, 4, 3, 2, 1});

    std::vector<int> even = {1, 2, 3, 4};
    reverseInPlace(even);
    CHECK(even == std::vector<int>{4, 3, 2, 1});
}

TEST_CASE("reverse tiny vectors") {
    std::vector<int> empty;
    reverseInPlace(empty);
    CHECK(empty.empty());

    std::vector<int> one = {42};
    reverseInPlace(one);
    CHECK(one == std::vector<int>{42});
}
