#include <doctest/doctest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>

#include "forge/bridge/api.hpp"
#include "forge/learn/progress.hpp"

using namespace forge;
using namespace forge::learn;

namespace {
std::filesystem::path tempDir(const char* name) {
    auto dir = std::filesystem::temp_directory_path() / (std::string("forge_test_") + name);
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);
    return dir;
}
}  // namespace

TEST_CASE("levels: 0, 100, 300, 600 XP") {
    CHECK(xpForLevel(1) == 0);
    CHECK(xpForLevel(2) == 100);
    CHECK(xpForLevel(4) == 600);
    CHECK(levelFor(99) == 1);
    CHECK(levelFor(100) == 2);
    CHECK(levelFor(299) == 2);
    CHECK(levelFor(300) == 3);
    CHECK(rankFor(1) == "Rookie");
    CHECK(rankFor(99) == "Grandmaster");
}

TEST_CASE("streaks: next day grows, same day holds, a gap resets") {
    Progress p;
    p.init("", {});
    p.completeLesson("a", {}, 100);
    CHECK(p.data().streak == 1);
    p.completeLesson("b", {}, 101);
    CHECK(p.data().streak == 2);
    p.completeLesson("c", {}, 101);
    CHECK(p.data().streak == 2);
    CHECK(p.state(102)["streak"] == 2);  // still alive the next day
    CHECK(p.state(103)["streak"] == 0);  // a whole day missed
    p.completeLesson("d", {}, 103);
    CHECK(p.data().streak == 1);
    CHECK(p.data().bestStreak == 2);
}

TEST_CASE("lesson XP: first completion vs replay") {
    Progress p;
    p.init("", {});
    const Json first = p.completeLesson("x", {{"x#1", true}, {"x#2", false}}, 10);
    CHECK(first["xpGained"] == 60);  // 50 + 10 for one right answer
    CHECK(first["firstTime"] == true);
    const Json again = p.completeLesson("x", {{"x#1", true}, {"x#2", true}}, 10);
    CHECK(again["xpGained"] == 14);  // 10 + 2 + 2: replays can't be farmed
    CHECK(p.data().lessons.at("x").bestScore == doctest::Approx(1.0));
}

TEST_CASE("Leitner boxes schedule reviews") {
    Progress p;
    p.init("", {});
    p.completeLesson("l", {{"l#1", true}, {"l#2", false}}, 0);
    CHECK(p.data().cards.at("l#1").box == 2);
    CHECK(p.data().cards.at("l#1").due == 2);
    CHECK(p.data().cards.at("l#2").box == 1);
    CHECK(p.data().cards.at("l#2").due == 1);
    p.answerReview("l#1", true, 2);
    CHECK(p.data().cards.at("l#1").box == 3);
    CHECK(p.data().cards.at("l#1").due == 6);
    p.answerReview("l#1", false, 6);
    CHECK(p.data().cards.at("l#1").box == 1);
    CHECK(p.data().cards.at("l#1").due == 7);
}

TEST_CASE("review queue: most overdue first, then the weakest box") {
    Progress p;
    p.init("", {});
    p.completeLesson("q", {{"q#a", true}, {"q#b", false}, {"q#c", false}}, 0);  // due 2, 1, 1 (boxes 2, 1, 1)
    p.answerReview("q#c", true, 1);  // c -> box 2, due 3
    const auto due = p.dueReviews(5, 10);
    REQUIRE(due.size() == 3);
    CHECK(due[0] == "q#b");  // due 1
    CHECK(due[1] == "q#a");  // due 2
    CHECK(due[2] == "q#c");  // due 3
    CHECK(p.dueReviews(5, 1).size() == 1);
    CHECK(p.dueReviews(0, 10).empty());
}

TEST_CASE("Topic Reel: fixed per day, centre row shows the pick, claim once") {
    Progress p;
    p.init("", {});
    const std::vector<std::string> lessons = {"a", "b", "c", "d"};
    const Json r1 = p.topicReel(500, lessons);
    const Json r2 = p.topicReel(500, lessons);
    CHECK(r1["lesson"] == r2["lesson"]);
    for (int k = 0; k < 3; ++k) {
        const auto& strip = r1["strips"][k];
        const auto stop = r1["stops"][k].get<std::size_t>();
        CHECK(strip[(stop + 1) % strip.size()] == r1["lesson"]);
    }
    CHECK_THROWS(p.claimReel(501));  // must spin that day first
    CHECK(p.claimReel(500)["xpGained"] == 20);
    CHECK(p.claimReel(500)["xpGained"] == 0);
}

TEST_CASE("achievements unlock once") {
    Progress p;
    p.init("", {{"pointers", {"p1", "p2"}}});
    const Json r = p.completeLesson("p1", {}, 1);
    REQUIRE(r["newAchievements"].size() == 1);
    CHECK(r["newAchievements"][0]["id"] == "first_lesson");
    const Json r2 = p.completeLesson("p2", {}, 1);
    bool tamer = false;
    for (const auto& a : r2["newAchievements"]) tamer = tamer || a["id"] == "track_pointers";
    CHECK(tamer);
    std::vector<QuizAnswer> ten;
    for (int i = 0; i < 10; ++i) ten.push_back({"z#" + std::to_string(i), true});
    const Json r3 = p.completeLesson("z", ten, 1);
    bool sharp = false;
    for (const auto& a : r3["newAchievements"]) sharp = sharp || a["id"] == "sharpshooter";
    CHECK(sharp);
}

TEST_CASE("progress survives a restart; a corrupt file is kept aside") {
    const auto dir = tempDir("persist");
    {
        Progress p;
        p.init(dir.string(), {});
        p.completeLesson("saved", {{"saved#1", true}}, 42);
    }
    {
        Progress p;
        p.init(dir.string(), {});
        CHECK(p.data().lessons.contains("saved"));
        CHECK(p.data().xp == 60);
        CHECK(p.data().cards.at("saved#1").box == 2);
    }
    {
        std::ofstream(dir / "progress.json") << "{ not json";
        Progress p;
        p.init(dir.string(), {});
        CHECK(p.data().xp == 0);
        CHECK(std::filesystem::exists(dir / "progress.json.corrupt"));
    }
    std::filesystem::remove_all(dir);
}

TEST_CASE("learn API: plan follows track order and prerequisites") {
    const Api api;
    const auto call = [&](const char* m, const Json& a) { return Json::parse(api.callJson(m, a.dump())); };
    call("learn.init", {{"dir", ""}, {"tracks", {{{"id", "t1"}, {"lessons", {"a", "b"}}}, {{"id", "t2"}, {"lessons", {"c"}}}}}});
    const Json plan = call("learn.plan", {{"requires", {{"a", {"c"}}}}});
    REQUIRE(plan["order"].size() == 3);
    CHECK(plan["order"][0] == "c");  // a requires c
    CHECK(plan["next"] == "c");
    call("learn.completeLesson", {{"lesson", "c"}, {"today", 1}});
    CHECK(call("learn.plan", {{"requires", {{"a", {"c"}}}}})["next"] == "a");
    CHECK(call("learn.plan", {{"requires", {{"a", {"b"}}}}}).contains("error"));  // b after a and a after b: cycle
    CHECK(call("learn.state", Json::object()).contains("error"));  // 'today' is required
}
