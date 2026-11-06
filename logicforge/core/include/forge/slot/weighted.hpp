#pragma once
// Weighted random picks: "cherries are common, sevens are rare".
//
// Three ways to turn a random number into a weighted choice:
//   pickLinear  O(n)       walk the weights until the random number runs out
//   pickBinary  O(log n)   binary-search the running totals (prefix sums)
//   AliasTable  O(1)       Vose's alias method: one column, one coin flip
#include <cstddef>
#include <cstdint>
#include <vector>

namespace forge::slot {

// r must be in [0, total weight). Counts comparisons for the visualizer.
std::size_t pickLinear(const std::vector<std::uint32_t>& weights, std::uint64_t r, int* comparisons = nullptr);

// prefix[i] = weights[0] + ... + weights[i]. r in [0, prefix.back()).
std::vector<std::uint64_t> runningTotals(const std::vector<std::uint32_t>& weights);
std::size_t pickBinary(const std::vector<std::uint64_t>& prefix, std::uint64_t r, int* comparisons = nullptr);

// Vose's alias method. Built once in O(n); every pick is O(1).
class AliasTable {
public:
    explicit AliasTable(const std::vector<std::uint32_t>& weights);
    // column: uniform in [0, n). coin: uniform in [0, 1).
    std::size_t pick(std::size_t column, double coin) const {
        return coin < prob_[column] ? column : alias_[column];
    }
    std::size_t size() const { return prob_.size(); }
    const std::vector<double>& prob() const { return prob_; }
    const std::vector<std::size_t>& alias() const { return alias_; }
    // Exact probability of each outcome implied by the tables (for testing).
    std::vector<double> impliedProbabilities() const;

private:
    std::vector<double> prob_;
    std::vector<std::size_t> alias_;
};

}  // namespace forge::slot
