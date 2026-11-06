// forge_cli: run LogicForge from a terminal.
//
//   forge_cli list [--json]                  every algorithm the core knows
//   forge_cli trace <id> [numbers...] [--target N] [--k N] [--text S] [--json]
//   forge_cli call <method> ['<json args>']  raw access to the same API the app uses
//   forge_cli dump-traces                    default trace of every algorithm (used by CI)
//
// It talks to the core ONLY through forge::Api, exactly like the Android
// app does. If something works here, it works on the phone.
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "forge/bridge/api.hpp"

namespace {

using forge::Json;

const char* colourFor(const std::string& mark) {
    if (mark == "compare") return "\033[1;33m";                  // yellow
    if (mark == "swap") return "\033[1;35m";                     // magenta
    if (mark == "sorted" || mark == "ok") return "\033[32m";      // green
    if (mark == "found") return "\033[1;32m";                    // bright green
    if (mark == "active" || mark == "pivot") return "\033[1;36m";// cyan
    if (mark == "dim" || mark == "visited") return "\033[2m";     // faint
    if (mark == "error") return "\033[1;31m";                    // red
    if (mark == "window") return "\033[4m";                      // underline
    return "";
}

std::string markOf(const Json& marks, std::size_t i) {
    const auto key = std::to_string(i);
    return marks.contains(key) ? marks[key].get<std::string>() : "";
}

void printVars(const Json& scene) {
    if (!scene.contains("vars") || scene["vars"].empty()) return;
    std::cout << "    ";
    for (const auto& v : scene["vars"]) std::cout << v["name"].get<std::string>() << "=" << v["value"].dump() << "  ";
    std::cout << "\n";
}

void printArray(const Json& s) {
    const auto& labels = s["labels"];
    const auto& marks = s["marks"];
    std::cout << "    ";
    for (std::size_t i = 0; i < labels.size(); ++i) {
        const auto m = markOf(marks, i);
        std::cout << colourFor(m) << "[" << labels[i].get<std::string>() << "]" << "\033[0m ";
    }
    std::cout << "\n";
    for (const auto& p : s["pointers"]) {
        std::cout << "      " << p["name"].get<std::string>() << " -> " << p["index"] << "\n";
    }
    printVars(s);
}

void printGraph(const Json& s) {
    std::cout << "    nodes: ";
    for (const auto& n : s["nodes"]) {
        std::cout << colourFor(n["mark"].get<std::string>()) << n["id"].get<std::string>() << "("
                  << n["label"].get<std::string>() << ")\033[0m ";
    }
    std::cout << "\n    edges: ";
    for (const auto& e : s["edges"]) {
        std::cout << e["from"].get<std::string>() << "->" << e["to"].get<std::string>() << " ";
    }
    std::cout << "\n";
    for (const auto& p : s["pointers"]) {
        std::cout << "      " << p["name"].get<std::string>() << " -> " << p["node"].get<std::string>() << "\n";
    }
    printVars(s);
}

void printGrid(const Json& s) {
    const int rows = s["rows"], cols = s["cols"];
    for (int r = 0; r < rows; ++r) {
        std::cout << "    ";
        if (!s["rowHeaders"].empty()) std::cout << s["rowHeaders"][r].get<std::string>() << "\t";
        for (int c = 0; c < cols; ++c) {
            const auto i = static_cast<std::size_t>(r * cols + c);
            std::cout << colourFor(markOf(s["marks"], i)) << s["cells"][i].get<std::string>() << "\033[0m\t";
        }
        std::cout << "\n";
    }
    printVars(s);
}

void printScene(const Json& s) {
    const auto kind = s.value("kind", "");
    if (kind == "array") printArray(s);
    else if (kind == "graph") printGraph(s);
    else if (kind == "grid") printGrid(s);
    else std::cout << "    " << s.dump() << "\n";
}

int usage() {
    std::cerr << "usage:\n"
                 "  forge_cli list [--json]\n"
                 "  forge_cli trace <id> [numbers...] [--target N] [--k N] [--text S] [--json]\n"
                 "  forge_cli call <method> ['<json args>']\n"
                 "  forge_cli dump-traces\n";
    return 2;
}

Json orFail(const Json& j) {
    if (j.is_object() && j.contains("error")) {
        std::cerr << "error: " << j["error"].get<std::string>() << "\n";
        std::exit(1);
    }
    return j;
}

}  // namespace

int main(int argc, char** argv) {
    const std::vector<std::string> args(argv + 1, argv + argc);
    if (args.empty()) return usage();
    const forge::Api api;
    const std::string& cmd = args[0];
    auto has = [&](const std::string& flag) {
        for (const auto& a : args) if (a == flag) return true;
        return false;
    };

    if (cmd == "list") {
        const Json list = orFail(Json::parse(api.callJson("algo.list", "{}")));
        if (has("--json")) {
            std::cout << list.dump(2) << "\n";
            return 0;
        }
        for (const auto& a : list) {
            std::cout << a["id"].get<std::string>() << "\t" << a["title"].get<std::string>() << "\t"
                      << a["time"].get<std::string>() << "\n";
        }
        return 0;
    }

    if (cmd == "call") {
        if (args.size() < 2) return usage();
        std::cout << Json::parse(api.callJson(args[1], args.size() > 2 ? args[2] : "{}")).dump(2) << "\n";
        return 0;
    }

    if (cmd == "dump-traces") {
        Json all = Json::object();
        for (const auto& a : orFail(Json::parse(api.callJson("algo.list", "{}")))) {
            const auto id = a["id"].get<std::string>();
            all[id] = orFail(Json::parse(api.callJson("algo.trace", Json{{"id", id}}.dump())));
        }
        std::cout << all.dump() << "\n";
        return 0;
    }

    if (cmd == "trace") {
        if (args.size() < 2) return usage();
        Json input = Json::object();
        Json numbers = Json::array();
        for (std::size_t i = 2; i < args.size(); ++i) {
            const auto& a = args[i];
            if ((a == "--target" || a == "--k" || a == "--cycle" || a == "--l" || a == "--r") && i + 1 < args.size()) {
                const std::string key = a == "--cycle" ? "cycleTo" : a.substr(2);
                input[key] = std::stoi(args[++i]);
            } else if (a == "--text" && i + 1 < args.size()) {
                input["text"] = args[++i];
            } else if (a != "--json") {
                numbers.push_back(std::stoi(a));
            }
        }
        Json req = {{"id", args[1]}};
        if (!numbers.empty() || !input.empty()) {
            // Start from the defaults so unspecified fields still work.
            for (const auto& a : orFail(Json::parse(api.callJson("algo.list", "{}")))) {
                if (a["id"] == args[1]) req["input"] = a["defaults"];
            }
            if (!numbers.empty()) req["input"]["array"] = numbers;
            for (auto& [k, v] : input.items()) req["input"][k] = v;
        }
        const Json trace = orFail(Json::parse(api.callJson("algo.trace", req.dump())));
        if (has("--json")) {
            std::cout << trace.dump(2) << "\n";
            return 0;
        }
        int step = 0;
        for (const auto& f : trace["frames"]) {
            std::cout << "\033[1mStep " << ++step << "\033[0m  " << f["caption"].get<std::string>();
            if (!f["line"].get<std::string>().empty()) std::cout << "   \033[2m(line: " << f["line"].get<std::string>() << ")\033[0m";
            std::cout << "\n";
            printScene(f["scene"]);
        }
        if (trace["truncated"].get<bool>()) std::cout << "(trace truncated)\n";
        std::cout << "result: " << trace["result"].dump() << "\n";
        return 0;
    }

    return usage();
}
