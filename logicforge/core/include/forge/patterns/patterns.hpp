#pragma once
// Design patterns for games: small, compiled, tested examples.
#include <array>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace forge::patterns {

// ------------------------------------------------------------ Command
struct Board {
    int width = 5, height = 5;
    int x = 0, y = 0;  // the player
};

class Command {
public:
    virtual ~Command() = default;
    virtual void execute(Board& b) = 0;
    virtual void undo(Board& b) = 0;
    virtual std::string name() const = 0;
};

std::unique_ptr<Command> makeMove(int dx, int dy);

class History {
public:
    void run(std::unique_ptr<Command> cmd, Board& b);
    bool undo(Board& b);
    bool redo(Board& b);
    std::vector<std::string> undoNames() const;
    std::vector<std::string> redoNames() const;

private:
    std::vector<std::unique_ptr<Command>> done_;
    std::vector<std::unique_ptr<Command>> undone_;
};

// ------------------------------------------------------------ Object pool
struct Bullet {
    float x = 0, y = 0, speed = 0;
};

// [snippet:pattern.object_pool]
// Allocate every bullet ONCE, up front. Free slots form a linked list
// stored inside the pool itself (next_[i] = the free slot after i).
template <std::size_t N>
class BulletPool {
public:
    BulletPool() {
        for (std::size_t i = 0; i < N; ++i) next_[i] = static_cast<int>(i) + 1;  // 0 -> 1 -> 2 ...
        next_[N - 1] = -1;
    }
    // O(1): take the first free slot. Returns -1 when the pool is exhausted.
    int acquire() {
        if (freeHead_ < 0) return -1;
        const int slot = freeHead_;
        freeHead_ = next_[static_cast<std::size_t>(slot)];  // @line acquire
        active_[static_cast<std::size_t>(slot)] = true;
        return slot;
    }
    // O(1): push the slot back on the front of the free list.
    void release(int slot) {
        active_[static_cast<std::size_t>(slot)] = false;
        next_[static_cast<std::size_t>(slot)] = freeHead_;
        freeHead_ = slot;  // @line release
    }
    Bullet& operator[](int slot) { return items_[static_cast<std::size_t>(slot)]; }
    bool active(int slot) const { return active_[static_cast<std::size_t>(slot)]; }
    int freeHead() const { return freeHead_; }
    int nextFree(int slot) const { return next_[static_cast<std::size_t>(slot)]; }
    static constexpr std::size_t capacity() { return N; }

private:
    std::array<Bullet, N> items_{};
    std::array<int, N> next_{};
    std::array<bool, N> active_{};
    int freeHead_ = 0;
};
// [/snippet]

// ------------------------------------------------------------ Game loop
struct LoopStats {
    int updates = 0;
    double accumulator = 0;  // leftover time, carried to the next frame
    double alpha = 0;        // how far between two updates we are, for smooth drawing
};
// Fixed-timestep loop: advance the simulation in fixed `dt` steps no
// matter how long each rendered frame took.
std::vector<LoopStats> runFixedStep(const std::vector<double>& frameTimes, double dt, int maxStepsPerFrame);

// ------------------------------------------------------------ Event queue
// [snippet:pattern.event_queue]
// A fixed-size circular queue: no allocation, ever. head = next to read,
// tail = next free slot; both wrap around with % N.
template <typename T, std::size_t N>
class RingQueue {
public:
    bool push(const T& v) {
        if (count_ == N) return false;  // full: the caller decides what to do  // @line full
        items_[tail_] = v;
        tail_ = (tail_ + 1) % N;  // @line push
        ++count_;
        return true;
    }
    bool pop(T& out) {
        if (count_ == 0) return false;
        out = items_[head_];
        head_ = (head_ + 1) % N;  // @line pop
        --count_;
        return true;
    }
    std::size_t size() const { return count_; }
    std::size_t head() const { return head_; }
    std::size_t tail() const { return tail_; }
    const std::array<T, N>& raw() const { return items_; }

private:
    std::array<T, N> items_{};
    std::size_t head_ = 0, tail_ = 0, count_ = 0;
};
// [/snippet]

// ------------------------------------------------------------ State
enum class Input { PressJump, PressDown, ReleaseDown, Land };
const char* toString(Input i);

class Hero;
class HeroState {
public:
    virtual ~HeroState() = default;
    virtual const char* name() const = 0;
    // Returns the next state, or nullptr to stay.
    virtual std::unique_ptr<HeroState> handle(Hero& hero, Input input) = 0;
};

class Hero {
public:
    Hero();
    void handle(Input input);
    const char* stateName() const { return state_->name(); }
    int jumps = 0;

private:
    std::unique_ptr<HeroState> state_;
};

// ------------------------------------------------------------ Flyweight
// [snippet:pattern.flyweight]
struct TreeModel {          // shared by every tree of the same kind
    std::array<float, 3 * 512> mesh{};   // 512 vertices x (x, y, z)
    std::array<unsigned char, 64 * 64 * 4> bark{};  // a small RGBA texture
};
struct TreeHeavy {          // the naive version: every tree carries its own copy
    TreeModel model;
    float x = 0, y = 0, scale = 1;
};
struct TreeLight {          // flyweight: every tree points at one shared model
    const TreeModel* model = nullptr;  // @line shared
    float x = 0, y = 0, scale = 1;
};
// [/snippet]

}  // namespace forge::patterns
