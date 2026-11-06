#pragma once
#include <string>
#include <utility>
#include <vector>

#include "forge/viz/recorder.hpp"

namespace forge::algo {

// Kahn's algorithm. edges: {a, b} means "a must come before b".
// Returns an order containing every node, or an empty vector if the
// edges contain a cycle (then no valid order exists).
std::vector<std::string> topoSort(const std::vector<std::string>& nodes,
                                  const std::vector<std::pair<std::string, std::string>>& edges,
                                  viz::Recorder& rec);

// "socks>shoes, pants>shoes" -> nodes in first-seen order + edges.
void parseEdges(const std::string& text, std::vector<std::string>& nodes,
                std::vector<std::pair<std::string, std::string>>& edges);

}  // namespace forge::algo
