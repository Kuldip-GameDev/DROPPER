#include "forge/bridge/api.hpp"

#include <algorithm>
#include <stdexcept>

#include "forge/algo/registry.hpp"
#include "forge/viz/recorder.hpp"

namespace forge {

const char* version() { return FORGE_VERSION; }

Api::Api() {
    on("meta.version", [](const Json&) -> Json { return {{"version", version()}}; });

    on("meta.methods", [this](const Json&) -> Json { return methods(); });

    on("algo.list", [](const Json&) -> Json {
        Json out = Json::array();
        for (const auto& e : algo::Registry::builtin().all()) out.push_back(algo::toJson(e.info));
        return out;
    });

    on("algo.trace", [](const Json& args) -> Json {
        const std::string id = args.value("id", "");
        const auto* entry = algo::Registry::builtin().find(id);
        if (entry == nullptr) throw std::invalid_argument("unknown algorithm: " + id);
        const Json input = args.contains("input") ? args["input"] : entry->info.defaults;
        viz::Recorder rec;
        Json result = entry->run(input, rec);
        Json out = rec.toJson();
        out["id"] = id;
        out["result"] = std::move(result);
        return out;
    });

    registerLearnApi(*this);
}

void Api::on(std::string method, Handler handler) {
    handlers_[std::move(method)] = std::move(handler);
}

Json Api::call(const std::string& method, const Json& args) const {
    const auto it = handlers_.find(method);
    if (it == handlers_.end()) throw std::invalid_argument("unknown method: " + method);
    return it->second(args);
}

std::string Api::callJson(const std::string& method, const std::string& argsJson) const noexcept {
    try {
        const Json args = argsJson.empty() ? Json::object() : Json::parse(argsJson);
        return call(method, args).dump();
    } catch (const Json::exception& e) {
        return Json{{"error", std::string("bad JSON: ") + e.what()}}.dump();
    } catch (const std::exception& e) {
        return Json{{"error", e.what()}}.dump();
    } catch (...) {
        return R"({"error":"unknown error"})";
    }
}

std::vector<std::string> Api::methods() const {
    std::vector<std::string> names;
    names.reserve(handlers_.size());
    for (const auto& [name, handler] : handlers_) names.push_back(name);
    std::sort(names.begin(), names.end());
    return names;
}

}  // namespace forge
