#pragma once
// Reading and validating learner input. A teaching app must never crash
// because someone typed 500 numbers or left a field empty.
#include <string>
#include <vector>

#include "forge/json.hpp"

namespace forge::algo {

inline constexpr int kMaxArrayLen = 40;
inline constexpr int kMaxAbsValue = 9999;
inline constexpr int kMaxTextLen = 80;

// Throws std::invalid_argument with a friendly message on bad input.
std::vector<int> intArray(const Json& input, const char* key, int maxLen = kMaxArrayLen);
int intValue(const Json& input, const char* key, int lo, int hi);
std::string textValue(const Json& input, const char* key, int maxLen = kMaxTextLen);

}  // namespace forge::algo
