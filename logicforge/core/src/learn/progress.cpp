#include "forge/learn/progress.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <queue>
#include <set>
#include <stdexcept>
#include <tuple>

#include "forge/slot/rng.hpp"

namespace forge::learn {

namespace {

bool trackDone(const Data& d, const std::vector<Track>& tracks, const std::string& id) {
    for (const auto& t : tracks) {
        if (t.id != id) continue;
        return !t.lessons.empty() && std::all_of(t.lessons.begin(), t.lessons.end(),
                                                 [&d](const std::string& l) { return d.lessons.contains(l); });
    }
    return false;
}

}  // namespace

// Achievements are data plus a lambda that decides when they're earned.
const std::vector<Achievement>& achievements() {
    static const std::vector<Achievement> list = {
        {"first_lesson", "First steps", "Complete your first lesson.",
         [](const Data& d, const auto&) { return d.lessons.size() >= 1; }},
        {"five_lessons", "On a roll", "Complete 5 lessons.",
         [](const Data& d, const auto&) { return d.lessons.size() >= 5; }},
        {"fifteen_lessons", "Committed", "Complete 15 lessons.",
         [](const Data& d, const auto&) { return d.lessons.size() >= 15; }},
        {"track_pointers", "Pointer tamer", "Finish the Pointer & Lambda Lab.",
         [](const Data& d, const auto& t) { return trackDone(d, t, "pointers"); }},
        {"track_slot", "Reel mathematician", "Finish the Slot Math Lab.",
         [](const Data& d, const auto& t) { return trackDone(d, t, "slot"); }},
        {"track_dsa", "Foundations laid", "Finish the Foundations track.",
         [](const Data& d, const auto& t) { return trackDone(d, t, "dsa"); }},
        {"streak_3", "Three in a row", "Learn on 3 days in a row.",
         [](const Data& d, const auto&) { return d.bestStreak >= 3; }},
        {"streak_7", "Week warrior", "Learn on 7 days in a row.",
         [](const Data& d, const auto&) { return d.bestStreak >= 7; }},
        {"streak_30", "Habit forged", "Learn on 30 days in a row.",
         [](const Data& d, const auto&) { return d.bestStreak >= 30; }},
        {"sharpshooter", "Sharpshooter", "Answer 10 questions right in a row.",
         [](const Data& d, const auto&) { return d.correctRun >= 10; }},
        {"reviewer", "Never forget", "Answer 25 review questions.",
         [](const Data& d, const auto&) { return d.reviewsAnswered >= 25; }},
        {"level_5", "Engineer", "Reach level 5.",
         [](const Data& d, const auto&) { return levelFor(d.xp) >= 5; }},
    };
    return list;
}

// Level L needs 50 * L * (L - 1) XP: 0, 100, 300, 600, 1000, 1500...
long xpForLevel(int level) { return 50L * level * (level - 1); }

int levelFor(long xp) {
    int level = 1;
    while (xpForLevel(level + 1) <= xp) ++level;
    return level;
}

std::string rankFor(int level) {
    static const char* ranks[] = {"Rookie",          "Apprentice", "Coder", "Debugger", "Engineer",
                                  "Senior Engineer", "Architect",  "Lead",  "Wizard",   "Grandmaster"};
    return ranks[std::clamp(level, 1, 10) - 1];
}

// ------------------------------------------------------------- persistence

Json toJson(const Data& d) {
    Json lessons = Json::object();
    for (const auto& [id, r] : d.lessons) lessons[id] = {{"completedDay", r.completedDay}, {"times", r.times}, {"bestScore", r.bestScore}};
    Json cards = Json::object();
    for (const auto& [id, c] : d.cards) cards[id] = {{"box", c.box}, {"due", c.due}, {"correct", c.correct}, {"wrong", c.wrong}};
    return {{"version", 1},
            {"xp", d.xp},
            {"streak", d.streak},
            {"bestStreak", d.bestStreak},
            {"lastActiveDay", d.lastActiveDay ? Json(*d.lastActiveDay) : Json(nullptr)},
            {"xpDay", d.xpDay},
            {"todayXp", d.todayXp},
            {"correctRun", d.correctRun},
            {"reviewsAnswered", d.reviewsAnswered},
            {"lessons", lessons},
            {"cards", cards},
            {"achievements", d.achievements},
            {"reel", {{"day", d.reelDay}, {"lesson", d.reelLesson}, {"claimed", d.reelClaimed}}}};
}

Data fromJson(const Json& j) {
    Data d;
    d.xp = j.value("xp", 0L);
    d.streak = j.value("streak", 0);
    d.bestStreak = j.value("bestStreak", 0);
    if (j.contains("lastActiveDay") && j["lastActiveDay"].is_number_integer()) d.lastActiveDay = j["lastActiveDay"].get<long>();
    d.xpDay = j.value("xpDay", 0L);
    d.todayXp = j.value("todayXp", 0L);
    d.correctRun = j.value("correctRun", 0);
    d.reviewsAnswered = j.value("reviewsAnswered", 0);
    if (j.contains("lessons"))
        for (const auto& [id, r] : j["lessons"].items())
            d.lessons[id] = {r.value("completedDay", 0L), r.value("times", 0), r.value("bestScore", 0.0)};
    if (j.contains("cards"))
        for (const auto& [id, c] : j["cards"].items())
            d.cards[id] = {std::clamp(c.value("box", 1), 1, kBoxes), c.value("due", 0L), c.value("correct", 0), c.value("wrong", 0)};
    if (j.contains("achievements"))
        for (const auto& [id, day] : j["achievements"].items()) d.achievements[id] = day.get<long>();
    if (j.contains("reel")) {
        d.reelDay = j["reel"].value("day", -1L);
        d.reelLesson = j["reel"].value("lesson", "");
        d.reelClaimed = j["reel"].value("claimed", false);
    }
    return d;
}

void Progress::init(const std::string& dir, std::vector<Track> tracks) {
    std::lock_guard lock(mutex_);
    tracks_ = std::move(tracks);
    data_ = Data{};
    path_ = dir.empty() ? "" : (std::filesystem::path(dir) / "progress.json").string();
    if (path_.empty() || !std::filesystem::exists(path_)) return;
    try {
        std::ifstream in(path_);
        data_ = fromJson(Json::parse(in));
    } catch (const std::exception&) {
        // Never lose a learner's progress silently: keep the broken file for inspection.
        std::error_code ec;
        std::filesystem::rename(path_, path_ + ".corrupt", ec);
        data_ = Data{};
    }
}

// Write to a temporary file, then rename it over the real one. A crash
// mid-write leaves the old file intact instead of half a file.
void Progress::save() const {
    if (path_.empty()) return;
    const std::string tmp = path_ + ".tmp";
    {
        std::ofstream out(tmp, std::ios::trunc);
        if (!out) throw std::runtime_error("cannot write progress file");
        out << toJson(data_).dump();
    }
    std::filesystem::rename(tmp, path_);
}

void Progress::reset() {
    std::lock_guard lock(mutex_);
    data_ = Data{};
    save();
}

// ----------------------------------------------------------------- rules

void Progress::touchStreak(long today) {
    if (!data_.lastActiveDay) {
        data_.streak = 1;
    } else if (today == *data_.lastActiveDay + 1) {
        ++data_.streak;  // came back the next day
    } else if (today > *data_.lastActiveDay + 1) {
        data_.streak = 1;  // missed at least one day
    }  // same day, or the clock went backwards: no change
    if (!data_.lastActiveDay || today > *data_.lastActiveDay) data_.lastActiveDay = today;
    data_.bestStreak = std::max(data_.bestStreak, data_.streak);
}

long Progress::award(long xp, long today) {
    touchStreak(today);
    if (data_.xpDay != today) {
        data_.xpDay = today;
        data_.todayXp = 0;
    }
    data_.todayXp += xp;
    data_.xp += xp;
    return xp;
}

std::vector<std::string> Progress::unlockNew(long today) {
    std::vector<std::string> fresh;
    for (const auto& a : achievements()) {
        if (!data_.achievements.contains(a.id) && a.earned(data_, tracks_)) {
            data_.achievements[a.id] = today;
            fresh.push_back(a.id);
        }
    }
    return fresh;
}

namespace {
void schedule(Card& c, bool correct, long today) {
    if (correct) {
        c.box = std::min(c.box + 1, kBoxes);  // know it: wait longer next time
        ++c.correct;
    } else {
        c.box = 1;  // forgot: back to the start
        ++c.wrong;
    }
    c.due = today + kIntervals[c.box];
}

Json achievementsJson(const std::vector<std::string>& ids) {
    Json out = Json::array();
    for (const auto& id : ids)
        for (const auto& a : achievements())
            if (a.id == id) out.push_back({{"id", a.id}, {"title", a.title}, {"description", a.description}});
    return out;
}
}  // namespace

Json Progress::completeLesson(const std::string& lessonId, const std::vector<QuizAnswer>& quiz, long today) {
    std::lock_guard lock(mutex_);
    const int levelBefore = levelFor(data_.xp);
    const bool firstTime = !data_.lessons.contains(lessonId);
    long gained = award(firstTime ? 50 : 10, today);
    int right = 0;
    for (const auto& a : quiz) {
        auto [it, isNew] = data_.cards.try_emplace(a.card);
        Card& c = it->second;
        if (isNew) {
            // First sight: right -> box 2 (see it again in 2 days); wrong -> box 1 (tomorrow).
            c.box = a.correct ? 2 : 1;
            c.due = today + kIntervals[c.box];
            (a.correct ? c.correct : c.wrong) += 1;
        } else {
            schedule(c, a.correct, today);
        }
        if (a.correct) {
            ++right;
            ++data_.correctRun;
            gained += award(firstTime ? 10 : 2, today);
        } else {
            data_.correctRun = 0;
        }
    }
    auto& rec = data_.lessons[lessonId];
    if (firstTime) rec.completedDay = today;
    ++rec.times;
    if (!quiz.empty()) rec.bestScore = std::max(rec.bestScore, static_cast<double>(right) / static_cast<double>(quiz.size()));
    const auto fresh = unlockNew(today);
    save();
    const int levelAfter = levelFor(data_.xp);
    return {{"xpGained", gained},
            {"firstTime", firstTime},
            {"correct", right},
            {"questions", quiz.size()},
            {"levelUp", levelAfter > levelBefore},
            {"newAchievements", achievementsJson(fresh)},
            {"state", stateLocked(today)}};
}

Json Progress::answerReview(const std::string& cardId, bool correct, long today) {
    std::lock_guard lock(mutex_);
    const int levelBefore = levelFor(data_.xp);
    Card& c = data_.cards[cardId];
    schedule(c, correct, today);
    ++data_.reviewsAnswered;
    data_.correctRun = correct ? data_.correctRun + 1 : 0;
    const long gained = award(correct ? 5 : 1, today);  // effort counts too
    const auto fresh = unlockNew(today);
    save();
    return {{"xpGained", gained},
            {"box", c.box},
            {"due", c.due},
            {"levelUp", levelFor(data_.xp) > levelBefore},
            {"newAchievements", achievementsJson(fresh)},
            {"state", stateLocked(today)}};
}

// [snippet:learn.due_reviews]
// The review queue is a priority queue (a heap): the most overdue card
// first; among equally overdue cards, the lowest box (weakest) first.
std::vector<std::string> Progress::dueReviews(long today, std::size_t limit) const {
    std::lock_guard lock(mutex_);
    using Item = std::tuple<long, int, std::string>;  // due day, box, card id
    auto later = [](const Item& a, const Item& b) { return a > b; };  // makes a MIN-heap
    std::priority_queue<Item, std::vector<Item>, decltype(later)> heap(later);
    for (const auto& [id, card] : data_.cards) {
        if (card.due <= today) heap.emplace(card.due, card.box, id);
    }
    std::vector<std::string> out;
    while (!heap.empty() && out.size() < limit) {
        out.push_back(std::get<2>(heap.top()));
        heap.pop();
    }
    return out;
}
// [/snippet]

// The Topic Reel: once a day, a reel picks today's review topic. The
// result is fixed for the whole day (no re-spinning), and the reward is
// a fixed 20 XP for doing the review. Same maths as the Slot Lab, no money.
Json Progress::topicReel(long today, const std::vector<std::string>& fallbackLessons) {
    std::lock_guard lock(mutex_);
    std::vector<std::string> candidates;
    for (const auto& [id, rec] : data_.lessons) candidates.push_back(id);
    if (candidates.size() < 3)
        for (const auto& l : fallbackLessons)
            if (std::find(candidates.begin(), candidates.end(), l) == candidates.end()) candidates.push_back(l);
    if (candidates.empty()) throw std::invalid_argument("no lessons to pick from");

    slot::Pcg32 rng(static_cast<std::uint64_t>(today), 0xF0C5);  // same day -> same spin
    std::string pick = candidates[slot::uniformBelow(rng, static_cast<std::uint32_t>(candidates.size()))];
    if (data_.reelDay == today && !data_.reelLesson.empty()) pick = data_.reelLesson;  // already spun today

    Json strips = Json::array(), stops = Json::array();
    for (int reel = 0; reel < 3; ++reel) {
        std::vector<std::string> strip = candidates;
        while (strip.size() < 6) strip.insert(strip.end(), candidates.begin(), candidates.end());
        // Fisher-Yates shuffle: swap each position with a random earlier-or-same one.
        for (std::size_t i = strip.size() - 1; i > 0; --i) {
            const auto j = slot::uniformBelow(rng, static_cast<std::uint32_t>(i + 1));
            std::swap(strip[i], strip[j]);
        }
        const auto at = static_cast<std::size_t>(std::find(strip.begin(), strip.end(), pick) - strip.begin());
        stops.push_back((at + strip.size() - 1) % strip.size());  // centre row shows the pick
        strips.push_back(strip);
    }
    if (data_.reelDay != today) {
        data_.reelDay = today;
        data_.reelLesson = pick;
        data_.reelClaimed = false;
        save();
    }
    return {{"lesson", pick}, {"strips", strips}, {"stops", stops}, {"claimed", data_.reelClaimed}};
}

Json Progress::claimReel(long today) {
    std::lock_guard lock(mutex_);
    if (data_.reelDay != today) throw std::invalid_argument("spin today's reel first");
    if (data_.reelClaimed) return {{"xpGained", 0}, {"alreadyClaimed", true}, {"state", stateLocked(today)}};
    data_.reelClaimed = true;
    const long gained = award(20, today);
    const auto fresh = unlockNew(today);
    save();
    return {{"xpGained", gained}, {"alreadyClaimed", false}, {"newAchievements", achievementsJson(fresh)}, {"state", stateLocked(today)}};
}

std::string Progress::nextLesson(const std::vector<std::string>& skillOrder) const {
    std::lock_guard lock(mutex_);
    for (const auto& id : skillOrder)
        if (!data_.lessons.contains(id)) return id;
    return "";
}

Json Progress::state(long today) const {
    std::lock_guard lock(mutex_);
    return stateLocked(today);
}

Json Progress::stateLocked(long today) const {
    const int level = levelFor(data_.xp);
    // A missed day breaks the streak; show it as 0 until the next activity resets it to 1.
    const bool alive = data_.lastActiveDay && today - *data_.lastActiveDay <= 1;
    Json completed = Json::array();
    for (const auto& [id, rec] : data_.lessons) completed.push_back(id);
    Json list = Json::array();
    for (const auto& a : achievements()) {
        auto it = data_.achievements.find(a.id);
        list.push_back({{"id", a.id}, {"title", a.title}, {"description", a.description},
                        {"unlocked", it != data_.achievements.end()},
                        {"day", it != data_.achievements.end() ? Json(it->second) : Json(nullptr)}});
    }
    std::size_t due = 0;
    for (const auto& [id, c] : data_.cards) due += c.due <= today ? 1 : 0;
    return {{"xp", data_.xp},
            {"level", level},
            {"rank", rankFor(level)},
            {"levelXp", xpForLevel(level)},
            {"nextLevelXp", xpForLevel(level + 1)},
            {"streak", alive ? data_.streak : 0},
            {"bestStreak", data_.bestStreak},
            {"todayXp", data_.xpDay == today ? data_.todayXp : 0},
            {"completed", completed},
            {"dueReviews", due},
            {"reviewsAnswered", data_.reviewsAnswered},
            {"achievements", list},
            {"reel", {{"spunToday", data_.reelDay == today}, {"lesson", data_.reelLesson}, {"claimed", data_.reelDay == today && data_.reelClaimed}}}};
}

}  // namespace forge::learn
