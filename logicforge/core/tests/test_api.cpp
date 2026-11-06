#include <doctest/doctest.h>

#include <set>
#include <string>

#include "forge/algo/registry.hpp"
#include "forge/bridge/api.hpp"

using namespace forge;

// These tests exercise the exact entry point the JNI bridge calls.

TEST_CASE("api: unknown method returns an error instead of throwing") {
    const Api api;
    const Json r = Json::parse(api.callJson("does.not.exist", "{}"));
    CHECK(r.contains("error"));
}

TEST_CASE("api: malformed JSON returns an error instead of throwing") {
    const Api api;
    const Json r = Json::parse(api.callJson("algo.trace", "{not json"));
    CHECK(r.contains("error"));
}

TEST_CASE("api: algo.list has unique ids and complete info") {
    const Api api;
    const Json list = Json::parse(api.callJson("algo.list", ""));
    REQUIRE(list.is_array());
    REQUIRE(list.size() >= 11);
    std::set<std::string> ids;
    for (const auto& a : list) {
        CHECK(ids.insert(a["id"].get<std::string>()).second);
        CHECK_FALSE(a["title"].get<std::string>().empty());
        CHECK_FALSE(a["snippet"].get<std::string>().empty());
        CHECK_FALSE(a["summary"].get<std::string>().empty());
        CHECK(a["defaults"].is_object());
    }
}

TEST_CASE("api: every algorithm runs on its defaults and produces frames") {
    const Api api;
    for (const auto& e : algo::Registry::builtin().all()) {
        CAPTURE(e.info.id);
        const Json t = Json::parse(api.callJson("algo.trace", Json{{"id", e.info.id}}.dump()));
        REQUIRE_FALSE(t.contains("error"));
        CHECK(t["frames"].size() >= 2);
        CHECK_FALSE(t["truncated"].get<bool>());
        for (const auto& f : t["frames"]) {
            CHECK(f["scene"].contains("kind"));
            CHECK_FALSE(f["caption"].get<std::string>().empty());
        }
    }
}

TEST_CASE("api: bad learner input is reported, not crashed on") {
    const Api api;
    auto err = [&](const Json& input) {
        const Json r = Json::parse(
            api.callJson("algo.trace", Json{{"id", "sort.bubble"}, {"input", input}}.dump()));
        return r.contains("error");
    };
    CHECK(err(Json::object()));                     // missing field
    CHECK(err({{"array", "hello"}}));               // wrong type
    CHECK(err({{"array", {1, 2.5}}}));              // not integers
    CHECK(err({{"array", std::vector<int>(41, 1)}}));  // too long
    CHECK(err({{"array", {1, 100000}}}));           // out of range
    CHECK_FALSE(err({{"array", {3, 1, 2}}}));
}

TEST_CASE("api: meta.version matches the CMake project version") {
    const Api api;
    const Json v = Json::parse(api.callJson("meta.version", "{}"));
    CHECK(v["version"].get<std::string>() == std::string(version()));
}
