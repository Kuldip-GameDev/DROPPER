// Lambda Lab scenarios.
//
// Key idea: a lambda is an OBJECT. The compiler writes a small struct for
// it, called a closure. Captured variables become its members:
//   [x]   copies x into the closure (a member holding a value)
//   [&x]  makes the closure refer to x (think: a member holding x's address)
// The drawings below show the closure as a box, with real sizeof values.
#include <algorithm>
#include <functional>
#include <ostream>
#include <string>
#include <vector>

#include "forge/algo/registry.hpp"
#include "forge/memlab/lab.hpp"

namespace forge::memlab {

using viz::ArrayScene;
using viz::Mark;
using viz::MemoryScene;

// ----------------------------------------------------------------------------
// [snippet:lambda.capture]
void captureDemo() {
    auto frame = lab().enter("main");  // @viz
    std::ostream& out = lab().out;  // @viz
    int coins = 10;  // @line declare
    lab().local("coins", "int", coins);  // @viz
    lab().snap("coins = 10.", "declare");  // @viz
    auto byValue = [coins] { return coins; };  // COPIES coins right now  // @line by_value
    lab().draw([&byValue](MemoryScene& m) { m.local("byValue", cat("closure, ", sizeof(byValue), " bytes"), cat("{ coins = ", byValue(), " }"), lab().book.stack(&byValue, sizeof(byValue))); });  // @viz
    lab().snap(cat("[coins] copies the value 10 INTO the lambda object. sizeof(byValue) = ", sizeof(byValue), " bytes: just one int."), "by_value", {&byValue});  // @viz
    auto byRef = [&coins] { return coins; };  // REFERS to coins  // @line by_ref
    lab().draw([&byRef, &coins](MemoryScene& m) { const auto self = lab().book.stack(&byRef, sizeof(byRef)); m.local("byRef", cat("closure, ", sizeof(byRef), " bytes"), cat("{ &coins }"), self); m.arrow(self, lab().book.of(&coins)); });  // @viz
    lab().snap(cat("[&coins] stores a way to reach coins (in practice its address: sizeof(byRef) = ", sizeof(byRef), " bytes, the size of a pointer here)."), "by_ref", {&byRef});  // @viz
    coins = 50;  // @line change
    lab().snap("coins changes to 50. byValue still holds its own copy (10). byRef will look at coins itself.", "change", {&coins});  // @viz
    out << byValue() << ' ' << byRef() << '\n';  // prints "10 50"  // @line call
    lab().snap("byValue() returns its copy: 10. byRef() follows the reference: 50.", "call");  // @viz
}
// [/snippet]

// ----------------------------------------------------------------------------
// [snippet:lambda.mutable]
void mutableDemo() {
    auto frame = lab().enter("main");  // @viz
    std::ostream& out = lab().out;  // @viz
    int seen = 0;  // @viz
    int spins = 0;
    auto counter = [spins]() mutable { return ++spins; };  // gets its OWN copy of spins  // @line make
    lab().local("spins", "int", spins); lab().draw([&seen, &counter](MemoryScene& m) { m.local("counter", "closure", cat("{ spins = ", seen, " }"), lab().book.stack(&counter, sizeof(counter))); });  // @viz
    lab().snap("[spins] copies 0 into the lambda. Without `mutable` the lambda could not change that copy.", "make");  // @viz
    int first = counter();   // 1  // @line call1
    seen = first; lab().local("first", "int", first);  // @viz
    lab().snap("The first call changes the lambda's own copy to 1.", "call1", {&counter});  // @viz
    int second = counter();  // 2  // @line call2
    seen = second; lab().local("second", "int", second);  // @viz
    lab().snap("The copy lives inside the lambda object, so it remembers: 2.", "call2", {&counter});  // @viz
    out << first << ' ' << second << ' ' << spins << '\n';  // "1 2 0"  // @line print
    lab().snap("The original spins is still 0. The lambda only ever changed its private copy.", "print", {&spins});  // @viz
}
// [/snippet]

// ----------------------------------------------------------------------------
namespace {
thread_local const void* tlDeadCount = nullptr;  // where the broken lambda's reference points
}

// [snippet:lambda.dangling]
std::function<int()> makeCounterBroken() {
    auto frame = lab().enter("makeCounterBroken");  // @viz
    int count = 0;
    lab().local("count", "int", count); tlDeadCount = &count;  // @viz
    lab().snap("count is a local variable in makeCounterBroken's stack frame.", "bad_capture", {&count});  // @viz
    return [&count] { return ++count; };  // BUG: count dies when we return  // @line bad_capture
}

std::function<int()> makeCounterFixed() {
    int count = 0;
    return [count]() mutable { return ++count; };  // the lambda keeps its own copy  // @line good_capture
}

void danglingDemo() {
    auto frame = lab().enter("main");  // @viz
    std::ostream& out = lab().out;  // @viz
    auto broken = makeCounterBroken();  // holds a reference to a DEAD variable  // @line call_broken
    lab().draw([&broken](MemoryScene& m) { lab().drawPointer(m, "broken", "function<int()>", lab().book.stack(&broken, sizeof(broken)), tlDeadCount); });  // @viz
    lab().snap("The function returned, so its frame and count are gone. broken still refers to that dead slot (red).", "call_broken", {&broken});  // @viz
    // broken();  // undefined behaviour: garbage, a crash, or "works" today and fails later
    auto fixed = makeCounterFixed();  // @line call_fixed
    lab().draw([&fixed](MemoryScene& m) { m.local("fixed", "function<int()>", "{ count = own copy }", lab().book.stack(&fixed, sizeof(fixed))); });  // @viz
    out << fixed() << ' ' << fixed() << '\n';  // "1 2"  // @line use_fixed
    lab().snap("fixed captured by value, so its count travels with the lambda. Calling it is safe: 1, 2.", "use_fixed", {&fixed});  // @viz
}
// [/snippet]

// ----------------------------------------------------------------------------
namespace {
struct Player {
    std::string name;
    int score;
};

ArrayScene board(const std::vector<Player>& players, const std::string& title) {
    std::vector<std::string> labels;
    for (const auto& p : players) labels.push_back(cat(p.name, " ", p.score));
    return ArrayScene::ofLabels(labels).title(title);
}
}  // namespace

// [snippet:lambda.sort]
void sortDemo(viz::Recorder& rec) {
    std::vector<Player> players = {{"Ava", 320}, {"Ben", 980}, {"Cy", 540}, {"Dee", 760}};
    rec.frame(board(players, "leaderboard (unsorted)"), "Players in the order they joined.", "data");  // @viz
    int comparisons = 0;  // @line data
    std::sort(players.begin(), players.end(), [&comparisons](const Player& a, const Player& b) {
        ++comparisons;              // captured by reference: we can count calls
        return a.score > b.score;   // true means "a goes before b"  // @line compare
    });
    rec.frame(board(players, "leaderboard (sorted)").markRange(0, 1, Mark::Found).var("comparisons", comparisons), cat("Sorted by score, highest first. std::sort called our lambda ", comparisons, " times (the exact number depends on the library)."), "compare");  // @viz
}
// [/snippet]

// ----------------------------------------------------------------------------
// [snippet:lambda.callbacks]
void callbacksDemo() {
    auto frame = lab().enter("main");  // @viz
    std::ostream& out = lab().out;  // @viz
    int totalWon = 0;
    std::vector<std::function<void(int)>> onWin;  // "call these when a spin wins"  // @line declare
    lab().local("totalWon", "int", totalWon); lab().draw([&onWin](MemoryScene& m) { m.local("onWin", "vector<function<void(int)>>", cat("size = ", onWin.size()), lab().book.stack(&onWin, sizeof(onWin))); });  // @viz
    lab().snap("An empty list of callbacks. std::function can hold any callable with the right signature.", "declare");  // @viz
    onWin.push_back([&totalWon](int amount) { totalWon += amount; });  // @line add_total
    onWin.push_back([&out](int amount) {
        if (amount >= 100) out << "BIG WIN: " << amount << '\n';
    });  // @line add_banner
    lab().snap("Two listeners subscribed: one keeps a running total, one shows a banner for big wins.", "add_banner");  // @viz
    for (int amount : {20, 150}) {
        for (auto& listener : onWin) listener(amount);  // notify everyone  // @line notify
        lab().snap(cat("A spin won ", amount, ". Every listener was called with it."), "notify", {&totalWon});  // @viz
    }
    out << "total " << totalWon << '\n';  // @line print
    lab().snap("The spin code never knew what the listeners do. That's the Observer pattern, built from lambdas.", "print");  // @viz
}
// [/snippet]

// ----------------------------------------------------------------------------

void registerLambdaLab(algo::Registry& r) {
    auto add = [&r](const char* id, const char* title, const char* summary, void (*scenario)()) {
        r.add({id, title, "Lambda Lab", id, "", "", summary, {}, Json::object()},
              [scenario](const Json&, viz::Recorder& rec) -> Json {
                  Lab session(rec);
                  scenario();
                  return {{"output", session.lines()}};
              });
    };
    add("lambda.capture", "Capture by value vs by reference", "[x] copies, [&x] refers. A lambda is an object.", captureDemo);
    add("lambda.mutable", "mutable lambdas", "A lambda that changes its own copy of a captured value.", mutableDemo);
    add("lambda.dangling", "Dangling reference captures", "Returning a lambda that refers to a dead local.", danglingDemo);
    r.add({"lambda.sort", "Lambdas as sort comparators", "Lambda Lab", "lambda.sort", "", "",
           "Tell std::sort how to order things, and count how often it asks.", {}, Json::object()},
          [](const Json&, viz::Recorder& rec) -> Json {
              sortDemo(rec);
              return Json::object();
          });
    add("lambda.callbacks", "Callbacks with std::function", "Store lambdas and call them later: the Observer pattern.", callbacksDemo);
}

}  // namespace forge::memlab
