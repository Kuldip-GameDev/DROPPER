#pragma once
// A slot machine's game flow as a finite-state machine.
#include <functional>
#include <string>
#include <vector>

#include "forge/slot/game.hpp"

namespace forge::slot {

enum class State { Idle, Spinning, Evaluating, WinShow, FreeSpins };
const char* toString(State s);

class SlotMachine {
public:
    explicit SlotMachine(const ReelGame& game, int credits) : game_(game), credits_(credits) {}

    // Observer: anyone can subscribe to game events ("win 16", "bonus +3"...).
    void subscribe(std::function<void(const std::string&)> listener) { listeners_.push_back(std::move(listener)); }

    // Events that drive the machine. Each returns false if the event is not
    // allowed in the current state (e.g. pressing Spin while reels spin).
    bool pressSpin(int bet);
    bool reelsStopped(const std::vector<int>& stops);
    bool winShown();

    State state() const { return state_; }
    int credits() const { return credits_; }
    int freeSpins() const { return freeSpins_; }
    int lastWin() const { return lastWin_; }
    const Screen& screen() const { return screen_; }

private:
    void emit(const std::string& event);
    void finishRound();

    const ReelGame& game_;
    std::vector<std::function<void(const std::string&)>> listeners_;
    State state_ = State::Idle;
    int credits_;
    int bet_ = 0;
    int freeSpins_ = 0;
    int lastWin_ = 0;
    bool inFreeSpin_ = false;
    Screen screen_;
};

}  // namespace forge::slot
