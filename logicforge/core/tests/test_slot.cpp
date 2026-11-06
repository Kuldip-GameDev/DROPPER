#include <doctest/doctest.h>

#include <algorithm>
#include <cmath>
#include <map>
#include <numeric>
#include <random>
#include <vector>

#include "forge/slot/game.hpp"
#include "forge/slot/machine.hpp"
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

TEST_CASE("Lucky 3 exact RTP equals the hand calculation") {
    // Each reel: SEVEN x1, BAR x2, BELL x3, CHERRY x5, LEMON x9 (20 stops).
    // Winning spins: 1 + 8 + 27 + 125 + 729 = 890 of 8000.
    // Total pay: 800*1 + 100*8 + 40*27 + 16*125 + 4*729 = 7596.
    // Sum of squares: 800^2*1 + 100^2*8 + 40^2*27 + 16^2*125 + 4^2*729 = 806864.
    const RtpReport r = exactRtpCentreLine(lucky3());
    CHECK(r.combos == 8000);
    CHECK(r.winningCombos == 890);
    CHECK(r.totalPay == 7596);
    CHECK(r.rtp == doctest::Approx(0.9495));
    CHECK(r.hitFrequency == doctest::Approx(0.11125));
    CHECK(r.stdDev == doctest::Approx(std::sqrt(806864.0 / 8000.0 - 0.9495 * 0.9495)));
}

TEST_CASE("window wraps around the circular strip") {
    const ReelGame& g = lucky3();
    const Screen s = window(g, {19, 0, 0});
    CHECK(s[0][0] == g.reels[0][19]);
    CHECK(s[1][0] == g.reels[0][0]);
    CHECK(s[2][0] == g.reels[0][1]);
}

TEST_CASE("payline evaluation with wilds") {
    const ReelGame& g = gemWays();  // 0 = WILD, 1 = DIAMOND, 2 = BELL ...
    Screen s(3, std::vector<int>(5, 4));
    s[1] = {0, 1, 0, 1, 2};  // W D W D BELL
    const LineResult line = evaluateLine(g, s, {1, 1, 1, 1, 1});
    CHECK(line.symbol == 1);
    CHECK(line.count == 4);
    CHECK(line.pay == 50);
    s[1] = {0, 0, 0, 0, 0};  // all wild: wild pays nothing in this game
    CHECK(evaluateLine(g, s, {1, 1, 1, 1, 1}).pay == 0);
}

TEST_CASE("ways evaluation equals brute force over all 243 paths") {
    const ReelGame& g = gemWays();
    std::mt19937 gen(11);
    for (int trial = 0; trial < 200; ++trial) {
        std::vector<int> stops(5);
        for (int& x : stops) x = static_cast<int>(gen() % 15);
        const Screen s = window(g, stops);
        // Brute force over all 243 paths. Ways rule: a symbol pays ONCE, for its longest
        // run m from the left; the number of ways is how many distinct length-m path
        // prefixes match. (Shorter runs on paths that branch off do not pay extra.)
        long long expectedPay = 0;
        for (int sym = 1; sym < 5; ++sym) {
            int lens[243];
            int m = 0;
            for (int path = 0; path < 243; ++path) {
                int rows[5], p = path;
                for (int k = 0; k < 5; ++k) { rows[k] = p % 3; p /= 3; }
                int len = 0;
                while (len < 5 && (s[rows[len]][len] == sym || s[rows[len]][len] == g.wild)) ++len;
                lens[path] = len;
                m = std::max(m, len);
            }
            long long pathsAtM = 0;
            for (int path = 0; path < 243; ++path) pathsAtM += lens[path] == m ? 1 : 0;
            long long free = 1;  // reels after the run can be anything: 3 choices each
            for (int k = m; k < 5; ++k) free *= 3;
            expectedPay += g.paytable[static_cast<std::size_t>(sym)][static_cast<std::size_t>(m)] * (pathsAtM / free);
        }
        long long gotPay = 0;
        for (const auto& w : evaluateWays(g, s)) gotPay += w.pay;
        CHECK(gotPay == expectedPay);
    }
}

TEST_CASE("cluster finder and gravity") {
    Grid g = {{0, 0, 1}, {0, 2, 1}, {0, 0, 1}};
    const auto clusters = findClusters(g, 3);
    REQUIRE(clusters.size() == 2);  // five 0s, three 1s
    std::vector<std::size_t> sizes = {clusters[0].size(), clusters[1].size()};
    std::sort(sizes.begin(), sizes.end());
    CHECK(sizes == std::vector<std::size_t>{3, 5});

    Grid h = {{1, -1}, {-1, 2}, {3, -1}};
    std::vector<int> before;
    for (auto& row : h) for (int x : row) if (x >= 0) before.push_back(x);
    const int empty = applyGravity(h);
    CHECK(empty == 3);
    CHECK(h == Grid{{-1, -1}, {1, -1}, {3, 2}});
    std::vector<int> after;
    for (auto& row : h) for (int x : row) if (x >= 0) after.push_back(x);
    std::sort(before.begin(), before.end());
    std::sort(after.begin(), after.end());
    CHECK(before == after);  // gravity moves symbols, never creates or destroys them
}

TEST_CASE("state machine: illegal events are rejected") {
    SlotMachine m(lucky3(), 10);
    std::vector<std::string> events;
    m.subscribe([&](const std::string& e) { events.push_back(e); });
    CHECK_FALSE(m.pressSpin(11));
    CHECK_FALSE(m.reelsStopped({0, 0, 0}));
    CHECK(m.pressSpin(1));
    CHECK_FALSE(m.pressSpin(1));
    CHECK(m.reelsStopped({2, 3, 9}));
    CHECK((m.state() == State::WinShow));
    CHECK(m.lastWin() == 40);
    CHECK(m.freeSpins() == 3);
}
