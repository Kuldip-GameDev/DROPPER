#include <doctest/doctest.h>

#include <string>
#include <vector>

#include "forge/patterns/patterns.hpp"
#include "forge/patterns/strategy_singleton.hpp"

using namespace forge;
using namespace forge::patterns;

TEST_CASE("command: undo restores, redo replays, a new action clears redo") {
    Board b;
    History h;
    h.run(makeMove(1, 0), b);
    h.run(makeMove(0, 1), b);
    CHECK(b.x == 1);
    CHECK(b.y == 1);
    CHECK(h.undo(b));
    CHECK(b.y == 0);
    CHECK(h.redo(b));
    CHECK(b.y == 1);
    CHECK(h.undo(b));
    h.run(makeMove(-1, 0), b);
    CHECK_FALSE(h.redo(b));
    CHECK(h.undoNames() == std::vector<std::string>{"right", "left"});
}

TEST_CASE("command: moves are clamped to the board and undo exactly") {
    Board b;
    History h;
    h.run(makeMove(-1, 0), b);  // already at the left edge
    CHECK(b.x == 0);
    CHECK(h.undo(b));
    CHECK(b.x == 0);
    CHECK_FALSE(h.undo(b));
}

TEST_CASE("object pool: O(1) reuse, exhaustion, LIFO free list") {
    BulletPool<4> pool;
    const int a = pool.acquire(), b = pool.acquire(), c = pool.acquire(), d = pool.acquire();
    CHECK(a == 0);
    CHECK(d == 3);
    CHECK(pool.acquire() == -1);
    pool.release(b);
    pool.release(c);
    CHECK(pool.acquire() == c);  // last released, first reused
    CHECK(pool.acquire() == b);
    CHECK(pool.active(a));
}

TEST_CASE("fixed timestep: total updates match total time, steps are capped") {
    const auto frames = runFixedStep({10, 10, 10, 10, 10, 10}, 20, 5);  // 60 ms at 20 ms per step
    int total = 0;
    for (const auto& f : frames) total += f.updates;
    CHECK(total == 3);
    const auto slow = runFixedStep({1000}, 20, 5);
    CHECK(slow[0].updates == 5);
    CHECK(slow[0].accumulator < 20);  // backlog dropped, not carried forever
    for (const auto& f : frames) {
        CHECK(f.alpha >= 0);
        CHECK(f.alpha < 1);
    }
}

TEST_CASE("ring queue: FIFO order across the wrap-around, rejects when full") {
    RingQueue<int, 3> q;
    CHECK(q.push(1));
    CHECK(q.push(2));
    int out = 0;
    CHECK(q.pop(out));
    CHECK(out == 1);
    CHECK(q.push(3));
    CHECK(q.push(4));  // wraps
    CHECK_FALSE(q.push(5));
    std::vector<int> got;
    while (q.pop(out)) got.push_back(out);
    CHECK(got == std::vector<int>{2, 3, 4});
    CHECK_FALSE(q.pop(out));
}

TEST_CASE("state pattern: no double jump, ducking ignores jump") {
    Hero h;
    h.handle(Input::PressJump);
    h.handle(Input::PressJump);
    CHECK(std::string(h.stateName()) == "Jumping");
    CHECK(h.jumps == 1);
    h.handle(Input::PressDown);
    CHECK(std::string(h.stateName()) == "Diving");
    h.handle(Input::Land);
    h.handle(Input::PressDown);
    h.handle(Input::PressJump);
    CHECK(std::string(h.stateName()) == "Ducking");
    h.handle(Input::ReleaseDown);
    CHECK(std::string(h.stateName()) == "Standing");
}

TEST_CASE("flyweight structs: the light tree is tiny") {
    CHECK(sizeof(TreeLight) < 64);
    CHECK(sizeof(TreeHeavy) > sizeof(TreeModel));
}

TEST_CASE("strategy: same screen, different rules") {
    const auto& g = slot::gemWays();
    const auto s = slot::window(g, {0, 0, 3, 1, 0});
    CHECK(settle(WaysPay{}, g, s) == 125);
    CHECK(settle(CentreLinePay{}, g, s) >= 0);
}

TEST_CASE("dependency injection makes the button testable with a fake") {
    struct FakeAudio : Audio {
        std::vector<std::string> played;
        void play(const std::string& sound) override { played.push_back(sound); }
    } fake;
    SpinButton button(fake);
    button.press();
    CHECK(fake.played == std::vector<std::string>{"reel_spin"});
    AudioSingleton::instance().play("x");
    CHECK(&AudioSingleton::instance() == &AudioSingleton::instance());
}
