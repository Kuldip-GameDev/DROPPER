#pragma once
// English -> Diagram: a tiny, forgiving language for drawing data structures.
//
//   array 5 3 8 1          sorted array of 9 2 7
//   list 1 -> 2 -> 3       tree 8 3 10 1 6 14
//   heap 5 9 3 7           stack push 3 push 4 pop
//   queue 1 2 3 dequeue    graph A-B:4, B-C, A->C
#include <string>
#include <utility>
#include <vector>

#include "forge/json.hpp"
#include "forge/viz/recorder.hpp"

namespace forge::dsl {

struct Token {
    enum class Type { Word, Number, Arrow, Dash, Comma, Colon, End };
    Type type;
    std::string text;
};
const char* toString(Token::Type t);

std::vector<Token> lex(const std::string& input);

struct Command {
    std::string kind;    // array, list, tree, heap, stack, queue, graph
    bool sorted = false;
    std::vector<int> numbers;
    std::vector<std::pair<std::string, int>> ops;  // stack/queue: {"push", n} / {"pop", 0}
    struct Edge {
        std::string from, to;
        int weight = -1;
        bool directed = false;
    };
    std::vector<Edge> edges;
};

// Throws std::invalid_argument with a friendly hint on input it can't understand.
Command parse(const std::vector<Token>& tokens);

// lex + parse + draw. Returns {tokens, command, frames...}.
Json render(const std::string& input, viz::Recorder& rec);

}  // namespace forge::dsl
