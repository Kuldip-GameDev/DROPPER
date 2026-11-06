#pragma once
// forge::Api is the ONLY door into the core.
//
// The Android app, the terminal tool and the tests all talk to the core
// through Api::call / Api::callJson. One door means one place to test,
// log and secure. See docs/adr/0002-json-jni-bridge.md.
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "forge/json.hpp"

namespace forge {

class Api {
public:
    using Handler = std::function<Json(const Json& args)>;

    Api();  // registers every built-in method

    // Add or replace a method. Handlers are usually lambdas.
    void on(std::string method, Handler handler);

    // Throws std::invalid_argument for unknown methods / bad input.
    Json call(const std::string& method, const Json& args) const;

    // Never throws. Errors come back as {"error": "..."}. This is what
    // crosses the JNI boundary, where an escaping C++ exception would
    // crash the whole app.
    std::string callJson(const std::string& method, const std::string& argsJson) const noexcept;

    std::vector<std::string> methods() const;

private:
    std::unordered_map<std::string, Handler> handlers_;
};

const char* version();

}  // namespace forge
