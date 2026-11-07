#include <doctest/doctest.h>

#include "forge/bridge/api.hpp"
#include "forge/dsl/dsl.hpp"

using namespace forge;
using namespace forge::dsl;

TEST_CASE("lexer: words, numbers, negatives, arrows and dashes") {
    const auto t = lex("list -4 -> 2, A-B:3");
    std::vector<Token::Type> types;
    for (const auto& x : t) types.push_back(x.type);
    using T = Token::Type;
    CHECK(types == std::vector<T>{T::Word, T::Number, T::Arrow, T::Number, T::Comma, T::Word, T::Dash, T::Word, T::Colon, T::Number, T::End});
    CHECK(t[1].text == "-4");
    CHECK_THROWS(lex("array 5 $"));
}

TEST_CASE("parser: forgiving phrasings mean the same thing") {
    for (const char* s : {"array 5 3 8", "an array of 5, 3 and 8", "Array 5 -> 3 -> 8", "show me the arr 5 3 8"}) {
        CAPTURE(s);
        const Command c = parse(lex(s));
        CHECK(c.kind == "array");
        CHECK(c.numbers == std::vector<int>{5, 3, 8});
    }
    CHECK(parse(lex("sorted array 3 1")).sorted);
    CHECK(parse(lex("draw a binary search tree with 8 3")).kind == "tree");
    CHECK(parse(lex("bst 1")).kind == "tree");
}

TEST_CASE("parser: stack/queue operations and graph edges") {
    const Command s = parse(lex("stack push 3 push 4 pop 7"));
    REQUIRE(s.ops.size() == 4);
    CHECK(s.ops[2].first == "pop");
    CHECK(s.ops[3] == std::pair<std::string, int>{"push", 7});
    const Command g = parse(lex("graph A-B:4, B->C"));
    REQUIRE(g.edges.size() == 2);
    CHECK(g.edges[0].weight == 4);
    CHECK_FALSE(g.edges[0].directed);
    CHECK(g.edges[1].directed);
}

TEST_CASE("parser: friendly errors") {
    CHECK_THROWS_WITH_AS(parse(lex("pizza 3")), doctest::Contains("pizza"), std::invalid_argument);
    CHECK_THROWS_AS(parse(lex("stack push")), std::invalid_argument);
    CHECK_THROWS_AS(parse(lex("graph A")), std::invalid_argument);
    CHECK_THROWS_AS(parse(lex("")), std::invalid_argument);
}

TEST_CASE("dsl.render produces frames for every kind and never throws across the API") {
    const Api api;
    for (const char* s : {"array 5 3 8", "sorted array 9 2 7", "list 1 2 3", "tree 8 3 10", "heap 5 9 3",
                          "stack push 1 pop pop", "queue 1 2 dequeue", "graph A-B, B-C:2"}) {
        CAPTURE(s);
        const Json r = Json::parse(api.callJson("dsl.render", Json{{"text", s}}.dump()));
        REQUIRE_FALSE(r.contains("error"));
        CHECK(r["frames"].size() >= 1);
        CHECK(r["result"]["tokens"].size() >= 2);
    }
    CHECK(Json::parse(api.callJson("dsl.render", R"({"text":"array"})")).contains("error"));
    CHECK(Json::parse(api.callJson("dsl.render", R"({"text":"array 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17"})")).contains("error"));
}
