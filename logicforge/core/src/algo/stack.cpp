#include "forge/algo/stack.hpp"

#include <vector>

#include "forge/algo/input.hpp"
#include "forge/algo/registry.hpp"
#include "forge/util/cat.hpp"
#include "forge/viz/scene.hpp"

namespace forge::algo {

using viz::ArrayScene;
using viz::Mark;
using viz::Recorder;

namespace {
ArrayScene stackScene(const std::vector<char>& stack, const std::string& text, std::size_t at) {
    std::vector<std::string> labels;
    for (char c : stack) labels.emplace_back(1, c);
    auto s = ArrayScene::ofLabels(labels).layout(ArrayScene::Layout::Stack).title("stack");
    if (!stack.empty()) s.mark(static_cast<int>(stack.size()) - 1, Mark::Active);
    std::string shown = text;
    if (at < text.size()) shown = text.substr(0, at) + "[" + text[at] + "]" + text.substr(at + 1);
    return s.var("input", shown);
}
}  // namespace

// [snippet:stack.balanced]
bool isBalanced(const std::string& text, Recorder& rec) {
    std::vector<char> stack;  // a vector makes a perfect stack: push_back / back / pop_back
    for (std::size_t i = 0; i < text.size(); ++i) {
        char c = text[i];
        if (c == '(' || c == '[' || c == '{') {
            stack.push_back(c);  // remember the opener  // @line push
            rec.frame(stackScene(stack, text, i), cat("'", c, "' opens something. Push it on the stack."), "push"); // @viz
        } else if (c == ')' || c == ']' || c == '}') {
            rec.frame(stackScene(stack, text, i).mark(static_cast<int>(stack.size()) - 1, Mark::Compare), cat("'", c, "' closes something. It must match the opener on top."), stack.empty() ? "empty" : "pop"); // @viz
            if (stack.empty()) return false;  // a closer with nothing open  // @line empty
            char open = stack.back();
            stack.pop_back();  // @line pop
            bool match = (open == '(' && c == ')') || (open == '[' && c == ']') ||
                         (open == '{' && c == '}');
            rec.frame(stackScene(stack, text, i), match ? cat("'", open, "' matches '", c, "'. Pop it.") : cat("'", open, "' does not match '", c, "'. Not balanced!"), "mismatch"); // @viz
            if (!match) return false;  // @line mismatch
        }
    }
    rec.frame(stackScene(stack, text, text.size()), stack.empty() ? "End of input and the stack is empty: balanced!" : "End of input but openers are left on the stack: not balanced.", "end"); // @viz
    return stack.empty();  // leftovers mean something was never closed  // @line end
}
// [/snippet]

void registerStack(Registry& r) {
    r.add({"stack.balanced", "Balanced brackets", "Stacks & queues", "stack.balanced", "O(n)", "O(n)",
           "Push every opener. Every closer must match the opener on top of the stack.",
           {{"text", "text", "Brackets"}},
           {{"text", "{[()()]}"}}},
          [](const Json& input, Recorder& rec) -> Json {
              const auto text = textValue(input, "text", 40);
              return {{"balanced", isBalanced(text, rec)}};
          });
}

}  // namespace forge::algo
