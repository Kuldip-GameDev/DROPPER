#pragma once
// The learning engine: XP, levels, streaks, spaced repetition,
// achievements and the daily Topic Reel. Pure C++ with one file of
// state, so it works the same on a phone and in tests.
//
// Days are passed in as integers ("days since 1970-01-01" in the
// learner's local time). The engine never reads the clock itself, which
// keeps it deterministic and testable.
#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "forge/json.hpp"

namespace forge::learn {

struct Track {
    std::string id;
    std::vector<std::string> lessons;
};

// Leitner box system: answer right -> next box (longer wait); wrong -> box 1.
inline constexpr int kBoxes = 5;
inline constexpr int kIntervals[kBoxes + 1] = {0, 1, 2, 4, 8, 16};  // days, indexed by box 1..5

struct Card {
    int box = 1;
    long due = 0;
    int correct = 0;
    int wrong = 0;
};

struct LessonRecord {
    long completedDay = 0;
    int times = 0;
    double bestScore = 0;  // fraction of quiz questions right first try
};

struct Data {
    long xp = 0;
    int streak = 0;
    int bestStreak = 0;
    std::optional<long> lastActiveDay;
    long xpDay = 0;      // the day todayXp belongs to
    long todayXp = 0;
    int correctRun = 0;  // answers right in a row
    int reviewsAnswered = 0;
    std::map<std::string, LessonRecord> lessons;
    std::map<std::string, Card> cards;  // "lessonId#cardIndex"
    std::map<std::string, long> achievements;  // id -> day unlocked
    long reelDay = -1;
    std::string reelLesson;
    bool reelClaimed = false;
};

struct Achievement {
    std::string id;
    std::string title;
    std::string description;
    std::function<bool(const Data&, const std::vector<Track>&)> earned;
};
const std::vector<Achievement>& achievements();

int levelFor(long xp);              // 1-based
long xpForLevel(int level);         // xp needed to reach `level`
std::string rankFor(int level);

struct QuizAnswer {
    std::string card;  // "lessonId#cardIndex"
    bool correct = false;
};

class Progress {
public:
    // Empty dir = in-memory only (tests).
    void init(const std::string& dir, std::vector<Track> tracks);

    Json state(long today) const;
    Json completeLesson(const std::string& lessonId, const std::vector<QuizAnswer>& quiz, long today);
    Json answerReview(const std::string& cardId, bool correct, long today);
    std::vector<std::string> dueReviews(long today, std::size_t limit) const;
    Json topicReel(long today, const std::vector<std::string>& fallbackLessons);
    Json claimReel(long today);
    // Recommended next lesson: first lesson (in skill order) not completed.
    std::string nextLesson(const std::vector<std::string>& skillOrder) const;
    void reset();

    const Data& data() const { return data_; }

private:
    long award(long xp, long today);  // returns xp actually given
    void touchStreak(long today);
    std::vector<std::string> unlockNew(long today);
    void save() const;
    Json stateLocked(long today) const;

    mutable std::mutex mutex_;
    std::string path_;
    std::vector<Track> tracks_;
    Data data_;
};

Json toJson(const Data& d);
Data fromJson(const Json& j);

}  // namespace forge::learn
