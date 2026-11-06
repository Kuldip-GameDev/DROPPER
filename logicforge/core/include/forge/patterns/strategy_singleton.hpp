#pragma once
#include <string>
#include <vector>

#include "forge/slot/game.hpp"

namespace forge::patterns {

// [snippet:pattern.strategy]
// Strategy: one question ("what did this spin win?"), interchangeable
// answers. The machine holds a PayRule&, not a hard-coded algorithm.
class PayRule {
public:
    virtual ~PayRule() = default;
    virtual const char* name() const = 0;
    virtual long long pay(const slot::ReelGame& g, const slot::Screen& s) const = 0;
};

class CentreLinePay : public PayRule {
public:
    const char* name() const override { return "centre payline"; }
    long long pay(const slot::ReelGame& g, const slot::Screen& s) const override {
        const std::vector<int> centre(g.reels.size(), 1);
        return slot::evaluateLine(g, s, centre).pay;
    }
};

class WaysPay : public PayRule {
public:
    const char* name() const override { return "243 ways"; }
    long long pay(const slot::ReelGame& g, const slot::Screen& s) const override {
        long long total = 0;
        for (const auto& w : slot::evaluateWays(g, s)) total += w.pay;
        return total;
    }
};

// Swapping the rule changes the game's maths without touching this code.
inline long long settle(const PayRule& rule, const slot::ReelGame& g, const slot::Screen& s) {
    return rule.pay(g, s);  // @line settle
}
// [/snippet]

// [snippet:pattern.singleton]
// The Singleton: one global instance reachable from anywhere.
class AudioSingleton {
public:
    static AudioSingleton& instance() {
        static AudioSingleton audio;  // created on first use
        return audio;
    }
    void play(const std::string& sound) { played.push_back(sound); }
    std::vector<std::string> played;

private:
    AudioSingleton() = default;
};
// Convenient... but any function may secretly depend on it, the order of
// global start-up gets fragile, and tests can't swap in a fake.

// The alternative: hand the dependency in (dependency injection).
class Audio {
public:
    virtual ~Audio() = default;
    virtual void play(const std::string& sound) = 0;
};

class SpinButton {
public:
    explicit SpinButton(Audio& audio) : audio_(audio) {}  // the dependency is visible  // @line inject
    void press() { audio_.play("reel_spin"); }

private:
    Audio& audio_;
};
// [/snippet]

}  // namespace forge::patterns
