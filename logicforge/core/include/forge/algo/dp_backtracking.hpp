#pragma once
#include <string>
#include <vector>

namespace forge::algo {

int coinChange(const std::vector<int>& coins, int amount);  // fewest coins, or -1
int lcsLength(const std::string& a, const std::string& b);
// Places n queens; colOf[row] = column. Returns false if impossible.
bool solveQueens(int n, std::vector<int>& colOf);
int countQueens(int n);

}  // namespace forge::algo
