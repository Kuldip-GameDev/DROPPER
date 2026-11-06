// Dynamic programming and backtracking.
#include <algorithm>
#include <cstdlib>
#include <stdexcept>

#include "forge/algo/dp_backtracking.hpp"
#include "forge/algo/input.hpp"
#include "forge/algo/registry.hpp"
#include "forge/util/cat.hpp"
#include "forge/viz/scene.hpp"

namespace forge::algo {

using viz::ArrayScene;
using viz::GridScene;
using viz::Mark;
using viz::Recorder;

// [snippet:dp.coin_change]
// dp[x] = the fewest coins that make amount x. Build it up from 0:
// small answers are reused to build bigger ones (that's dynamic programming).
int coinChange(const std::vector<int>& coins, int amount) {
    const int INF = amount + 1;  // more coins than any real answer could need
    std::vector<int> dp(amount + 1, INF);
    dp[0] = 0;  // zero coins make zero
    for (int x = 1; x <= amount; ++x) {
        for (int c : coins) {
            if (c <= x) dp[x] = std::min(dp[x], dp[x - c] + 1);  // use coin c last  // @line relax
        }
    }
    return dp[amount] >= INF ? -1 : dp[amount];  // @line answer
}
// [/snippet]

// [snippet:dp.lcs]
// Longest common subsequence: dp[i][j] = answer for the first i letters
// of a and the first j letters of b.
int lcsLength(const std::string& a, const std::string& b) {
    std::vector<std::vector<int>> dp(a.size() + 1, std::vector<int>(b.size() + 1, 0));
    for (std::size_t i = 1; i <= a.size(); ++i) {
        for (std::size_t j = 1; j <= b.size(); ++j) {
            if (a[i - 1] == b[j - 1]) dp[i][j] = dp[i - 1][j - 1] + 1;  // letters match: extend  // @line match
            else dp[i][j] = std::max(dp[i - 1][j], dp[i][j - 1]);  // drop one letter  // @line skip
        }
    }
    return dp[a.size()][b.size()];
}
// [/snippet]

// [snippet:backtrack.nqueens]
// Backtracking: try a choice, go deeper, and UNDO it if it leads nowhere.
static bool safe(const std::vector<int>& colOf, int row, int col) {
    for (int r = 0; r < row; ++r) {
        // same column, or same diagonal (row distance == column distance)
        if (colOf[r] == col || std::abs(colOf[r] - col) == row - r) return false;
    }
    return true;
}

static bool place(int row, int n, std::vector<int>& colOf) {
    if (row == n) return true;  // every row has a queen: solved
    for (int col = 0; col < n; ++col) {
        if (!safe(colOf, row, col)) continue;
        colOf[row] = col;  // try it  // @line try
        if (place(row + 1, n, colOf)) return true;
        colOf[row] = -1;  // dead end: undo and try the next column  // @line undo
    }
    return false;
}

bool solveQueens(int n, std::vector<int>& colOf) {
    colOf.assign(n, -1);
    return place(0, n, colOf);
}
// [/snippet]

int countQueens(int n) {
    std::vector<int> colOf(n, -1);
    int count = 0;
    auto go = [&](auto&& self, int row) -> void {
        if (row == n) { ++count; return; }
        for (int col = 0; col < n; ++col) {
            if (!safe(colOf, row, col)) continue;
            colOf[row] = col;
            self(self, row + 1);
            colOf[row] = -1;
        }
    };
    go(go, 0);
    return count;
}

void registerDpBacktracking(Registry& reg) {
    reg.add({"dp.coin_change", "Coin change (DP)", "Dynamic programming", "dp.coin_change", "O(amount x coins)", "O(amount)",
           "Fewest coins for an amount: build every smaller answer once and reuse it.",
           {{"coins", "int_array", "Coin values"}, {"amount", "int", "Amount"}},
           {{"coins", {1, 3, 4}}, {"amount", 6}}},
          [](const Json& input, Recorder& rec) -> Json {
              const auto coins = intArray(input, "coins", 6);
              for (int c : coins) if (c <= 0 || c > 100) throw std::invalid_argument("coins must be 1-100");
              const int amount = intValue(input, "amount", 1, 30);
              const int INF = amount + 1;
              std::vector<int> dp(static_cast<std::size_t>(amount) + 1, INF);
              dp[0] = 0;
              auto scene = [&](int x) {
                  std::vector<std::string> labels;
                  for (int v : dp) labels.push_back(v >= INF ? "∞" : std::to_string(v));
                  auto s = ArrayScene::ofLabels(labels).title("dp[x] = fewest coins for amount x");
                  s.sortedUpTo(x);
                  if (x >= 0 && x <= amount) s.mark(x, Mark::Active).pointer("x", x);
                  return s;
              };
              rec.frame(scene(-1), "dp[0] = 0 (no coins needed). Everything else starts at ∞ (unknown).", "");
              for (int x = 1; x <= amount; ++x) {
                  std::string tried;
                  for (int c : coins) {
                      if (c > x) continue;
                      const int via = dp[static_cast<std::size_t>(x - c)] >= INF ? INF : dp[static_cast<std::size_t>(x - c)] + 1;
                      tried += cat(tried.empty() ? "" : ", ", "coin ", c, " → ", via >= INF ? std::string("∞") : std::to_string(via));
                      dp[static_cast<std::size_t>(x)] = std::min(dp[static_cast<std::size_t>(x)], via);
                  }
                  rec.frame(scene(x), cat("Amount ", x, ": try each coin as the LAST coin, reuse dp[x - coin]: ", tried.empty() ? "no coin fits" : tried, ". Best: ", dp[static_cast<std::size_t>(x)] >= INF ? std::string("impossible") : std::to_string(dp[static_cast<std::size_t>(x)]), "."), "relax");
              }
              const int answer = coinChange(coins, amount);
              rec.frame(scene(amount), answer < 0 ? "Impossible with these coins." : cat("Answer: ", answer, " coins. Greedy (biggest coin first) would say 4+1+1 = 3 coins for 6 with {1,3,4}; DP finds 3+3 = 2."), "answer");
              return {{"coins", answer}};
          });

    reg.add({"dp.lcs", "Longest common subsequence", "Dynamic programming", "dp.lcs", "O(n x m)", "O(n x m)",
           "How similar are two strings? A 2D table where each cell reuses three neighbours. Diff tools use this idea.",
           {{"a", "text", "First word"}, {"b", "text", "Second word"}},
           {{"a", "SPINNER"}, {"b", "WINNER"}}},
          [](const Json& input, Recorder& rec) -> Json {
              const auto a = textValue(input, "a", 8), b = textValue(input, "b", 8);
              if (a.empty() || b.empty()) throw std::invalid_argument("type two words");
              const int n = static_cast<int>(a.size()), m = static_cast<int>(b.size());
              std::vector<std::vector<int>> dp(static_cast<std::size_t>(n) + 1, std::vector<int>(static_cast<std::size_t>(m) + 1, 0));
              auto scene = [&](int ci, int cj, int filledRow, int filledCol) {
                  GridScene g(n + 1, m + 1);
                  g.rowHeader(0, "-").colHeader(0, "-");
                  for (int i = 1; i <= n; ++i) g.rowHeader(i, std::string(1, a[static_cast<std::size_t>(i - 1)]));
                  for (int j = 1; j <= m; ++j) g.colHeader(j, std::string(1, b[static_cast<std::size_t>(j - 1)]));
                  for (int i = 0; i <= n; ++i)
                      for (int j = 0; j <= m; ++j) {
                          const bool filled = i == 0 || j == 0 || i < filledRow || (i == filledRow && j <= filledCol);
                          g.set(i, j, filled ? std::to_string(dp[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)]) : "");
                      }
                  if (ci > 0) g.mark(ci, cj, Mark::Active);
                  return g;
              };
              rec.frame(scene(0, 0, 0, 0), "Row 0 and column 0 are 0: an empty word shares nothing.", "");
              for (int i = 1; i <= n; ++i)
                  for (int j = 1; j <= m; ++j) {
                      const bool match = a[static_cast<std::size_t>(i - 1)] == b[static_cast<std::size_t>(j - 1)];
                      auto& cell = dp[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)];
                      cell = match ? dp[static_cast<std::size_t>(i - 1)][static_cast<std::size_t>(j - 1)] + 1
                                   : std::max(dp[static_cast<std::size_t>(i - 1)][static_cast<std::size_t>(j)], dp[static_cast<std::size_t>(i)][static_cast<std::size_t>(j - 1)]);
                      auto s = scene(i, j, i, j);
                      if (match) s.mark(i - 1, j - 1, Mark::Found);
                      else s.mark(i - 1, j, Mark::Compare).mark(i, j - 1, Mark::Compare);
                      rec.frame(s, match ? cat("'", a[static_cast<std::size_t>(i - 1)], "' matches: diagonal + 1 = ", cell, ".") : cat("'", a[static_cast<std::size_t>(i - 1)], "' vs '", b[static_cast<std::size_t>(j - 1)], "': no match, take the bigger of up/left = ", cell, "."), match ? "match" : "skip");
                  }
              const int len = lcsLength(a, b);
              rec.frame(scene(n, m, n, m), cat("The bottom-right cell is the answer: ", len, " letters in common, in order."), "");
              return {{"length", len}};
          });

    reg.add({"backtrack.nqueens", "N-Queens (backtracking)", "Backtracking", "backtrack.nqueens", "exponential", "O(n)",
           "Place queens row by row; when stuck, undo the last choice and try the next one.",
           {{"n", "int", "Board size (4-8)"}}, {{"n", 5}}},
          [](const Json& input, Recorder& rec) -> Json {
              const int n = intValue(input, "n", 4, 8);
              std::vector<int> colOf(static_cast<std::size_t>(n), -1);
              auto board = [&](int hotRow, int hotCol, Mark m) {
                  GridScene g(n, n);
                  for (int r = 0; r < n; ++r)
                      for (int c = 0; c < n; ++c) {
                          g.set(r, c, colOf[static_cast<std::size_t>(r)] == c ? "♛" : "");
                          if ((r + c) % 2 == 1) g.mark(r, c, Mark::Dim);
                      }
                  if (hotRow >= 0) g.mark(hotRow, hotCol, m);
                  return g;
              };
              rec.frame(board(-1, -1, Mark::None), cat("Place ", n, " queens so that none attack each other: one per row, column and diagonal."), "");
              auto go = [&](auto&& self, int row) -> bool {
                  if (row == n) return true;
                  for (int col = 0; col < n; ++col) {
                      if (!safe(colOf, row, col)) continue;
                      colOf[static_cast<std::size_t>(row)] = col;
                      rec.frame(board(row, col, Mark::Active), cat("Row ", row, ": column ", col, " is safe. Try it and go to the next row."), "try");
                      if (self(self, row + 1)) return true;
                      colOf[static_cast<std::size_t>(row)] = -1;
                      rec.frame(board(row, col, Mark::Error), cat("Dead end below. Undo row ", row, ", column ", col, " and try further right."), "undo");
                  }
                  return false;
              };
              const bool ok = go(go, 0);
              rec.frame(board(-1, -1, Mark::None), ok ? cat("Solved! (There are ", countQueens(n), " solutions for n = ", n, " in total.)") : "No solution.", "");
              return {{"solved", ok}, {"columns", colOf}};
          });
}

}  // namespace forge::algo
