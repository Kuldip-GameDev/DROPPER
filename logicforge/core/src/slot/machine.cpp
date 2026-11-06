#include "forge/slot/machine.hpp"

#include "forge/util/cat.hpp"

namespace forge::slot {

const char* toString(State s) {
    switch (s) {
        case State::Idle: return "IDLE";
        case State::Spinning: return "SPINNING";
        case State::Evaluating: return "EVALUATING";
        case State::WinShow: return "WIN_SHOW";
        case State::FreeSpins: return "FREE_SPINS";
    }
    return "?";
}

void SlotMachine::emit(const std::string& event) {
    for (auto& listener : listeners_) listener(event);
}

// [snippet:slot.state_machine]
// Every event checks "is this allowed RIGHT NOW?" before changing state.
// That one rule prevents a whole class of bugs: double spins, paying twice,
// spinning with no credits...
bool SlotMachine::pressSpin(int bet) {
    if (state_ == State::Idle) {
        if (bet <= 0 || bet > credits_) return false;  // can't afford it  // @line check_bet
        credits_ -= bet;
        bet_ = bet;
        inFreeSpin_ = false;
    } else if (state_ == State::FreeSpins) {
        --freeSpins_;  // free: no credits taken  // @line free_spin
        inFreeSpin_ = true;
    } else {
        return false;  // ignored: reels already moving, or a win is showing  // @line ignore
    }
    state_ = State::Spinning;  // @line to_spinning
    emit("spin");
    return true;
}

bool SlotMachine::reelsStopped(const std::vector<int>& stops) {
    if (state_ != State::Spinning) return false;
    state_ = State::Evaluating;  // @line to_evaluating
    screen_ = window(game_, stops);
    const LineResult win = evaluateLine(game_, screen_, {1, 1, 1});
    lastWin_ = win.pay * bet_;
    credits_ += lastWin_;
    if (win.count == 3 && game_.symbols[win.symbol].id == "BELL") {
        freeSpins_ += 3;  // bonus trigger: three bells  // @line bonus
        emit("bonus +3 free spins");
    }
    if (lastWin_ > 0) {
        state_ = State::WinShow;  // @line to_win
        emit(cat("win ", lastWin_));
    } else {
        finishRound();
    }
    return true;
}

bool SlotMachine::winShown() {
    if (state_ != State::WinShow) return false;
    finishRound();
    return true;
}

void SlotMachine::finishRound() {
    state_ = freeSpins_ > 0 ? State::FreeSpins : State::Idle;  // @line finish
}
// [/snippet]

}  // namespace forge::slot
