#include "forge/patterns/patterns.hpp"

#include <algorithm>

namespace forge::patterns {

// [snippet:pattern.command]
// Each action is an OBJECT that knows how to do itself and undo itself.
class MoveCommand : public Command {
public:
    MoveCommand(int dx, int dy) : dx_(dx), dy_(dy) {}
    void execute(Board& b) override {
        fromX_ = b.x;  // remember where we were, for undo
        fromY_ = b.y;
        b.x = std::clamp(b.x + dx_, 0, b.width - 1);
        b.y = std::clamp(b.y + dy_, 0, b.height - 1);
    }
    void undo(Board& b) override {
        b.x = fromX_;
        b.y = fromY_;
    }
    std::string name() const override {
        return dx_ > 0 ? "right" : dx_ < 0 ? "left" : dy_ > 0 ? "down" : "up";
    }

private:
    int dx_, dy_;
    int fromX_ = 0, fromY_ = 0;
};

// Two stacks give you undo AND redo.
void History::run(std::unique_ptr<Command> cmd, Board& b) {
    cmd->execute(b);
    done_.push_back(std::move(cmd));  // @line run
    undone_.clear();  // a new action makes the old "future" invalid
}

bool History::undo(Board& b) {
    if (done_.empty()) return false;
    done_.back()->undo(b);  // @line undo
    undone_.push_back(std::move(done_.back()));
    done_.pop_back();
    return true;
}

bool History::redo(Board& b) {
    if (undone_.empty()) return false;
    undone_.back()->execute(b);  // @line redo
    done_.push_back(std::move(undone_.back()));
    undone_.pop_back();
    return true;
}
// [/snippet]

std::unique_ptr<Command> makeMove(int dx, int dy) { return std::make_unique<MoveCommand>(dx, dy); }

std::vector<std::string> History::undoNames() const {
    std::vector<std::string> out;
    for (const auto& c : done_) out.push_back(c->name());
    return out;
}

std::vector<std::string> History::redoNames() const {
    std::vector<std::string> out;
    for (const auto& c : undone_) out.push_back(c->name());
    return out;
}

}  // namespace forge::patterns
