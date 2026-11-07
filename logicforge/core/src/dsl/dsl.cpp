#include "forge/dsl/dsl.hpp"

#include <algorithm>
#include <cctype>
#include <set>
#include <stdexcept>

#include "forge/algo/trees_heaps.hpp"
#include "forge/bridge/api.hpp"
#include "forge/util/cat.hpp"
#include "forge/viz/scene.hpp"

namespace forge::dsl {

using viz::ArrayScene;
using viz::GraphScene;
using viz::Mark;
using viz::Recorder;

const char* toString(Token::Type t) {
    switch (t) {
        case Token::Type::Word:
            return "word";
        case Token::Type::Number:
            return "number";
        case Token::Type::Arrow:
            return "arrow";
        case Token::Type::Dash:
            return "dash";
        case Token::Type::Comma:
            return "comma";
        case Token::Type::Colon:
            return "colon";
        case Token::Type::End:
            return "end";
    }
    return "?";
}

namespace {
bool digit(char c) {
    return std::isdigit(static_cast<unsigned char>(c)) != 0;
}
// A '-' starts a negative number only at the start or after a space: "list -4" vs "A-4".
bool isNegativeNumber(const std::string& s, std::size_t i) {
    const bool spaceBefore = i == 0 || std::isspace(static_cast<unsigned char>(s[i - 1]));
    return spaceBefore && i + 1 < s.size() && digit(s[i + 1]);
}
}  // namespace

// [snippet:dsl.lexer]
// Step 1, the LEXER: turn characters into tokens (words, numbers, symbols).
// "list 1 -> 2" becomes  [word list] [number 1] [arrow] [number 2]
std::vector<Token> lex(const std::string& input) {
    std::vector<Token> tokens;
    std::size_t i = 0;
    while (i < input.size()) {
        const char c = input[i];
        if (std::isspace(static_cast<unsigned char>(c))) {
            ++i;
            continue;
        }
        if (c == '-' && i + 1 < input.size() && input[i + 1] == '>') {
            tokens.push_back(
                {Token::Type::Arrow, "->"});  // two characters, one token  // @line arrow
            i += 2;
        } else if (c == '-' && isNegativeNumber(input, i)) {
            std::size_t j = i + 1;  // "-4" after a space is a number; "A-4" is an edge
            while (j < input.size() && std::isdigit(static_cast<unsigned char>(input[j]))) ++j;
            tokens.push_back({Token::Type::Number, input.substr(i, j - i)});
            i = j;
        } else if (std::isdigit(static_cast<unsigned char>(c))) {
            std::size_t j = i;
            while (j < input.size() && std::isdigit(static_cast<unsigned char>(input[j]))) ++j;
            tokens.push_back({Token::Type::Number, input.substr(i, j - i)});  // @line number
            i = j;
        } else if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            std::size_t j = i;
            while (j < input.size() &&
                   (std::isalnum(static_cast<unsigned char>(input[j])) || input[j] == '_'))
                ++j;
            std::string word = input.substr(i, j - i);
            tokens.push_back({Token::Type::Word, word});  // @line word
            i = j;
        } else if (c == '-') {
            tokens.push_back({Token::Type::Dash, "-"});
            ++i;
        } else if (c == ',') {
            tokens.push_back({Token::Type::Comma, ","});
            ++i;
        } else if (c == ':') {
            tokens.push_back({Token::Type::Colon, ":"});
            ++i;
        } else
            throw std::invalid_argument(cat("I don't understand the character '", c, "'"));
    }
    tokens.push_back({Token::Type::End, ""});
    return tokens;
}
// [/snippet]

namespace {

std::string lower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

const std::set<std::string> kFiller = {
    "a",        "an",    "the",   "of",   "with", "numbers", "values", "and",        "then",
    "please",   "show",  "me",    "draw", "make", "create",  "build",  "containing", "items",
    "elements", "nodes", "edges", "from", "to",   "binary",  "search", "linked"};

const char* kHint =
    "Try: array 5 3 8, sorted array of 9 2 7, list 1 -> 2 -> 3, tree 8 3 10 1 6, heap 5 9 3, "
    "stack push 3 push 4 pop, queue 1 2 dequeue, graph A-B:4, B-C";

int toInt(const std::string& s) {
    if (s.size() > 5) throw std::invalid_argument("numbers must be between -9999 and 9999");
    return std::stoi(s);
}

// [snippet:dsl.parser]
// Step 2, the PARSER: read tokens left to right and decide what they mean.
// This is "recursive descent": one small function per part of the grammar.
class Parser {
   public:
    explicit Parser(const std::vector<Token>& t) : t_(t) {}

    Command command() {
        Command cmd;
        skipFiller();
        if (isWord("sorted")) {
            cmd.sorted = true;
            ++pos_;
            skipFiller();
        }
        cmd.kind = kind();  // what are we drawing?  // @line kind
        if (cmd.kind == "stack" || cmd.kind == "queue")
            cmd.ops = operations();
        else if (cmd.kind == "graph")
            cmd.edges = edges();
        else
            cmd.numbers = numbers();  // @line numbers
        if (t_[pos_].type != Token::Type::End)
            throw std::invalid_argument(
                cat("I didn't expect '", t_[pos_].text, "' there. ", kHint));
        return cmd;
    }

   private:
    std::string kind() {
        if (t_[pos_].type != Token::Type::Word)
            throw std::invalid_argument(cat("Start with what to draw. ", kHint));
        const std::string w = lower(t_[pos_++].text);
        if (w == "array" || w == "arr" || w == "list" || w == "tree" || w == "bst" ||
            w == "heap" || w == "stack" || w == "queue" || w == "graph") {
            return w == "arr" ? "array" : w == "bst" ? "tree" : w;
        }
        throw std::invalid_argument(cat("I don't know how to draw '", w, "'. ", kHint));
    }

    std::vector<int> numbers() {  // 5 3 8, "5, 3 and 8", 1 -> 2 -> 3
        std::vector<int> out;
        while (t_[pos_].type != Token::Type::End) {
            skipFiller();
            const Token& tok = t_[pos_];
            if (tok.type == Token::Type::Number)
                out.push_back(toInt(tok.text));
            else if (tok.type != Token::Type::Comma && tok.type != Token::Type::Arrow)
                break;
            ++pos_;
        }
        return out;
    }

    std::vector<std::pair<std::string, int>> operations() {  // push 3 push 4 pop, or just 3 4
        std::vector<std::pair<std::string, int>> ops;
        while (t_[pos_].type != Token::Type::End) {
            skipFiller();
            const Token& tok = t_[pos_];
            if (tok.type == Token::Type::Number) {
                ops.emplace_back("push", toInt(tok.text));
                ++pos_;
            } else if (isWord("push") || isWord("enqueue") || isWord("add")) {
                ++pos_;
                if (t_[pos_].type != Token::Type::Number)
                    throw std::invalid_argument("push needs a number after it, like: push 5");
                ops.emplace_back("push", toInt(t_[pos_++].text));
            } else if (isWord("pop") || isWord("dequeue") || isWord("remove")) {
                ops.emplace_back("pop", 0);
                ++pos_;
            } else if (tok.type == Token::Type::Comma)
                ++pos_;
            else
                break;
        }
        return ops;
    }

    std::vector<Command::Edge> edges() {  // A-B:4, B->C
        std::vector<Command::Edge> out;
        while (t_[pos_].type != Token::Type::End) {
            if (t_[pos_].type == Token::Type::Comma) {
                ++pos_;
                continue;
            }
            Command::Edge e;
            e.from = name();
            if (t_[pos_].type == Token::Type::Arrow)
                e.directed = true;
            else if (t_[pos_].type != Token::Type::Dash)
                throw std::invalid_argument(
                    "write edges like A-B or A->B, separated by commas");
            ++pos_;
            e.to = name();
            if (t_[pos_].type == Token::Type::Colon) {
                ++pos_;
                if (t_[pos_].type != Token::Type::Number)
                    throw std::invalid_argument("a weight goes after ':' like A-B:4");
                e.weight = toInt(t_[pos_++].text);
            }
            out.push_back(e);
        }
        return out;
    }

    std::string name() {
        const Token& tok = t_[pos_];
        if (tok.type != Token::Type::Word && tok.type != Token::Type::Number)
            throw std::invalid_argument("expected a node name, like A");
        if (tok.text.size() > 6)
            throw std::invalid_argument("node names can be at most 6 characters");
        ++pos_;
        return tok.text;
    }

    bool isWord(const char* w) const {
        return t_[pos_].type == Token::Type::Word && lower(t_[pos_].text) == w;
    }
    void skipFiller() {
        while (t_[pos_].type == Token::Type::Word && kFiller.contains(lower(t_[pos_].text)))
            ++pos_;  // ignore "a", "of"...
    }

    const std::vector<Token>& t_;
    std::size_t pos_ = 0;
};
// [/snippet]

}  // namespace

Command parse(const std::vector<Token>& tokens) {
    return Parser(tokens).command();
}

namespace {

void drawCommand(const Command& c, Recorder& rec) {
    constexpr std::size_t kMax = 16;
    if (c.numbers.size() > kMax || c.ops.size() > kMax * 2 || c.edges.size() > 24)
        throw std::invalid_argument(
            "that's a bit big for a phone screen: use at most 16 values");
    if (c.kind == "array") {
        auto v = c.numbers;
        if (v.empty())
            throw std::invalid_argument("an array needs some numbers, like: array 5 3 8");
        if (c.sorted) {
            rec.frame(ArrayScene(v), "Your numbers, as typed.", "numbers");
            std::sort(v.begin(), v.end());
            rec.frame(ArrayScene(v).markRange(0, static_cast<int>(v.size()), Mark::Sorted),
                      "Sorted, as you asked. Now binary search would work on it.", "kind");
        } else {
            rec.frame(ArrayScene(v),
                      cat("An array of ", v.size(),
                          " numbers, side by side in memory. Index 0 is first."),
                      "numbers");
        }
    } else if (c.kind == "list") {
        if (c.numbers.empty())
            throw std::invalid_argument("a list needs some numbers, like: list 1 -> 2 -> 3");
        GraphScene g;
        for (std::size_t i = 0; i < c.numbers.size(); ++i) {
            g.node("n" + std::to_string(i), std::to_string(c.numbers[i]));
            if (i > 0) g.edge("n" + std::to_string(i - 1), "n" + std::to_string(i));
        }
        g.sub("n" + std::to_string(c.numbers.size() - 1), "next = null")
            .pointer("head", "n0")
            .layoutRow();
        rec.frame(g,
                  "A singly linked list: each node points to the next. The last one points to "
                  "nullptr.",
                  "numbers");
    } else if (c.kind == "tree") {
        if (c.numbers.empty())
            throw std::invalid_argument("a tree needs some numbers, like: tree 8 3 10");
        std::vector<int> sofar;
        for (int v : c.numbers) {
            sofar.push_back(v);
            const auto root = algo::buildBstFrom(sofar);
            rec.frame(
                algo::drawTree(root.get()),
                cat("Insert ", v,
                    " into the binary search tree: smaller goes left, bigger goes right."),
                "numbers");
        }
    } else if (c.kind == "heap") {
        if (c.numbers.empty())
            throw std::invalid_argument("a heap needs some numbers, like: heap 5 9 3");
        std::vector<int> h;
        for (int v : c.numbers) {
            algo::heapPush(h, v);
            rec.frame(
                algo::drawHeap(h),
                cat("Push ", v, " into the min-heap. The smallest value is always on top."),
                "numbers");
        }
    } else if (c.kind == "stack" || c.kind == "queue") {
        if (c.ops.empty())
            throw std::invalid_argument(
                cat("a ", c.kind, " needs operations, like: ", c.kind, " push 3 push 4 pop"));
        std::vector<int> items;
        auto scene = [&](int hot) {
            std::vector<std::string> labels;
            for (int v : items) labels.push_back(std::to_string(v));
            auto s = ArrayScene::ofLabels(labels).title(c.kind);
            if (c.kind == "stack")
                s.layout(ArrayScene::Layout::Stack);
            else if (!items.empty())
                s.pointer("front", 0).pointer("back", static_cast<int>(items.size()) - 1);
            if (hot >= 0) s.mark(hot, Mark::Active);
            return s;
        };
        rec.frame(scene(-1),
                  c.kind == "stack" ? "An empty stack: last in, first out."
                                    : "An empty queue: first in, first out.",
                  "kind");
        for (const auto& [op, v] : c.ops) {
            if (op == "push") {
                items.push_back(v);
                rec.frame(scene(static_cast<int>(items.size()) - 1),
                          c.kind == "stack" ? cat("push ", v, ": it goes on top.")
                                            : cat("enqueue ", v, ": it joins the back."),
                          "kind");
            } else if (items.empty()) {
                rec.frame(scene(-1).var("error", "empty"),
                          cat("Can't ", c.kind == "stack" ? "pop" : "dequeue", ": the ", c.kind,
                              " is empty. In real code that's a bug to check for."),
                          "kind");
            } else if (c.kind == "stack") {
                const int top = items.back();
                items.pop_back();
                rec.frame(scene(-1).var("popped", top),
                          cat("pop: ", top, " comes off the top (the last one in)."), "kind");
            } else {
                const int front = items.front();
                items.erase(items.begin());
                rec.frame(scene(-1).var("dequeued", front),
                          cat("dequeue: ", front, " leaves from the front (the first one in)."),
                          "kind");
            }
        }
    } else if (c.kind == "graph") {
        if (c.edges.empty())
            throw std::invalid_argument("a graph needs edges, like: graph A-B, B-C");
        GraphScene g;
        for (const auto& e : c.edges) {
            if (!g.hasNode(e.from)) g.node(e.from, e.from);
            if (!g.hasNode(e.to)) g.node(e.to, e.to);
            g.edge(e.from, e.to, e.weight >= 0 ? std::to_string(e.weight) : "", e.directed);
        }
        g.layoutCircle();
        const bool directed = std::any_of(c.edges.begin(), c.edges.end(),
                                          [](const auto& e) { return e.directed; });
        rec.frame(g,
                  cat("A graph with ", c.edges.size(), " edge(s). ",
                      directed ? "Arrows (->) are one-way." : "Dashes (-) are two-way.",
                      " Numbers after ':' are weights."),
                  "kind");
    }
}

}  // namespace

Json render(const std::string& input, Recorder& rec) {
    if (input.size() > 200) throw std::invalid_argument("please keep it under 200 characters");
    const auto tokens = lex(input);
    const Command cmd = parse(tokens);
    drawCommand(cmd, rec);
    Json toks = Json::array();
    for (const auto& t : tokens)
        if (t.type != Token::Type::End)
            toks.push_back({{"type", toString(t.type)}, {"text", t.text}});
    Json edges = Json::array();
    for (const auto& e : cmd.edges)
        edges.push_back(
            {{"from", e.from}, {"to", e.to}, {"weight", e.weight}, {"directed", e.directed}});
    Json ops = Json::array();
    for (const auto& [op, v] : cmd.ops) ops.push_back({{"op", op}, {"value", v}});
    return {{"tokens", toks},
            {"command",
             {{"kind", cmd.kind},
              {"sorted", cmd.sorted},
              {"numbers", cmd.numbers},
              {"ops", ops},
              {"edges", edges}}}};
}

}  // namespace forge::dsl

namespace forge {

void registerDslApi(Api& api) {
    // Same output shape as algo.trace, plus how the text was understood.
    api.on("dsl.render", [](const Json& args) -> Json {
        viz::Recorder rec;
        Json parsed = dsl::render(args.value("text", ""), rec);
        Json out = rec.toJson();
        out["id"] = "dsl";
        out["result"] = std::move(parsed);
        return out;
    });
}

}  // namespace forge
