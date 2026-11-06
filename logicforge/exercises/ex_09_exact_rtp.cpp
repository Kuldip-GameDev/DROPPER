// Exercise 09: exact RTP by counting (no simulation, no randomness).
//
// A 3-reel game with one payline. Every stop is equally likely.
// counts[s][k] = how many stops on reel k show symbol s.
// pay[s]       = what three of symbol s pays for a 1-credit bet.
// Only three-of-a-kind pays.
//
// RTP = (sum over symbols of winning combinations * pay) / (total combinations)
//
// For symbol s, the winning combinations are counts[s][0] * counts[s][1] * counts[s][2].
// Total combinations = (stops on reel 1) * (stops on reel 2) * (stops on reel 3).
//
// Use 64-bit integers for the counts; only divide at the very end.
// Then check your answer against the Slot Math Lab's "RTP, exactly" visual.
#include <doctest/doctest.h>

#include <array>
#include <cstdint>
#include <vector>

double exactRtp(const std::vector<std::array<int, 3>>& counts, const std::vector<int>& pay) {
    // TODO
    (void)counts;
    (void)pay;
    return -1.0;
}

double hitFrequency(const std::vector<std::array<int, 3>>& counts, const std::vector<int>& pay) {
    // TODO: combinations that PAY something / total combinations
    (void)counts;
    (void)pay;
    return -1.0;
}

TEST_CASE("a tiny game you can do on paper") {
    // Reels of 10 stops: one 7 per reel, the rest blanks. 777 pays 500.
    const std::vector<std::array<int, 3>> counts = {{1, 1, 1}, {9, 9, 9}};
    const std::vector<int> pay = {500, 0};
    CHECK(exactRtp(counts, pay) == doctest::Approx(0.5));  // 500 / 1000
    CHECK(hitFrequency(counts, pay) == doctest::Approx(0.001));  // three blanks pay nothing
}

TEST_CASE("Lucky 3 from the Slot Math Lab") {
    // SEVEN, BAR, BELL, CHERRY, LEMON on every reel: 1, 2, 3, 5, 9 stops (20 total).
    const std::vector<std::array<int, 3>> counts = {{1, 1, 1}, {2, 2, 2}, {3, 3, 3}, {5, 5, 5}, {9, 9, 9}};
    const std::vector<int> pay = {800, 100, 40, 16, 4};
    CHECK(exactRtp(counts, pay) == doctest::Approx(0.9495));
    CHECK(hitFrequency(counts, pay) == doctest::Approx(0.11125));
}
