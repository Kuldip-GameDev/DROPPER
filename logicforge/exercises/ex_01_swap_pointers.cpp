// Exercise 01: swap two ints through pointers.
//
// A pointer holds an ADDRESS. `*p` means "the int living at that address".
//
//   int x = 1, y = 2;
//   swapValues(&x, &y);   // &x = "address of x"
//   // now x == 2, y == 1
//
// Hint: you need one temporary int. Draw two boxes and two arrows on paper first.
// Bonus: what should happen if a == b (same address)? Make sure it still works.
#include <doctest/doctest.h>

void swapValues(int* a, int* b) {
    // TODO: swap the ints that a and b point to.
    (void)a;
    (void)b;
}

TEST_CASE("swap two different ints") {
    int x = 1, y = 2;
    swapValues(&x, &y);
    CHECK(x == 2);
    CHECK(y == 1);
}

TEST_CASE("swap with itself leaves the value alone") {
    int x = 7;
    swapValues(&x, &x);
    CHECK(x == 7);
}

TEST_CASE("swap elements inside an array") {
    int a[] = {10, 20, 30};
    swapValues(&a[0], a + 2);  // a + 2 is pointer arithmetic: the address of a[2]
    CHECK(a[0] == 30);
    CHECK(a[2] == 10);
}
