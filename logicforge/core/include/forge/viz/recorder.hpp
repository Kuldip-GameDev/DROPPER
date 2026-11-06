#pragma once
// The Recorder collects Frames while an algorithm runs.
//
// A Frame is a COMPLETE snapshot (scene + caption + code line), not a
// diff from the previous frame. That is the "Memento" design pattern:
// going one step back is just frames[i - 1]. No undo logic needed.
//
// The price is memory, so a trace is capped at kMaxFrames.
#include <cstddef>
#include <string>
#include <vector>

#include "forge/json.hpp"

namespace forge::viz {

struct Frame {
    Json scene;
    std::string caption;  // plain-English explanation of this step
    std::string line;     // label of the code line, see "// @line" in sources
};

class Recorder {
public:
    static constexpr std::size_t kMaxFrames = 2000;

    void frame(Json scene, std::string caption, std::string line = "");

    // Accept any scene type that has toJson(). This is a template, so
    // ArrayScene, GraphScene, GridScene... all work without a base class.
    template <typename Scene>
        requires requires(const Scene& s) { s.toJson(); }
    void frame(const Scene& scene, std::string caption, std::string line = "") {
        // Build the JSON only if we are going to keep the frame.
        if (frames_.size() >= kMaxFrames) {
            ++dropped_;
            return;
        }
        frame(scene.toJson(), std::move(caption), std::move(line));
    }

    const std::vector<Frame>& frames() const { return frames_; }
    bool truncated() const { return dropped_ > 0; }
    std::size_t dropped() const { return dropped_; }

    Json toJson() const;

private:
    std::vector<Frame> frames_;
    std::size_t dropped_ = 0;
};

}  // namespace forge::viz
