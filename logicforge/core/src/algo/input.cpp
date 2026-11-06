#include "forge/algo/input.hpp"

#include <cstdlib>
#include <stdexcept>

#include "forge/util/cat.hpp"

namespace forge::algo {

std::vector<int> intArray(const Json& input, const char* key, int maxLen) {
    if (!input.contains(key) || !input[key].is_array()) {
        throw std::invalid_argument(cat("'", key, "' must be a list of numbers"));
    }
    const Json& arr = input[key];
    if (static_cast<int>(arr.size()) > maxLen) {
        throw std::invalid_argument(cat("'", key, "' can have at most ", maxLen, " numbers"));
    }
    std::vector<int> out;
    out.reserve(arr.size());
    for (const Json& v : arr) {
        if (!v.is_number_integer()) {
            throw std::invalid_argument(cat("'", key, "' must contain whole numbers only"));
        }
        const auto x = v.get<long long>();
        if (std::llabs(x) > kMaxAbsValue) {
            throw std::invalid_argument(cat("numbers must be between -", kMaxAbsValue, " and ",
                                            kMaxAbsValue));
        }
        out.push_back(static_cast<int>(x));
    }
    return out;
}

int intValue(const Json& input, const char* key, int lo, int hi) {
    if (!input.contains(key) || !input[key].is_number_integer()) {
        throw std::invalid_argument(cat("'", key, "' must be a whole number"));
    }
    const auto x = input[key].get<long long>();
    if (x < lo || x > hi) {
        throw std::invalid_argument(cat("'", key, "' must be between ", lo, " and ", hi));
    }
    return static_cast<int>(x);
}

std::string textValue(const Json& input, const char* key, int maxLen) {
    if (!input.contains(key) || !input[key].is_string()) {
        throw std::invalid_argument(cat("'", key, "' must be text"));
    }
    auto s = input[key].get<std::string>();
    if (static_cast<int>(s.size()) > maxLen) {
        throw std::invalid_argument(cat("'", key, "' can be at most ", maxLen, " characters"));
    }
    return s;
}

}  // namespace forge::algo
