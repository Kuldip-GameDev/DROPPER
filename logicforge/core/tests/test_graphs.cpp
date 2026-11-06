#include <doctest/doctest.h>

#include <algorithm>
#include <random>
#include <string>
#include <vector>

#include "forge/algo/graphs.hpp"

using namespace forge;

TEST_CASE("topological sort respects every edge, and detects cycles") {
    std::mt19937 gen(4);
    for (int trial = 0; trial < 100; ++trial) {
        const int n = 2 + static_cast<int>(gen() % 10);
        std::vector<std::string> nodes;
        for (int i = 0; i < n; ++i) nodes.push_back("n" + std::to_string(i));
        std::vector<std::pair<std::string, std::string>> edges;
        for (int i = 0; i < n; ++i)
            for (int j = i + 1; j < n; ++j)
                if (gen() % 3 == 0) edges.emplace_back(nodes[static_cast<std::size_t>(i)], nodes[static_cast<std::size_t>(j)]);  // i<j: acyclic
        std::shuffle(nodes.begin(), nodes.end(), gen);
        viz::Recorder rec;
        const auto order = algo::topoSort(nodes, edges, rec);
        REQUIRE(order.size() == nodes.size());
        for (const auto& [a, b] : edges) {
            const auto ia = std::find(order.begin(), order.end(), a) - order.begin();
            const auto ib = std::find(order.begin(), order.end(), b) - order.begin();
            CHECK(ia < ib);
        }
    }
    viz::Recorder rec;
    CHECK(algo::topoSort({"a", "b", "c"}, {{"a", "b"}, {"b", "c"}, {"c", "a"}}, rec).empty());
}

TEST_CASE("edge parser: names, lone nodes and validation") {
    std::vector<std::string> nodes;
    std::vector<std::pair<std::string, std::string>> edges;
    algo::parseEdges(" a > b , c, b>c ", nodes, edges);
    CHECK(nodes == std::vector<std::string>{"a", "b", "c"});
    CHECK(edges.size() == 2);
    CHECK_THROWS(algo::parseEdges("a>b c", nodes, edges));  // space inside a name
}
