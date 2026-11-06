#include <doctest/doctest.h>

#include <algorithm>
#include <cmath>
#include <map>
#include <numeric>
#include <random>
#include <vector>

#include "forge/slot/rng.hpp"
#include "forge/slot/weighted.hpp"

using namespace forge;
using namespace forge::slot;

TEST_CASE("PCG32 matches the reference implementation (pcg-random.org demo, seed 42, sequence 54)") {
    Pcg32 rng(42, 54);
    const std::uint32_t expected[] = {0xa15c02b7, 0x7b47f409, 0xba1d3330, 0x83d2f293, 0xbfa4784b, 0xcbed606e};
    for (std::uint32_t e : expected) CHECK(rng.next() == e);
}

TEST_CASE("xorshift32 first output for seed 1 matches a hand calculation") {
    // x = 1; x ^= x << 13 -> 8193; x ^= x >> 17 -> 8193; x ^= x << 5 -> 270369
    XorShift32 rng(1);
    CHECK(rng.next() == 270369u);
}

TEST_CASE("the LCG's lowest bit alternates (its famous weakness)") {
    Lcg32 rng(12345);
    std::uint32_t prev = rng.next() & 1u;
    for (int i = 0; i < 64; ++i) {
        const std::uint32_t bit = rng.next() & 1u;
        CHECK(bit != prev);
        prev = bit;
    }
}

TEST_CASE("uniformBelow: in range, rejection limit is 2^32 mod n, roughly uniform") {
    Pcg32 rng(7, 1);
    for (std::uint32_t n : {1u, 2u, 3u, 10u, 1000u, 4000000000u}) {
        CHECK(static_cast<std::uint32_t>(-n) % n == static_cast<std::uint32_t>((std::uint64_t{1} << 32) % n));
        for (int i = 0; i < 1000; ++i) CHECK(uniformBelow(rng, n) < n);
    }
    std::vector<std::uint64_t> buckets(10, 0);
    for (int i = 0; i < 100000; ++i) ++buckets[uniformBelow(rng, 10)];
    CHECK(chiSquare(buckets, 10000.0) < 30.0);  // 99.9th percentile for 9 dof is ~27.9
}

TEST_CASE("linear, binary and alias picks agree with the weights") {
    std::mt19937 gen(3);
    for (int trial = 0; trial < 50; ++trial) {
        std::vector<std::uint32_t> w(1 + gen() % 8);
        for (auto& x : w) x = 1 + gen() % 50;
        const auto prefix = runningTotals(w);
        const std::uint64_t total = prefix.back();
        std::vector<std::uint64_t> countLinear(w.size(), 0);
        for (std::uint64_t r = 0; r < total; ++r) {
            const auto a = pickLinear(w, r);
            CHECK(a == pickBinary(prefix, r));
            ++countLinear[a];
        }
        for (std::size_t i = 0; i < w.size(); ++i) CHECK(countLinear[i] == w[i]);  // each symbol owns exactly w[i] values
        const AliasTable table(w);
        const auto implied = table.impliedProbabilities();
        for (std::size_t i = 0; i < w.size(); ++i) {
            CHECK(implied[i] == doctest::Approx(static_cast<double>(w[i]) / static_cast<double>(total)).epsilon(1e-12));
        }
    }
    CHECK_THROWS(pickLinear({1, 2}, 3));
    CHECK_THROWS(pickBinary(runningTotals({1, 2}), 3));
}
