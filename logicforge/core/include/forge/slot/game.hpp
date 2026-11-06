#pragma once
// Reel games for the Slot Math Lab. Everything here is plain, testable
// logic. The visual traces live in src/slot/slot_viz.cpp.
//
// Play credits only: this teaches the maths and engineering of slot
// games. It is not a gambling product.
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace forge::slot {

struct Symbol {
    std::string id;     // "CHERRY"
    std::string label;  // what the player sees, e.g. "🍒"
};

struct ReelGame {
    std::string name;
    std::vector<Symbol> symbols;
    std::vector<std::vector<int>> reels;  // one strip of symbol indices per reel (circular)
    int rows = 3;                         // visible rows
    // paytable[symbol][count] = pay for `count` matching symbols from the left, per 1 credit bet.
    std::vector<std::vector<int>> paytable;
    int wild = -1;  // substitutes for any symbol; -1 = none
};

// screen[row][reel]
using Screen = std::vector<std::vector<int>>;

Screen window(const ReelGame& g, const std::vector<int>& stops);

struct LineResult {
    int symbol = -1;
    int count = 0;
    int pay = 0;
};
// rowOnReel[k] = which visible row the line uses on reel k.
LineResult evaluateLine(const ReelGame& g, const Screen& s, const std::vector<int>& rowOnReel);

struct WaysResult {
    int symbol = -1;
    int reels = 0;        // how many reels in a row (from the left) contain the symbol
    long long ways = 0;   // number of distinct left-to-right paths
    long long pay = 0;    // pay * ways
};
std::vector<WaysResult> evaluateWays(const ReelGame& g, const Screen& s);

struct RtpReport {
    std::uint64_t combos = 0;      // every possible combination of stops
    std::uint64_t winningCombos = 0;
    std::uint64_t totalPay = 0;    // sum of pays over all combos (bet 1 each)
    long double sumSquares = 0;    // sum of pay^2, for the variance
    double rtp = 0;                // totalPay / combos
    double hitFrequency = 0;       // winningCombos / combos
    double stdDev = 0;             // standard deviation of one spin's pay
};
// Brute force: every stop on every reel, centre line (row 1). Exact, no randomness.
RtpReport exactRtpCentreLine(const ReelGame& g);

// The lesson games.
const ReelGame& lucky3();   // 3 reels x 20 stops, one centre payline, no wild
const ReelGame& gemWays();  // 5 reels x 3 rows, 243 ways, with a wild

// ---- cluster pays & cascades (grid games) ----
using Grid = std::vector<std::vector<int>>;  // grid[row][col], -1 = empty
// All groups of orthogonally connected equal symbols with size >= minSize.
std::vector<std::vector<std::pair<int, int>>> findClusters(const Grid& grid, int minSize);
// Drop symbols down into empty cells (column by column); returns how many cells are empty at the top.
int applyGravity(Grid& grid);

}  // namespace forge::slot
