#include <doctest/doctest.h>

#include <string>
#include <vector>

#include "forge/algo/registry.hpp"
#include "forge/memlab/lab.hpp"

using namespace forge;

namespace {
struct Run {
    viz::Recorder rec;
    Json result;
};

Run run(const std::string& id) {
    const auto* e = algo::Registry::builtin().find(id);
    REQUIRE(e != nullptr);
    Run r;
    r.result = e->run(Json::object(), r.rec);
    return r;
}

std::vector<std::string> output(const Run& r) { return r.result["output"].get<std::vector<std::string>>(); }

// Value of a stack variable in a frame's memory scene, or "" if absent.
std::string localValue(const Json& scene, const std::string& name) {
    for (const auto& f : scene["stack"])
        for (const auto& c : f["locals"])
            if (c["name"] == name) return c["value"].get<std::string>();
    return "";
}

[[maybe_unused]] bool hasDanglingArrow(const Json& scene) {
    for (const auto& a : scene["arrows"])
        if (a["mark"] == "error" && a["label"] == "dangling") return true;
    return false;
}
}  // namespace

TEST_CASE("AddressBook keeps real offsets inside an object") {
    memlab::AddressBook book;
    int arr[4] = {0, 0, 0, 0};
    const auto base = book.stack(arr, sizeof arr);
    CHECK(book.of(&arr[0]) == base);
    // Friendly addresses are hex; arr[2] must be exactly 2 * sizeof(int) after arr[0].
    const auto toNum = [](const std::string& s) { return std::stoul(s, nullptr, 16); };
    CHECK(toNum(book.of(&arr[2])) - toNum(base) == 2 * sizeof(int));
    CHECK(book.of(nullptr) == "nullptr");
    int unknown = 0;
    CHECK(book.of(&unknown) == "?");
}

TEST_CASE("AddressBook marks freed memory as dangling") {
    memlab::AddressBook book;
    int* p = new int(1);
    book.heap(p, sizeof(int));
    CHECK_FALSE(book.freed(p));
    book.markFreed(p);
    CHECK(book.freed(p));
    delete p;
}

TEST_CASE("pointer scenarios print what real C++ prints") {
    CHECK(output(run("mem.address_of")) == std::vector<std::string>{"42", "100"});
    CHECK(output(run("mem.array_pointer")) == std::vector<std::string>{"7", "9", "5"});
}

TEST_CASE("swap scenario: by value changes nothing, by pointer swaps") {
    const auto r = run("mem.swap");
    const auto& frames = r.rec.frames();
    REQUIRE(frames.size() >= 4);
    // Find the frame right after swapByValue returned, and the last one.
    bool sawUnchanged = false;
    for (const auto& f : frames) {
        if (f.line == "call_value") {
            CHECK(localValue(f.scene, "x") == "1");
            CHECK(localValue(f.scene, "y") == "2");
            sawUnchanged = true;
        }
    }
    CHECK(sawUnchanged);
    CHECK(localValue(frames.back().scene, "x") == "2");
    CHECK(localValue(frames.back().scene, "y") == "1");
}

TEST_CASE("delete leaves a dangling pointer, nullptr clears it") {
    const auto r = run("mem.heap");
    bool dangling = false;
    for (const auto& f : r.rec.frames()) {
        if (f.line == "delete") dangling = hasDanglingArrow(f.scene);
    }
    CHECK(dangling);
    CHECK(localValue(r.rec.frames().back().scene, "coins") == "nullptr");
    CHECK_FALSE(hasDanglingArrow(r.rec.frames().back().scene));
}

TEST_CASE("unique_ptr move empties the source") {
    const auto r = run("mem.unique_ptr");
    for (const auto& f : r.rec.frames()) {
        if (f.line == "move") {
            CHECK(localValue(f.scene, "gold") == "nullptr");
            CHECK(localValue(f.scene, "vault") != "nullptr");
        }
    }
}

TEST_CASE("shared_ptr and weak_ptr report real owner counts") {
    const auto shared = run("mem.shared_ptr");
    bool sawTwo = false;
    for (const auto& f : shared.rec.frames())
        if (f.line == "copy") sawTwo = f.caption.find("use_count() = 2") != std::string::npos;
    CHECK(sawTwo);

    const auto weak = run("mem.weak_ptr");
    for (const auto& f : weak.rec.frames()) {
        if (f.line == "rival") CHECK(f.caption.find("still has 1 owner") != std::string::npos);
    }
    // At the end both heroes were really freed (LeakSanitizer would also fail this test otherwise).
    const Json& last = weak.rec.frames().back().scene;
    REQUIRE(last["heap"].size() == 2);
    CHECK(last["heap"][0]["freed"] == true);
    CHECK(last["heap"][1]["freed"] == true);
}

TEST_CASE("Lab requires a running session") {
    CHECK_THROWS(memlab::Lab::current());
}
