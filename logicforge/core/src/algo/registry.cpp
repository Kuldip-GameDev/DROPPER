#include "forge/algo/registry.hpp"

#include <stdexcept>

namespace forge::algo {

void Registry::add(AlgoInfo info, AlgoFn run) {
    if (find(info.id) != nullptr) {
        throw std::logic_error("duplicate algorithm id: " + info.id);
    }
    entries_.push_back(Entry{std::move(info), std::move(run)});
}

const Registry::Entry* Registry::find(std::string_view id) const {
    for (const auto& e : entries_) {
        if (e.info.id == id) return &e;
    }
    return nullptr;
}

const Registry& Registry::builtin() {
    // An "immediately invoked lambda" builds the registry exactly once.
    static const Registry instance = [] {
        Registry r;
        registerSorting(r);
        registerSearching(r);
        registerArrayPatterns(r);
        registerLinkedList(r);
        registerStack(r);
        memlab::registerPointerLab(r);
        memlab::registerLambdaLab(r);
        slot::registerSlotLab(r);
        return r;
    }();
    return instance;
}

Json toJson(const AlgoInfo& info) {
    Json fields = Json::array();
    for (const auto& f : info.fields) {
        fields.push_back({{"name", f.name}, {"kind", f.kind}, {"label", f.label}});
    }
    return {{"id", info.id},           {"title", info.title}, {"category", info.category},
            {"snippet", info.snippet}, {"time", info.time},   {"space", info.space},
            {"summary", info.summary}, {"fields", fields},    {"defaults", info.defaults}};
}

}  // namespace forge::algo
