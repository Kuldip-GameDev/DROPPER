// Exercise 07: lambdas.
//
// 1. makeCounter(start): return a callable that returns start+1, start+2, ...
//    each time it is called. Capture `start` BY VALUE and mark the lambda
//    `mutable` so it can change its own copy.
// 2. countAtLeast(values, threshold): use std::count_if with a lambda that
//    captures threshold.
// 3. sortLeaderboard(players): highest score first; equal scores sorted by
//    name A-Z. Use std::sort with a lambda comparator that returns true when
//    a should come BEFORE b.
//
// Hint: watch "Capture by value vs by reference" and "mutable lambdas" first.
// Bonus: why would capturing start by reference ([&start]) be a bug here?
#include <doctest/doctest.h>

#include <algorithm>
#include <functional>
#include <string>
#include <vector>

std::function<int()> makeCounter(int start) {
    // TODO
    (void)start;
    return [] { return 0; };
}

long countAtLeast(const std::vector<int>& values, int threshold) {
    // TODO
    (void)values;
    (void)threshold;
    return -1;
}

struct Player {
    std::string name;
    int score = 0;
};

void sortLeaderboard(std::vector<Player>& players) {
    // TODO
    (void)players;
}

TEST_CASE("counters count independently") {
    auto a = makeCounter(0);
    auto b = makeCounter(10);
    CHECK(a() == 1);
    CHECK(a() == 2);
    CHECK(b() == 11);
    CHECK(a() == 3);
}

TEST_CASE("countAtLeast") {
    CHECK(countAtLeast({5, 120, 80, 200}, 100) == 2);
    CHECK(countAtLeast({}, 1) == 0);
}

TEST_CASE("leaderboard order") {
    std::vector<Player> p = {{"Cy", 50}, {"Ava", 90}, {"Ben", 50}, {"Dee", 70}};
    sortLeaderboard(p);
    std::vector<std::string> names;
    for (const auto& x : p) names.push_back(x.name);
    CHECK(names == std::vector<std::string>{"Ava", "Dee", "Ben", "Cy"});
}
