#pragma once
// The algorithm registry: id -> (description, function).
//
// Design note (Factory + Registry pattern):
//   The UI only knows algorithm *ids* ("sort.bubble"). The registry turns
//   an id into something runnable. New algorithms are added without the
//   UI or the bridge changing at all.
//
// Why explicit registerXxx(Registry&) functions instead of "magic"
// self-registering static objects? Because self-registration silently
// breaks in static libraries: the linker drops object files nobody
// references, and their registrations vanish. Explicit is boring and
// always works.
#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include "forge/json.hpp"
#include "forge/viz/recorder.hpp"

namespace forge::algo {

// One editable input field, so the UI can build an input form.
struct Field {
    std::string name;   // key in the input JSON, e.g. "array"
    std::string kind;   // "int_array" | "int" | "text"
    std::string label;  // shown to the learner, e.g. "Numbers"
};

struct AlgoInfo {
    std::string id;        // "sort.bubble"
    std::string title;     // "Bubble sort"
    std::string category;  // "Sorting"
    std::string snippet;   // snippet id shown next to the visual
    std::string time;      // "O(n²)"
    std::string space;     // "O(1)"
    std::string summary;   // one beginner-friendly sentence
    std::vector<Field> fields;
    Json defaults;         // default input, e.g. {"array": [5,1,4,2,8]}
};

// Runs the algorithm on `input`, records frames, returns the result.
using AlgoFn = std::function<Json(const Json& input, viz::Recorder& rec)>;

class Registry {
public:
    struct Entry {
        AlgoInfo info;
        AlgoFn run;
    };

    void add(AlgoInfo info, AlgoFn run);
    const Entry* find(std::string_view id) const;
    const std::vector<Entry>& all() const { return entries_; }

    // Every algorithm that ships with LogicForge. Built once, thread-safe
    // (C++11 guarantees a function-local static is initialised once).
    static const Registry& builtin();

private:
    std::vector<Entry> entries_;
};

Json toJson(const AlgoInfo& info);

// Each topic file provides one of these.
void registerSorting(Registry& r);
void registerSearching(Registry& r);
void registerArrayPatterns(Registry& r);
void registerLinkedList(Registry& r);
void registerStack(Registry& r);

}  // namespace forge::algo
