#pragma once
#include <string>

#include "forge/viz/recorder.hpp"

namespace forge::algo {

// True if every (, [ and { is closed in the right order. Other characters are ignored.
bool isBalanced(const std::string& text, viz::Recorder& rec);

}  // namespace forge::algo
