#include <doctest/doctest.h>

#include <bit>
#include <random>

#include "forge/hardcpp.hpp"

using namespace forge::hardcpp;

TEST_CASE("declarations and precedence examples compute what the comments say") {
    CHECK(declarationsDemo() == 29);
    CHECK(precedenceDemo() == 75);
    const auto order = evaluationOrderDemo();
    CHECK((order == "gh" || order == "hg"));
}

TEST_CASE("bit tricks agree with <bit>") {
    static_assert(isPowerOfTwo(64) && !isPowerOfTwo(0) && !isPowerOfTwo(12));
    static_assert(lowestSetBit(12) == 4);
    std::mt19937 gen(5);
    for (int i = 0; i < 1000; ++i) {
        const unsigned x = static_cast<unsigned>(gen());
        CHECK(countBits(x) == std::popcount(x));
        CHECK(isPowerOfTwo(x) == std::has_single_bit(x));
        if (x != 0) CHECK(lowestSetBit(x) == (1u << std::countr_zero(x)));
    }
    unsigned f = 0;
    f = setFlag(f, 3);
    CHECK(hasFlag(f, 3));
    CHECK_FALSE(hasFlag(f, 2));
    CHECK_FALSE(hasFlag(clearFlag(f, 3), 3));
}

TEST_CASE("templates, concepts and CRTP") {
    CHECK(maxOf(3, 9) == 9);
    CHECK(maxOf(std::string("a"), std::string("b")) == "b");
    CHECK(sum(1, 2, 3, 4) == 10);
    CHECK(sum() == 0);
    struct Player {
        int s;
        int score() const { return s; }
    };
    CHECK(best(std::vector<Player>{{5}, {12}, {7}}) == 12);
    CHECK(Square(3).area() == doctest::Approx(9.0));
}

TEST_CASE("STL pipeline") {
    CHECK(topWins({50, 400, 120, 90, 800, 101, 30}) == 800 + 400 + 120);
    CHECK(topWins({1, 2, 3}) == 0);
}
