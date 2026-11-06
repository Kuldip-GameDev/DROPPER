#include <algorithm>
#include <cmath>

#include "forge/patterns/patterns.hpp"

namespace forge::patterns {

// [snippet:pattern.game_loop]
// Physics wants a steady tick (say every 20 ms) even when frames take
// 8 ms on one device and 50 ms on another. Bank the real time in an
// accumulator and spend it in fixed-size steps.
std::vector<LoopStats> runFixedStep(const std::vector<double>& frameTimes, double dt,
                                    int maxStepsPerFrame) {
    std::vector<LoopStats> frames;
    double accumulator = 0;
    for (double frameTime : frameTimes) {
        accumulator += frameTime;  // real time that passed  // @line bank
        int updates = 0;
        while (accumulator >= dt && updates < maxStepsPerFrame) {
            // update(dt);  <- the simulation always advances by exactly dt
            accumulator -= dt;  // @line step
            ++updates;
        }
        // Too far behind? Drop the backlog instead of falling further behind every frame.
        if (updates == maxStepsPerFrame) accumulator = std::fmod(accumulator, dt);  // @line clamp
        const double alpha = accumulator / dt;  // render(alpha) blends the last two states  // @line render
        frames.push_back({updates, accumulator, alpha});
    }
    return frames;
}
// [/snippet]

const char* toString(Input i) {
    switch (i) {
        case Input::PressJump: return "press JUMP";
        case Input::PressDown: return "press DOWN";
        case Input::ReleaseDown: return "release DOWN";
        case Input::Land: return "land";
    }
    return "?";
}

// [snippet:pattern.state]
// Each state is its own class. It only handles the inputs that make
// sense for it, and says which state comes next. No giant switch.
class Standing;
class Jumping;
class Ducking;
class Diving;

class Standing : public HeroState {
public:
    const char* name() const override { return "Standing"; }
    std::unique_ptr<HeroState> handle(Hero& hero, Input in) override;
};
class Jumping : public HeroState {
public:
    const char* name() const override { return "Jumping"; }
    std::unique_ptr<HeroState> handle(Hero& hero, Input in) override;
};
class Ducking : public HeroState {
public:
    const char* name() const override { return "Ducking"; }
    std::unique_ptr<HeroState> handle(Hero& hero, Input in) override;
};
class Diving : public HeroState {
public:
    const char* name() const override { return "Diving"; }
    std::unique_ptr<HeroState> handle(Hero& hero, Input in) override;
};

std::unique_ptr<HeroState> Standing::handle(Hero& hero, Input in) {
    if (in == Input::PressJump) { ++hero.jumps; return std::make_unique<Jumping>(); }  // @line stand_jump
    if (in == Input::PressDown) return std::make_unique<Ducking>();
    return nullptr;  // everything else: stay
}
std::unique_ptr<HeroState> Jumping::handle(Hero&, Input in) {
    if (in == Input::PressDown) return std::make_unique<Diving>();  // no double jump!  // @line jump_dive
    if (in == Input::Land) return std::make_unique<Standing>();
    return nullptr;
}
std::unique_ptr<HeroState> Ducking::handle(Hero&, Input in) {
    if (in == Input::ReleaseDown) return std::make_unique<Standing>();  // @line duck_stand
    return nullptr;
}
std::unique_ptr<HeroState> Diving::handle(Hero&, Input in) {
    if (in == Input::Land) return std::make_unique<Standing>();
    return nullptr;
}

void Hero::handle(Input input) {
    if (auto next = state_->handle(*this, input)) state_ = std::move(next);  // @line switch
}
// [/snippet]

Hero::Hero() : state_(std::make_unique<Standing>()) {}

}  // namespace forge::patterns
