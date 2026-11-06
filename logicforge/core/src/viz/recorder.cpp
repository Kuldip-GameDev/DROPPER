#include "forge/viz/recorder.hpp"

namespace forge::viz {

void Recorder::frame(Json scene, std::string caption, std::string line) {
    if (frames_.size() >= kMaxFrames) {
        ++dropped_;
        return;
    }
    frames_.push_back(Frame{std::move(scene), std::move(caption), std::move(line)});
}

Json Recorder::toJson() const {
    Json frames = Json::array();
    for (const auto& f : frames_) {
        frames.push_back({{"scene", f.scene}, {"caption", f.caption}, {"line", f.line}});
    }
    return {{"frames", frames}, {"truncated", truncated()}, {"dropped", dropped_}};
}

}  // namespace forge::viz
