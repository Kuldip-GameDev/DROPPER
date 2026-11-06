// Visual traces for the design-pattern examples.
#include <cmath>
#include <stdexcept>

#include "forge/algo/input.hpp"
#include "forge/algo/registry.hpp"
#include "forge/patterns/patterns.hpp"
#include "forge/patterns/strategy_singleton.hpp"
#include "forge/util/cat.hpp"
#include "forge/viz/scene.hpp"

namespace forge::patterns {

using viz::ArrayScene;
using viz::GraphScene;
using viz::GridScene;
using viz::Mark;
using viz::PlotScene;
using viz::Recorder;

namespace {

std::string joined(const std::vector<std::string>& v) {
    std::string s;
    for (const auto& x : v) s += (s.empty() ? "" : ", ") + x;
    return s.empty() ? "(empty)" : s;
}

GridScene boardScene(const Board& b, const History& h) {
    GridScene g(b.height, b.width);
    for (int r = 0; r < b.height; ++r)
        for (int c = 0; c < b.width; ++c) g.set(r, c, "·");
    g.set(b.y, b.x, "P").mark(b.y, b.x, Mark::Active);
    g.var("undo stack", joined(h.undoNames())).var("redo stack", joined(h.redoNames()));
    return g;
}

Json commandTrace(Recorder& rec) {
    Board b;
    History h;
    rec.frame(boardScene(b, h), "P stands on a 5x5 board. Every move will be a Command object.", "");
    struct Step { const char* action; int dx, dy; };
    const Step script[] = {{"run", 1, 0}, {"run", 1, 0}, {"run", 0, 1}, {"undo", 0, 0}, {"undo", 0, 0}, {"redo", 0, 0}, {"run", 0, 1}, {"redo", 0, 0}};
    for (const auto& s : script) {
        const std::string a = s.action;
        if (a == "run") {
            auto cmd = makeMove(s.dx, s.dy);
            const std::string n = cmd->name();
            h.run(std::move(cmd), b);
            rec.frame(boardScene(b, h), cat("Run '", n, "': execute it, push it on the undo stack. A NEW action also clears the redo stack."), "run");
        } else if (a == "undo") {
            h.undo(b);
            rec.frame(boardScene(b, h), "Undo: pop the last command, call its undo(), keep it on the redo stack.", "undo");
        } else {
            const bool ok = h.redo(b);
            rec.frame(boardScene(b, h), ok ? "Redo: pop from the redo stack and execute again." : "Redo does nothing: the new move wiped the old future.", "redo");
        }
    }
    return {{"x", b.x}, {"y", b.y}};
}

ArrayScene poolScene(const BulletPool<8>& pool) {
    std::vector<std::string> labels;
    for (int i = 0; i < 8; ++i) labels.push_back(pool.active(i) ? "bullet" : (pool.nextFree(i) < 0 ? "free→end" : cat("free→", pool.nextFree(i))));
    auto s = ArrayScene::ofLabels(labels).title("8 pre-allocated slots");
    for (int i = 0; i < 8; ++i) s.mark(i, pool.active(i) ? Mark::Found : Mark::None);
    if (pool.freeHead() >= 0) s.pointer("freeHead", pool.freeHead());
    s.var("freeHead", pool.freeHead());
    return s;
}

Json poolTrace(Recorder& rec) {
    BulletPool<8> pool;
    rec.frame(poolScene(pool), "All 8 bullets exist from the start. The free slots form a linked list: 0 → 1 → 2 → ...", "");
    int a = pool.acquire(), b2 = pool.acquire(), c = pool.acquire();
    (void)a;
    (void)c;
    rec.frame(poolScene(pool), "Fire 3 bullets: each acquire() takes the head of the free list. O(1), no new.", "acquire");
    pool.release(b2);
    rec.frame(poolScene(pool), cat("Bullet ", b2, " hits a wall: release() pushes its slot back on the FRONT of the free list. O(1), no delete."), "release");
    const int again = pool.acquire();
    rec.frame(poolScene(pool), cat("The next shot reuses slot ", again, ": memory is recycled, never re-allocated."), "acquire");
    int last = 0;
    while ((last = pool.acquire()) >= 0) {}
    rec.frame(poolScene(pool), "Keep firing until every slot is busy: acquire() returns -1. A real game would skip the shot, reuse the oldest bullet, or grow the pool.", "acquire");
    return {{"reused", again}};
}

Json loopTrace(const Json& input, Recorder& rec) {
    const auto frames = algo::intArray(input, "frames", 20);
    if (frames.empty()) throw std::invalid_argument("'frames' needs at least one frame time");
    for (int f : frames) if (f <= 0 || f > 500) throw std::invalid_argument("frame times must be 1-500 ms");
    const int dt = algo::intValue(input, "dt", 5, 100);
    std::vector<double> ft(frames.begin(), frames.end());
    const auto stats = runFixedStep(ft, dt, 5);
    PlotScene p;
    p.title(cat("simulation updates per rendered frame (dt = ", dt, " ms)")).axes("frame", "updates");
    for (std::size_t i = 0; i < stats.size(); ++i) {
        p.bar(cat(frames[i], "ms"), stats[i].updates, stats[i].updates == 5 ? Mark::Error : Mark::Active);
        PlotScene shown = p;
        shown.var("accumulator", cat(std::round(stats[i].accumulator * 10) / 10, " ms")).var("alpha", std::round(stats[i].alpha * 100) / 100);
        rec.frame(shown, cat("Frame took ", frames[i], " ms. Bank it, then step while at least ", dt, " ms is banked: ", stats[i].updates, " update(s). ", std::round(stats[i].accumulator * 10) / 10, " ms carry over; render blends ", std::round(stats[i].alpha * 100), "% towards the next state.", stats[i].updates == 5 ? " Capped at 5 steps so a slow frame can't snowball." : ""), stats[i].updates == 5 ? "clamp" : "step");
    }
    int total = 0;
    for (const auto& s : stats) total += s.updates;
    return {{"updates", total}};
}

ArrayScene queueScene(const RingQueue<char, 6>& q) {
    std::vector<std::string> labels;
    for (std::size_t i = 0; i < 6; ++i) {
        // occupied if i is within [head, head + size) modulo 6
        const bool used = (i + 6 - q.head()) % 6 < q.size();
        labels.push_back(used ? std::string(1, q.raw()[i]) : "·");
    }
    auto s = ArrayScene::ofLabels(labels).title("ring buffer, capacity 6");
    for (std::size_t i = 0; i < 6; ++i)
        if ((i + 6 - q.head()) % 6 < q.size()) s.mark(static_cast<int>(i), Mark::Window);
    s.pointer("head", static_cast<int>(q.head())).pointer("tail", static_cast<int>(q.tail())).var("size", q.size());
    return s;
}

Json queueTrace(Recorder& rec) {
    RingQueue<char, 6> q;
    rec.frame(queueScene(q), "Sound and UI events go into a queue and are processed later, in order. head and tail start together.", "");
    for (char c : std::string("ABCD")) q.push(c);
    rec.frame(queueScene(q), "Push A, B, C, D: each goes in at tail, and tail moves right.", "push");
    char out = 0;
    q.pop(out);
    q.pop(out);
    rec.frame(queueScene(q), "Pop twice (A, B): head moves right. The old cells are free again.", "pop");
    for (char c : std::string("EFG")) q.push(c);
    rec.frame(queueScene(q), "Push E, F, G: tail runs off the end and WRAPS to index 0. Same modulo trick as a reel strip.", "push");
    const bool ok = q.push('H');
    q.push('I');
    const bool full = !q.push('J');
    rec.frame(queueScene(q), cat("Push H", ok ? "" : " (rejected)", ", I. Now 6 events wait: the queue is full", full ? ", so J is rejected. The caller decides: drop it, merge it with a similar event, or process sooner." : "."), "full");
    return {{"size", q.size()}};
}

GraphScene heroScene(const char* current, const char* from) {
    GraphScene g;
    for (const char* s : {"Standing", "Jumping", "Ducking", "Diving"}) g.node(s, s, std::string(s) == current ? Mark::Active : Mark::None);
    g.at("Standing", 1, 1).at("Jumping", 3, 0).at("Ducking", 3, 2).at("Diving", 5, 0);
    auto e = [&](const char* a, const char* b, const char* label) {
        g.edge(a, b, label, true, std::string(a) == from && std::string(b) == current ? Mark::Compare : Mark::None);
    };
    e("Standing", "Jumping", "jump");
    e("Standing", "Ducking", "down");
    e("Ducking", "Standing", "release");
    e("Jumping", "Diving", "down");
    e("Jumping", "Standing", "land");
    e("Diving", "Standing", "land");
    return g;
}

Json stateTrace(Recorder& rec) {
    Hero hero;
    rec.frame(heroScene(hero.stateName(), ""), "The hero starts Standing. Each state is a class that decides what each input means.", "");
    const Input script[] = {Input::PressJump, Input::PressJump, Input::PressDown, Input::Land, Input::PressDown, Input::PressJump, Input::ReleaseDown};
    for (Input in : script) {
        const std::string before = hero.stateName();
        hero.handle(in);
        const std::string after = hero.stateName();
        const char* line = before == "Standing" && after == "Jumping" ? "stand_jump" : before == "Jumping" && after == "Diving" ? "jump_dive" : before == "Ducking" && after == "Standing" ? "duck_stand" : "switch";
        rec.frame(heroScene(hero.stateName(), before.c_str()).var("jumps", hero.jumps),
                  before == after ? cat(toString(in), " while ", before, ": this state ignores it. (No accidental double jump, no jumping out of a duck.)")
                                  : cat(toString(in), ": ", before, " → ", after, "."),
                  line);
    }
    return {{"state", hero.stateName()}, {"jumps", hero.jumps}};
}

Json flyweightTrace(const Json& input, Recorder& rec) {
    const int trees = algo::intValue(input, "trees", 1, 100000);
    const double mb = 1024.0 * 1024.0;
    const double heavy = static_cast<double>(trees) * sizeof(TreeHeavy) / mb;
    const double light = (static_cast<double>(trees) * sizeof(TreeLight) + sizeof(TreeModel)) / mb;
    PlotScene naive;
    naive.title(cat(trees, " trees: memory in MB")).axes("", "MB").bar("own copy each", std::round(heavy * 100) / 100, Mark::Error);
    naive.var("sizeof(TreeHeavy)", sizeof(TreeHeavy));
    rec.frame(naive, cat("A forest of ", trees, " trees where every tree stores its own mesh and bark texture: ", sizeof(TreeHeavy), " bytes each."), "");
    PlotScene p;
    p.title(cat(trees, " trees: memory in MB")).axes("", "MB");
    p.bar("own copy each", std::round(heavy * 100) / 100, Mark::Error).bar("shared model", std::round(light * 100) / 100, Mark::Ok);
    p.var("sizeof(TreeModel)", sizeof(TreeModel)).var("sizeof(TreeHeavy)", sizeof(TreeHeavy)).var("sizeof(TreeLight)", sizeof(TreeLight));
    rec.frame(p, cat("Every tree carrying its own mesh and texture: ", std::round(heavy * 10) / 10, " MB. One shared model plus a small per-tree struct (position, scale, pointer): ", std::round(light * 100) / 100, " MB. Same forest, about ", std::lround(heavy / light), "x less memory."), "shared");
    return {{"heavyMB", heavy}, {"lightMB", light}};
}

Json strategyTrace(Recorder& rec) {
    const auto& g = slot::gemWays();
    const slot::Screen s = slot::window(g, {0, 0, 3, 1, 0});
    GridScene grid(g.rows, static_cast<int>(g.reels.size()));
    for (int r = 0; r < g.rows; ++r)
        for (int k = 0; k < static_cast<int>(g.reels.size()); ++k) grid.set(r, k, g.symbols[static_cast<std::size_t>(s[static_cast<std::size_t>(r)][static_cast<std::size_t>(k)])].label);
    rec.frame(grid, "One screen, two pay rules. The machine code only calls settle(rule, game, screen).", "");
    const CentreLinePay line;
    const WaysPay ways;
    GridScene withLine = grid;
    withLine.path({{1, 0}, {1, 1}, {1, 2}, {1, 3}, {1, 4}}, Mark::Pivot).var("rule", line.name()).var("pays", settle(line, g, s));
    rec.frame(withLine, cat("Strategy 1, ", line.name(), ": pays ", settle(line, g, s), "."), "settle");
    GridScene withWays = grid;
    withWays.var("rule", ways.name()).var("pays", settle(ways, g, s));
    rec.frame(withWays, cat("Strategy 2, ", ways.name(), ": pays ", settle(ways, g, s), ". Same caller, different algorithm, chosen at runtime."), "settle");
    return {{"line", settle(line, g, s)}, {"ways", settle(ways, g, s)}};
}

}  // namespace

void registerPatterns(algo::Registry& r) {
    const std::vector<algo::Field> none;
    const Json noInput = Json::object();
    r.add({"pattern.command", "Command: undo and redo", "Design patterns", "pattern.command", "", "",
           "Turn every action into an object that can execute and undo itself.", none, noInput},
          [](const Json&, Recorder& rec) { return commandTrace(rec); });
    r.add({"pattern.object_pool", "Object pool", "Design patterns", "pattern.object_pool", "O(1) acquire/release", "",
           "Allocate once, recycle forever: no new/delete in the middle of a frame.", none, noInput},
          [](const Json&, Recorder& rec) { return poolTrace(rec); });
    r.add({"pattern.game_loop", "Game loop: fixed timestep", "Design patterns", "pattern.game_loop", "", "",
           "Steady physics on any device: bank real time, spend it in fixed steps.",
           {{"frames", "int_array", "Frame times in ms"}, {"dt", "int", "Fixed step in ms"}},
           {{"frames", {16, 33, 8, 50, 16, 16, 120}}, {"dt", 20}}},
          loopTrace);
    r.add({"pattern.event_queue", "Event queue (ring buffer)", "Design patterns", "pattern.event_queue", "O(1) push/pop", "",
           "Decouple WHEN something happens from when it's handled, with zero allocations.", none, noInput},
          [](const Json&, Recorder& rec) { return queueTrace(rec); });
    r.add({"pattern.state", "State pattern", "Design patterns", "pattern.state", "", "",
           "Each state is a class; inputs mean different things in different states.", none, noInput},
          [](const Json&, Recorder& rec) { return stateTrace(rec); });
    r.add({"pattern.flyweight", "Flyweight", "Design patterns", "pattern.flyweight", "", "",
           "Share the heavy data, keep only what differs per object.",
           {{"trees", "int", "Number of trees"}}, {{"trees", 10000}}},
          flyweightTrace);
    r.add({"pattern.strategy", "Strategy", "Design patterns", "pattern.strategy", "", "",
           "Swap algorithms behind one interface: here, two ways to pay a slot screen.", none, noInput},
          [](const Json&, Recorder& rec) { return strategyTrace(rec); });
}

}  // namespace forge::patterns
