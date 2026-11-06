#include <doctest/doctest.h>

#include "forge/viz/scene.hpp"

using namespace forge;
using namespace forge::viz;

TEST_CASE("ArrayScene serialises values, marks, pointers and window") {
    const Json j = ArrayScene({5, 1, 4}).compare(0, 1).pointer("i", 0).window(0, 2).var("sum", 6).toJson();
    CHECK(j["kind"] == "array");
    CHECK(j["values"] == Json({5, 1, 4}));
    CHECK(j["labels"] == Json({"5", "1", "4"}));
    CHECK(j["marks"]["0"] == "compare");
    CHECK(j["marks"]["1"] == "compare");
    CHECK_FALSE(j["marks"].contains("2"));
    CHECK(j["pointers"][0]["name"] == "i");
    CHECK(j["window"] == Json({0, 2}));
    CHECK(j["vars"][0]["value"] == 6);
}

TEST_CASE("sortedFrom does not overwrite a more specific mark") {
    const Json j = ArrayScene({1, 2, 3}).swapped(1, 2).sortedFrom(1).toJson();
    CHECK(j["marks"]["1"] == "swap");
    CHECK(j["marks"]["2"] == "swap");
}

TEST_CASE("GraphScene tree layout puts children below and parents centred") {
    GraphScene g;
    g.node("a", "8").node("b", "3").node("c", "10").edge("a", "b", "L").edge("a", "c", "R").layoutTree("a");
    const Json j = g.toJson();
    const auto& n = j["nodes"];
    CHECK(n[0]["y"] == 0.0);
    CHECK(n[1]["y"] == 1.0);
    CHECK(n[1]["x"].get<double>() < n[0]["x"].get<double>());
    CHECK(n[2]["x"].get<double>() > n[0]["x"].get<double>());
}

TEST_CASE("GraphScene tree layout distinguishes an only-right child") {
    GraphScene g;
    g.node("a", "1").node("b", "2").edge("a", "b", "R").layoutTree("a");
    const Json j = g.toJson();
    CHECK(j["nodes"][1]["x"].get<double>() > j["nodes"][0]["x"].get<double>());
}

TEST_CASE("GridScene validates cells") {
    GridScene g(2, 3);
    CHECK_THROWS(g.set(2, 0, 1));
    CHECK_THROWS(g.mark(0, 3, Mark::Active));
    g.set(1, 2, 7).mark(1, 2, Mark::Found).path({{0, 0}, {1, 1}});
    const Json j = g.toJson();
    CHECK(j["cells"][5] == "7");
    CHECK(j["marks"]["5"] == "found");
    CHECK(j["paths"][0]["cells"].size() == 2);
}
