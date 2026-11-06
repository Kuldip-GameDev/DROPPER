#include <algorithm>
#include <numeric>
#include <stdexcept>

#include "forge/slot/rng.hpp"
#include "forge/slot/weighted.hpp"

namespace forge::slot {

double chiSquare(const std::vector<std::uint64_t>& observed, double expectedEach) {
    double chi = 0;
    for (std::uint64_t o : observed) {
        const double d = static_cast<double>(o) - expectedEach;
        chi += d * d / expectedEach;
    }
    return chi;
}

// [snippet:slot.pick_linear]
std::size_t pickLinear(const std::vector<std::uint32_t>& weights, std::uint64_t r,
                       int* comparisons) {
    for (std::size_t i = 0; i < weights.size(); ++i) {
        if (comparisons) ++*comparisons;
        if (r < weights[i]) return i;  // r landed inside this symbol's slice  // @line check
        r -= weights[i];               // skip past this slice  // @line skip
    }
    throw std::out_of_range("r must be smaller than the total weight");
}
// [/snippet]

std::vector<std::uint64_t> runningTotals(const std::vector<std::uint32_t>& weights) {
    std::vector<std::uint64_t> prefix(weights.size());
    std::uint64_t sum = 0;
    for (std::size_t i = 0; i < weights.size(); ++i) prefix[i] = sum += weights[i];
    return prefix;
}

// [snippet:slot.pick_binary]
// First running total that is GREATER than r. Totals [40, 70, 90, 100]:
// r = 0..39 -> 0,  r = 40..69 -> 1,  r = 70..89 -> 2,  r = 90..99 -> 3
std::size_t pickBinary(const std::vector<std::uint64_t>& prefix, std::uint64_t r,
                       int* comparisons) {
    std::size_t lo = 0, hi = prefix.size();  // answer is in [lo, hi)
    while (lo < hi) {
        const std::size_t mid = lo + (hi - lo) / 2;
        if (comparisons) ++*comparisons;
        if (prefix[mid] > r) hi = mid;  // mid could be the answer  // @line go_left
        else lo = mid + 1;              // answer is to the right  // @line go_right
    }
    if (lo == prefix.size()) throw std::out_of_range("r must be smaller than the total weight");
    return lo;
}
// [/snippet]

// [snippet:slot.alias]
AliasTable::AliasTable(const std::vector<std::uint32_t>& weights)
    : prob_(weights.size(), 0.0), alias_(weights.size(), 0) {
    const std::size_t n = weights.size();
    const double total = std::accumulate(weights.begin(), weights.end(), 0.0);
    if (n == 0 || total <= 0) throw std::invalid_argument("AliasTable needs positive weights");

    // Scale so the average column height is exactly 1.
    std::vector<double> scaled(n);
    std::vector<std::size_t> small, large;
    for (std::size_t i = 0; i < n; ++i) {
        scaled[i] = static_cast<double>(weights[i]) * static_cast<double>(n) / total;
        (scaled[i] < 1.0 ? small : large).push_back(i);
    }
    // Fill each short column to height 1 with a piece of a tall one.
    while (!small.empty() && !large.empty()) {
        const std::size_t s = small.back();
        small.pop_back();
        const std::size_t l = large.back();
        prob_[s] = scaled[s];  // @line pair
        alias_[s] = l;
        scaled[l] -= 1.0 - scaled[s];  // l donated the gap  // @line donate
        if (scaled[l] < 1.0) {
            large.pop_back();
            small.push_back(l);
        }
    }
    // Leftovers are (up to rounding) exactly 1: they need no partner.
    for (std::size_t i : large) { prob_[i] = 1.0; alias_[i] = i; }
    for (std::size_t i : small) { prob_[i] = 1.0; alias_[i] = i; }  // @line leftovers
}
// [/snippet]

std::vector<double> AliasTable::impliedProbabilities() const {
    const std::size_t n = prob_.size();
    std::vector<double> p(n, 0.0);
    for (std::size_t i = 0; i < n; ++i) {
        p[i] += prob_[i] / static_cast<double>(n);
        p[alias_[i]] += (1.0 - prob_[i]) / static_cast<double>(n);
    }
    return p;
}

}  // namespace forge::slot
