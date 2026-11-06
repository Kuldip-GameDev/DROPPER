#include <doctest/doctest.h>

#include <string>
#include <vector>

#include "forge/algo/registry.hpp"
#include "forge/memlab/lab.hpp"

using namespace forge;

namespace {
struct [[maybe_unused]] Run {
    viz::Recorder rec;
    Json result;
};

[[maybe_unused]] Run run(const std::string& id) {
    const auto* e = algo::Registry::builtin().find(id);
    REQUIRE(e != nullptr);
    Run r;
    r.result = e->run(Json::object(), r.rec);
    return r;
}

[[maybe_unused]] std::vector<std::string> output(const Run& r) { return r.result["output"].get<std::vector<std::string>>(); }

// Value of a stack variable in a frame's memory scene, or "" if absent.
[[maybe_unused]] std::string localValue(const Json& scene, const std::string& name) {
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

TEST_CASE("Lab requires a running session") {
    CHECK_THROWS(memlab::Lab::current());
}
