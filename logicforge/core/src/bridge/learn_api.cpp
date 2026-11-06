// learn.* API: the app's progress, reviews, daily reel and skill plan.
#include <memory>
#include <stdexcept>

#include "forge/algo/graphs.hpp"
#include "forge/bridge/api.hpp"
#include "forge/learn/progress.hpp"

namespace forge {

namespace {

long today(const Json& args) {
    if (!args.contains("today") || !args["today"].is_number_integer()) throw std::invalid_argument("'today' (epoch day) is required");
    return args["today"].get<long>();
}

std::vector<learn::Track> tracks(const Json& args) {
    std::vector<learn::Track> out;
    if (args.contains("tracks"))
        for (const auto& t : args["tracks"]) out.push_back({t.value("id", ""), t.value("lessons", std::vector<std::string>{})});
    return out;
}

}  // namespace

void registerLearnApi(Api& api) {
    // One progress store for the app's lifetime, shared by every handler.
    // shared_ptr because several lambdas own it together.
    auto progress = std::make_shared<learn::Progress>();
    auto tracksCache = std::make_shared<std::vector<learn::Track>>();

    api.on("learn.init", [progress, tracksCache](const Json& a) -> Json {
        *tracksCache = tracks(a);
        progress->init(a.value("dir", ""), *tracksCache);
        return {{"ok", true}};
    });
    api.on("learn.state", [progress](const Json& a) -> Json { return progress->state(today(a)); });
    api.on("learn.completeLesson", [progress](const Json& a) -> Json {
        std::vector<learn::QuizAnswer> quiz;
        if (a.contains("quiz"))
            for (const auto& q : a["quiz"]) quiz.push_back({q.value("card", ""), q.value("correct", false)});
        return progress->completeLesson(a.value("lesson", ""), quiz, today(a));
    });
    api.on("learn.answer", [progress](const Json& a) -> Json {
        return progress->answerReview(a.value("card", ""), a.value("correct", false), today(a));
    });
    api.on("learn.reviews", [progress](const Json& a) -> Json {
        return progress->dueReviews(today(a), static_cast<std::size_t>(a.value("limit", 10)));
    });
    api.on("learn.reel", [progress](const Json& a) -> Json {
        return progress->topicReel(today(a), a.value("lessons", std::vector<std::string>{}));
    });
    api.on("learn.claimReel", [progress](const Json& a) -> Json { return progress->claimReel(today(a)); });
    api.on("learn.reset", [progress](const Json&) -> Json {
        progress->reset();
        return {{"ok", true}};
    });
    // Skill plan: lessons in an order that respects every prerequisite
    // (track order + explicit "requires"), via topological sort.
    api.on("learn.plan", [progress, tracksCache](const Json& a) -> Json {
        std::vector<std::string> nodes;
        std::vector<std::pair<std::string, std::string>> edges;
        for (const auto& t : *tracksCache) {
            for (std::size_t i = 0; i < t.lessons.size(); ++i) {
                nodes.push_back(t.lessons[i]);
                if (i > 0) edges.emplace_back(t.lessons[i - 1], t.lessons[i]);
            }
        }
        if (a.contains("requires"))
            for (const auto& [lesson, reqs] : a["requires"].items())
                for (const auto& r : reqs) edges.emplace_back(r.get<std::string>(), lesson);
        viz::Recorder discard;
        const auto order = algo::topoSort(nodes, edges, discard);
        if (order.empty() && !nodes.empty()) throw std::invalid_argument("lesson prerequisites contain a cycle");
        return {{"order", order}, {"next", progress->nextLesson(order)}};
    });
}

}  // namespace forge
