#pragma once
// Examples for the "Hard-to-read C++" track. Each function returns a value
// the tests check, so every claim in the lessons is verified by a compiler.
#include <algorithm>
#include <concepts>
#include <string>
#include <vector>

namespace forge::hardcpp {

int declarationsDemo();
int precedenceDemo();
std::string evaluationOrderDemo();

// [snippet:hard.bit_tricks]
// x - 1 flips the lowest set bit and every bit below it, so
// x & (x - 1) clears the lowest set bit. Many tricks follow from that.
constexpr bool isPowerOfTwo(unsigned x) { return x != 0 && (x & (x - 1)) == 0; }
constexpr unsigned lowestSetBit(unsigned x) { return x & (~x + 1); }  // same as x & -x
constexpr int countBits(unsigned x) {  // Kernighan: one loop per set bit
    int n = 0;
    while (x != 0) {
        x &= x - 1;
        ++n;
    }
    return n;
}
// Flags: one bit per property (wild, scatter, sticky...). Compact and fast.
constexpr unsigned setFlag(unsigned flags, unsigned bit) { return flags | (1u << bit); }
constexpr unsigned clearFlag(unsigned flags, unsigned bit) { return flags & ~(1u << bit); }
constexpr bool hasFlag(unsigned flags, unsigned bit) { return ((flags >> bit) & 1u) != 0; }
// [/snippet]

// [snippet:hard.templates]
// One function for every type that supports <.
template <typename T>
T maxOf(T a, T b) { return a < b ? b : a; }

// Variadic template + fold expression: sum(1, 2, 3) expands to (1 + (2 + (3 + 0))).
template <typename... Ts>
auto sum(Ts... xs) { return (xs + ... + 0); }

// A concept names a requirement, and errors say WHICH requirement failed.
template <typename T>
concept Scorable = requires(const T& t) {
    { t.score() } -> std::convertible_to<int>;
};

template <Scorable T>
int best(const std::vector<T>& items) {
    int top = 0;
    for (const auto& item : items) top = std::max(top, item.score());
    return top;
}

// CRTP: the base knows the derived type at compile time, so area() needs no virtual call.
template <typename Derived>
struct Shape {
    double area() const { return static_cast<const Derived*>(this)->areaImpl(); }
};
struct Square : Shape<Square> {
    double side;
    explicit Square(double s) : side(s) {}
    double areaImpl() const { return side * side; }
};
// [/snippet]

long long topWins(std::vector<int> wins);

}  // namespace forge::hardcpp
