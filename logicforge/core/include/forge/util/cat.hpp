#pragma once
// cat("a = ", 5, ", b = ", 2.5) -> "a = 5, b = 2.5"
//
// A tiny variadic template. The "fold expression" `(out << ... << args)`
// expands to  out << arg1 << arg2 << arg3 ...  at compile time.
// We use it instead of std::format so the same code builds with every
// toolchain we target (desktop GCC/Clang and the Android NDK).
#include <sstream>
#include <string>

namespace forge {

template <typename... Args>
std::string cat(const Args&... args) {
    std::ostringstream out;
    (out << ... << args);
    return out.str();
}

}  // namespace forge
