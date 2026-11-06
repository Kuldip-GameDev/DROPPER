#pragma once
// One alias so the rest of the code never spells out nlohmann::json.
#include <nlohmann/json.hpp>

namespace forge {
using Json = nlohmann::json;
}  // namespace forge
