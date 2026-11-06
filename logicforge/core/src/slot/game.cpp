#include "forge/slot/game.hpp"

#include <cmath>
#include <queue>
#include <stdexcept>

namespace forge::slot {

// [snippet:slot.window]
// A reel strip is a CIRCULAR array: after the last stop comes the first.
// The % (modulo) operator does the wrap-around for us.
Screen window(const ReelGame& g, const std::vector<int>& stops) {
    Screen screen(g.rows, std::vector<int>(g.reels.size()));
    for (std::size_t reel = 0; reel < g.reels.size(); ++reel) {
        const auto& strip = g.reels[reel];
        const int len = static_cast<int>(strip.size());
        for (int row = 0; row < g.rows; ++row) {
            screen[row][reel] = strip[(stops[reel] + row) % len];  // @line wrap
        }
    }
    return screen;
}
// [/snippet]

// [snippet:slot.payline]
// Left to right: the first non-wild symbol decides what the line is
// "about"; count how many reels in a row match it (wilds count too).
LineResult evaluateLine(const ReelGame& g, const Screen& s, const std::vector<int>& rowOnReel) {
    const int reels = static_cast<int>(rowOnReel.size());
    int symbol = g.wild;
    for (int k = 0; k < reels; ++k) {
        const int sym = s[rowOnReel[k]][k];
        if (sym != g.wild) { symbol = sym; break; }  // @line pick_symbol
    }
    int count = 0;
    while (count < reels) {
        const int sym = s[rowOnReel[count]][count];
        if (sym != symbol && sym != g.wild) break;  // chain broken  // @line chain
        ++count;
    }
    const int pay = (symbol >= 0) ? g.paytable[symbol][count] : 0;  // @line pay
    return {symbol, count, pay};
}
// [/snippet]

// [snippet:slot.ways]
// "243 ways" on 5 reels x 3 rows: any position on reel 1, then any on
// reel 2, ... (3 x 3 x 3 x 3 x 3 = 243 paths). A symbol pays for every
// path that matches from the left, so we MULTIPLY the matches per reel.
std::vector<WaysResult> evaluateWays(const ReelGame& g, const Screen& s) {
    std::vector<WaysResult> wins;
    for (int sym = 0; sym < static_cast<int>(g.symbols.size()); ++sym) {
        if (sym == g.wild) continue;
        long long ways = 1;
        int reels = 0;
        for (std::size_t k = 0; k < g.reels.size(); ++k) {
            int matches = 0;
            for (int row = 0; row < g.rows; ++row) {
                if (s[row][k] == sym || s[row][k] == g.wild) ++matches;
            }
            if (matches == 0) break;  // the chain stops at the first reel without it
            ways *= matches;  // @line multiply
            ++reels;
        }
        const int pay = g.paytable[sym][reels];
        if (pay > 0) wins.push_back({sym, reels, ways, pay * ways});  // @line win
    }
    return wins;
}
// [/snippet]

// [snippet:slot.rtp_exact]
// Return To Player, exactly: try EVERY combination of stops once.
// Each combination is equally likely, so the average pay is the RTP.
RtpReport exactRtpCentreLine(const ReelGame& g) {
    if (g.reels.size() != 3) throw std::invalid_argument("this brute force is written for 3 reels");
    RtpReport r;
    const std::vector<int> centre = {1, 1, 1};
    const int n0 = static_cast<int>(g.reels[0].size());
    const int n1 = static_cast<int>(g.reels[1].size());
    const int n2 = static_cast<int>(g.reels[2].size());
    for (int a = 0; a < n0; ++a) {
        for (int b = 0; b < n1; ++b) {
            for (int c = 0; c < n2; ++c) {
                const LineResult win = evaluateLine(g, window(g, {a, b, c}), centre);
                ++r.combos;
                if (win.pay > 0) ++r.winningCombos;
                r.totalPay += static_cast<std::uint64_t>(win.pay);
                r.sumSquares += static_cast<long double>(win.pay) * win.pay;
            }
        }
    }
    r.rtp = static_cast<double>(r.totalPay) / static_cast<double>(r.combos);
    r.hitFrequency = static_cast<double>(r.winningCombos) / static_cast<double>(r.combos);
    const long double mean = static_cast<long double>(r.rtp);
    r.stdDev = static_cast<double>(std::sqrt(r.sumSquares / r.combos - mean * mean));  // @line variance
    return r;
}
// [/snippet]

// [snippet:slot.flood_fill]
// Breadth-first search: start at one cell, visit its matching neighbours,
// then THEIR neighbours... A queue keeps the "to visit" cells in order.
std::vector<std::vector<std::pair<int, int>>> findClusters(const Grid& grid, int minSize) {
    const int rows = static_cast<int>(grid.size());
    const int cols = rows ? static_cast<int>(grid[0].size()) : 0;
    std::vector<std::vector<bool>> seen(rows, std::vector<bool>(cols, false));
    std::vector<std::vector<std::pair<int, int>>> clusters;
    const int dr[4] = {-1, 1, 0, 0}, dc[4] = {0, 0, -1, 1};  // up, down, left, right

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            if (seen[r][c] || grid[r][c] < 0) continue;
            const int sym = grid[r][c];
            std::vector<std::pair<int, int>> cluster;
            std::queue<std::pair<int, int>> todo;
            todo.push({r, c});
            seen[r][c] = true;
            while (!todo.empty()) {
                auto [cr, cc] = todo.front();  // @line visit
                todo.pop();
                cluster.push_back({cr, cc});
                for (int d = 0; d < 4; ++d) {
                    const int nr = cr + dr[d], nc = cc + dc[d];
                    if (nr < 0 || nr >= rows || nc < 0 || nc >= cols) continue;  // off the grid
                    if (seen[nr][nc] || grid[nr][nc] != sym) continue;
                    seen[nr][nc] = true;  // mark when QUEUED, so nothing is queued twice
                    todo.push({nr, nc});  // @line enqueue
                }
            }
            if (static_cast<int>(cluster.size()) >= minSize) clusters.push_back(cluster);  // @line keep
        }
    }
    return clusters;
}
// [/snippet]

// [snippet:slot.gravity]
// Cascades: winning symbols vanish and everything above falls down.
// Per column, a "write" index walks up from the bottom (two pointers!).
int applyGravity(Grid& grid) {
    const int rows = static_cast<int>(grid.size());
    const int cols = rows ? static_cast<int>(grid[0].size()) : 0;
    int empty = 0;
    for (int c = 0; c < cols; ++c) {
        int write = rows - 1;
        for (int read = rows - 1; read >= 0; --read) {
            if (grid[read][c] >= 0) grid[write--][c] = grid[read][c];  // @line fall
        }
        for (int r = write; r >= 0; --r) {
            grid[r][c] = -1;  // these cells are now empty at the top
            ++empty;
        }
    }
    return empty;
}
// [/snippet]

// ---------------------------------------------------------------- lesson games

const ReelGame& lucky3() {
    static const ReelGame game = [] {
        ReelGame g;
        g.name = "Lucky 3";
        //            0             1                2              3             4
        g.symbols = {{"SEVEN", "7"}, {"BAR", "BAR"}, {"BELL", "🔔"}, {"CHERRY", "🍒"}, {"LEMON", "🍋"}};
        enum { S, R, B, C, L };
        // 20 stops: LEMON x9, CHERRY x5, BELL x3, BAR x2, SEVEN x1.
        const std::vector<int> strip = {L, C, L, B, L, C, L, R, L, C, L, B, L, C, L, S, L, C, B, R};
        auto rotated = [&strip](int by) {
            std::vector<int> r(strip.size());
            for (std::size_t i = 0; i < strip.size(); ++i) r[i] = strip[(i + by) % strip.size()];
            return r;
        };
        g.reels = {strip, rotated(7), rotated(13)};
        g.rows = 3;
        // Pays for 3 in a row on the centre line (index = count).
        g.paytable = {
            {0, 0, 0, 800},  // SEVEN
            {0, 0, 0, 100},  // BAR
            {0, 0, 0, 40},   // BELL
            {0, 0, 0, 16},   // CHERRY
            {0, 0, 0, 4},    // LEMON
        };
        return g;
    }();
    return game;
}

const ReelGame& gemWays() {
    static const ReelGame game = [] {
        ReelGame g;
        g.name = "Gem Ways";
        g.symbols = {{"WILD", "W"}, {"DIAMOND", "💎"}, {"BELL", "🔔"}, {"CHERRY", "🍒"}, {"LEMON", "🍋"}};
        enum { W, D, B, C, L };
        g.reels = {
            {L, C, B, L, D, C, L, B, C, L, D, L, C, B, L},
            {C, L, W, B, L, C, D, L, B, C, L, W, C, L, B},
            {L, B, C, W, L, D, C, L, B, L, C, W, L, B, C},
            {B, L, C, D, L, W, C, B, L, C, L, D, B, L, C},
            {C, L, B, L, D, C, L, B, C, L, W, L, B, C, L},
        };
        g.rows = 3;
        g.wild = W;
        g.paytable = {
            {0, 0, 0, 0, 0, 0},       // WILD (substitutes only)
            {0, 0, 0, 10, 50, 200},   // DIAMOND: 3, 4, 5 reels
            {0, 0, 0, 5, 20, 80},     // BELL
            {0, 0, 0, 2, 8, 30},      // CHERRY
            {0, 0, 0, 1, 4, 15},      // LEMON
        };
        return g;
    }();
    return game;
}

}  // namespace forge::slot
