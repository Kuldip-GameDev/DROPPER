#pragma once
// Random number generators for the Slot Math Lab.
//
// Real slot games use certified RNGs and careful seeding. These classes
// exist to TEACH: they are small enough to read in one sitting, and each
// shows a real property (good or bad) you can see in the visualizer.
#include <concepts>
#include <cstdint>
#include <vector>

namespace forge::slot {

// Anything with `std::uint32_t next()` is a 32-bit generator (a C++20 concept).
template <typename G>
concept Rng32 = requires(G g) {
    { g.next() } -> std::same_as<std::uint32_t>;
};

// [snippet:slot.rng_lcg]
// Linear congruential generator: x = a*x + c (mod 2^32).
// Fast and tiny, but its LOW bits are weak: the lowest bit just alternates.
class Lcg32 {
public:
    explicit Lcg32(std::uint32_t seed) : state_(seed) {}
    std::uint32_t next() {
        state_ = 1664525u * state_ + 1013904223u;  // unsigned overflow wraps = mod 2^32
        return state_;
    }

private:
    std::uint32_t state_;
};
// [/snippet]

// [snippet:slot.rng_xorshift]
// Marsaglia's xorshift32: three shift-and-xor steps. Never outputs 0
// (so the seed must not be 0). Period 2^32 - 1.
class XorShift32 {
public:
    explicit XorShift32(std::uint32_t seed) : state_(seed == 0 ? 0x9E3779B9u : seed) {}
    std::uint32_t next() {
        std::uint32_t x = state_;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        return state_ = x;
    }

private:
    std::uint32_t state_;
};
// [/snippet]

// [snippet:slot.rng_pcg]
// PCG32 (O'Neill, pcg-random.org): an LCG on 64 bits, then a permutation
// that scrambles the output. Small, fast and statistically strong.
class Pcg32 {
public:
    Pcg32(std::uint64_t seed, std::uint64_t sequence) : inc_((sequence << 1u) | 1u) {
        next();
        state_ += seed;
        next();
    }
    std::uint32_t next() {
        const std::uint64_t old = state_;
        state_ = old * 6364136223846793005ULL + inc_;
        const auto xorshifted = static_cast<std::uint32_t>(((old >> 18u) ^ old) >> 27u);
        const auto rot = static_cast<std::uint32_t>(old >> 59u);
        return (xorshifted >> rot) | (xorshifted << ((32u - rot) & 31u));
    }

private:
    std::uint64_t state_ = 0;
    std::uint64_t inc_;
};
// [/snippet]

// [snippet:slot.uniform_below]
// A fair number in [0, n). `rng() % n` is BIASED when 2^32 is not a
// multiple of n: some results get one extra chance. Fix: throw away the
// few raw values at the top that would cause the imbalance.
template <Rng32 G>
std::uint32_t uniformBelow(G& rng, std::uint32_t n) {
    const std::uint32_t limit = static_cast<std::uint32_t>(-n) % n;  // = 2^32 mod n  // @line limit
    while (true) {
        const std::uint32_t r = rng.next();
        if (r >= limit) return r % n;  // the rest is an exact multiple of n  // @line accept
    }
}
// [/snippet]

// Pearson's chi-square statistic: sum of (observed - expected)^2 / expected.
// Small = consistent with "fair"; huge = something is off.
double chiSquare(const std::vector<std::uint64_t>& observed, double expectedEach);

}  // namespace forge::slot
