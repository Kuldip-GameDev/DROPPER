// Visual traces for the Slot Math Lab. The logic being visualised lives in
// game.cpp, rng_weighted.cpp and machine.cpp; this file only records frames.
#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>

#include "forge/algo/input.hpp"
#include "forge/algo/registry.hpp"
#include "forge/slot/game.hpp"
#include "forge/slot/machine.hpp"
#include "forge/slot/rng.hpp"
#include "forge/slot/weighted.hpp"
#include "forge/util/cat.hpp"
#include "forge/viz/scene.hpp"

namespace forge::slot {

using algo::Field;
using viz::ArrayScene;
using viz::GraphScene;
using viz::GridScene;
using viz::Mark;
using viz::PlotScene;
using viz::Recorder;

namespace {

const std::vector<std::string> kGemLabels = {"🍒", "🍋", "🔔", "💎"};

std::string pct(double x, int decimals = 2) {
    const double scale = std::pow(10.0, decimals);
    return cat(std::round(x * 100 * scale) / scale, "%");
}

GridScene screenGrid(const ReelGame& g, const Screen& s) {
    GridScene grid(g.rows, static_cast<int>(g.reels.size()));
    for (int r = 0; r < g.rows; ++r)
        for (int k = 0; k < static_cast<int>(g.reels.size()); ++k)
            grid.set(r, k, g.symbols[static_cast<std::size_t>(s[r][k])].label);
    for (int k = 0; k < static_cast<int>(g.reels.size()); ++k) grid.colHeader(k, cat("reel ", k + 1));
    return grid;
}

Grid randomGrid(int rows, int cols, int symbols, Pcg32& rng) {
    Grid g(static_cast<std::size_t>(rows), std::vector<int>(static_cast<std::size_t>(cols)));
    for (auto& row : g)
        for (int& cell : row) cell = static_cast<int>(uniformBelow(rng, static_cast<std::uint32_t>(symbols)));
    return g;
}

GridScene gemGrid(const Grid& g) {
    const int rows = static_cast<int>(g.size());
    const int cols = rows ? static_cast<int>(g[0].size()) : 0;
    GridScene scene(rows, cols);
    for (int r = 0; r < rows; ++r)
        for (int c = 0; c < cols; ++c) {
            const int v = g[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)];
            scene.set(r, c, v < 0 ? "·" : kGemLabels[static_cast<std::size_t>(v)]);
            if (v < 0) scene.mark(r, c, Mark::Dim);
        }
    return scene;
}

std::vector<std::uint32_t> weightsInput(const Json& input) {
    const auto raw = algo::intArray(input, "weights", 12);
    if (raw.empty()) throw std::invalid_argument("'weights' needs at least one number");
    std::vector<std::uint32_t> w;
    for (int x : raw) {
        if (x <= 0) throw std::invalid_argument("every weight must be at least 1");
        w.push_back(static_cast<std::uint32_t>(x));
    }
    return w;
}

std::vector<int> toInts(const std::vector<std::uint64_t>& v) {
    std::vector<int> out;
    for (auto x : v) out.push_back(static_cast<int>(x));
    return out;
}

// --------------------------------------------------------------- RNG traces

Json moduloBias(Recorder& rec) {
    PlotScene raw;
    raw.title("A toy 3-bit generator: 8 equally likely outputs").axes("output", "chances out of 8");
    for (int v = 0; v < 8; ++v) raw.bar(std::to_string(v), 1);
    rec.frame(raw, "Imagine a generator that outputs 0..7, each equally likely. We want a fair choice of 3 symbols.", "");

    std::vector<int> counts(3, 0);
    for (int v = 0; v < 8; ++v) ++counts[static_cast<std::size_t>(v % 3)];
    PlotScene biased;
    biased.title("output % 3").axes("symbol", "chances out of 8");
    for (int s = 0; s < 3; ++s) biased.bar(std::to_string(s), counts[static_cast<std::size_t>(s)], s == 2 ? Mark::Error : Mark::Compare);
    biased.reference(8.0 / 3.0, "fair = 2.67");
    rec.frame(biased, "With % 3: 0 comes from {0,3,6}, 1 from {1,4,7}, but 2 only from {2,5}. Symbol 2 is LESS likely. That is modulo bias.", "");

    const int limit = 8 % 3;  // same rule as uniformBelow: reject raw values below 2^bits mod n
    std::vector<int> fair(3, 0);
    for (int v = limit; v < 8; ++v) ++fair[static_cast<std::size_t>(v % 3)];
    PlotScene fixed;
    fixed.title("reject 0 and 1, then % 3").axes("symbol", "chances out of 6 kept values");
    for (int s = 0; s < 3; ++s) fixed.bar(std::to_string(s), fair[static_cast<std::size_t>(s)], Mark::Ok);
    rec.frame(fixed, cat("Fix: 8 mod 3 = ", limit, ", so throw away raw values below ", limit, " and draw again. The 6 values left split evenly: 2 each."), "limit");

    PlotScene real;
    real.title("Same idea with 32 bits").axes("", "");
    real.var("2^32 mod 3", (std::uint64_t{1} << 32) % 3).var("2^32 mod 1000", (std::uint64_t{1} << 32) % 1000);
    rec.frame(real, "With a 32-bit generator the bias for small n is tiny (for n = 1000, 296 of 4.29 billion values are rejected), but a certified game must still remove it. uniformBelow does exactly that.", "accept");
    return {{"biased", counts}, {"fair", fair}};
}

template <Rng32 G>
std::vector<std::uint64_t> histogram10(G& rng, int draws) {
    std::vector<std::uint64_t> buckets(10, 0);
    for (int i = 0; i < draws; ++i) ++buckets[uniformBelow(rng, 10)];
    return buckets;
}

Json rngQuality(const Json& input, Recorder& rec) {
    const int draws = algo::intValue(input, "draws", 1000, 500000);
    const double expected = draws / 10.0;
    Lcg32 lcg(12345);
    XorShift32 xs(12345);
    Pcg32 pcg(12345, 54);
    Json chis = Json::object();
    auto show = [&](const std::vector<std::uint64_t>& h, const char* name) {
        const double chi = chiSquare(h, expected);
        chis[name] = chi;
        PlotScene p;
        p.title(cat(name, ": ", draws, " draws into 10 buckets")).axes("bucket", "count");
        for (int b = 0; b < 10; ++b) p.bar(std::to_string(b), static_cast<double>(h[static_cast<std::size_t>(b)]));
        p.reference(expected, "expected").var("chi-square", std::round(chi * 100) / 100).var("typical if fair", "below ~16.9");
        rec.frame(p, cat(name, ": chi-square = ", std::round(chi * 100) / 100, ". With 10 buckets, a fair generator stays below about 16.9 in 95% of runs."), "");
    };
    show(histogram10(lcg, draws), "LCG");
    show(histogram10(xs, draws), "xorshift32");
    show(histogram10(pcg, draws), "PCG32");

    // The LCG's hidden flaw: its lowest bit simply alternates.
    Lcg32 lcg2(12345);
    Pcg32 pcg2(12345, 54);
    PlotScene bits;
    bits.title("Lowest bit of the first 24 raw outputs").axes("draw", "bit");
    std::vector<std::pair<double, double>> lcgBits, pcgBits;
    for (int i = 0; i < 24; ++i) {
        lcgBits.emplace_back(i, lcg2.next() & 1u);
        pcgBits.emplace_back(i, (pcg2.next() & 1u) + 1.5);
    }
    bits.line("LCG", lcgBits, Mark::Error).line("PCG32 (shifted up)", pcgBits, Mark::Ok);
    rec.frame(bits, "The histograms looked fine, but look at the LCG's lowest bit: 1, 0, 1, 0... forever. Code that uses `rng() % 2` on this LCG gets a perfectly predictable coin. Passing one test is not enough.", "");
    return chis;
}

// ---------------------------------------------------------- weighted picks

Json pickLinearTrace(const Json& input, Recorder& rec) {
    const auto w = weightsInput(input);
    const std::uint64_t total = std::accumulate(w.begin(), w.end(), std::uint64_t{0});
    const int r0 = algo::intValue(input, "r", 0, static_cast<int>(total) - 1);
    std::vector<int> wi(w.begin(), w.end());
    std::uint64_t r = static_cast<std::uint64_t>(r0);
    for (std::size_t i = 0; i < w.size(); ++i) {
        const bool hit = r < w[i];
        rec.frame(ArrayScene(wi).title("weights").mark(static_cast<int>(i), hit ? Mark::Found : Mark::Compare).markRange(0, static_cast<int>(i), Mark::Dim).pointer("i", static_cast<int>(i)).var("r", r),
                  hit ? cat("Is r = ", r, " < ", w[i], "? Yes: symbol ", i, " wins.") : cat("Is r = ", r, " < ", w[i], "? No. Subtract it: r = ", r - w[i], "."),
                  hit ? "check" : "skip");
        if (hit) break;
        r -= w[i];
    }
    int comparisons = 0;
    const auto picked = pickLinear(w, static_cast<std::uint64_t>(r0), &comparisons);
    return {{"symbol", picked}, {"comparisons", comparisons}};
}

Json pickBinaryTrace(const Json& input, Recorder& rec) {
    const auto w = weightsInput(input);
    const auto prefix = runningTotals(w);
    const int r = algo::intValue(input, "r", 0, static_cast<int>(prefix.back()) - 1);
    const auto shown = toInts(prefix);
    rec.frame(ArrayScene(shown).title("running totals"), cat("Running totals of the weights. The pick is the first total GREATER than r = ", r, "."), "");
    std::size_t lo = 0, hi = prefix.size();
    while (lo < hi) {
        const std::size_t mid = lo + (hi - lo) / 2;
        const bool left = prefix[mid] > static_cast<std::uint64_t>(r);
        auto scene = ArrayScene(shown).title("running totals").window(static_cast<int>(lo), static_cast<int>(hi)).mark(static_cast<int>(mid), Mark::Active).pointer("lo", static_cast<int>(lo)).pointer("mid", static_cast<int>(mid)).pointer("hi", static_cast<int>(hi)).var("r", r);
        rec.frame(scene, left ? cat(prefix[mid], " > ", r, ": the answer is here or to the left.") : cat(prefix[mid], " <= ", r, ": the answer is to the right."), left ? "go_left" : "go_right");
        if (left) hi = mid;
        else lo = mid + 1;
    }
    rec.frame(ArrayScene(shown).title("running totals").mark(static_cast<int>(lo), Mark::Found), cat("Symbol ", lo, " wins. Binary search needs about log2(n) checks, however many symbols there are."), "");
    int comparisons = 0;
    return {{"symbol", pickBinary(prefix, static_cast<std::uint64_t>(r), &comparisons)}, {"comparisons", comparisons}};
}

Json aliasTrace(const Json& input, Recorder& rec) {
    const auto w = weightsInput(input);
    const std::size_t n = w.size();
    const double total = std::accumulate(w.begin(), w.end(), 0.0);
    // Re-run Vose's steps visibly (same algorithm as AliasTable).
    std::vector<double> scaled(n);
    for (std::size_t i = 0; i < n; ++i) scaled[i] = static_cast<double>(w[i]) * static_cast<double>(n) / total;
    auto heights = [&scaled] {
        std::vector<int> h;
        for (double x : scaled) h.push_back(static_cast<int>(std::lround(x * 100)));
        return h;
    };
    rec.frame(ArrayScene(heights()).layout(ArrayScene::Layout::Bars).title("column heights x100 (average = 100)"),
              "Scale the weights so the AVERAGE column is exactly 100. Some columns are short (< 100), some are tall.", "");
    std::vector<std::size_t> small, large;
    for (std::size_t i = 0; i < n; ++i) (scaled[i] < 1.0 ? small : large).push_back(i);
    std::vector<double> prob(n, 1.0);
    std::vector<std::size_t> alias(n);
    std::iota(alias.begin(), alias.end(), std::size_t{0});
    while (!small.empty() && !large.empty()) {
        const std::size_t s = small.back();
        small.pop_back();
        const std::size_t l = large.back();
        prob[s] = scaled[s];
        alias[s] = l;
        const double gap = 1.0 - scaled[s];
        scaled[l] -= gap;
        rec.frame(ArrayScene(heights()).layout(ArrayScene::Layout::Bars).title("column heights x100").mark(static_cast<int>(s), Mark::Swap).mark(static_cast<int>(l), Mark::Pivot),
                  cat("Column ", s, " is short by ", std::lround(gap * 100), ". Column ", l, " donates that much. Column ", s, " is now full: ", std::lround(prob[s] * 100), "% itself, the rest goes to ", l, "."),
                  "donate");
        if (scaled[l] < 1.0) {
            large.pop_back();
            small.push_back(l);
        }
    }
    const AliasTable table(w);
    GridScene g(static_cast<int>(n), 3);
    g.colHeader(0, "column").colHeader(1, "keep %").colHeader(2, "else alias");
    for (std::size_t i = 0; i < n; ++i) {
        g.set(static_cast<int>(i), 0, static_cast<int>(i));
        g.set(static_cast<int>(i), 1, cat(std::lround(table.prob()[i] * 100)));
        g.set(static_cast<int>(i), 2, table.prob()[i] >= 1.0 ? std::string("-") : std::to_string(table.alias()[i]));
    }
    rec.frame(g, "The finished table. To pick: choose a column uniformly, then flip a biased coin: keep the column, or take its alias. Always exactly 2 random steps, so O(1), even with 1,000 symbols.", "leftovers");
    const auto implied = table.impliedProbabilities();
    Json probs = Json::array();
    for (double p : implied) probs.push_back(p);
    return {{"probabilities", probs}};
}

// ------------------------------------------------------------- reel traces

Json reelWindowTrace(const Json& input, Recorder& rec) {
    const ReelGame& g = lucky3();
    const auto stops = algo::intArray(input, "stops", 3);
    if (stops.size() != 3) throw std::invalid_argument("'stops' needs exactly 3 numbers (one per reel)");
    for (std::size_t k = 0; k < 3; ++k) {
        const auto& strip = g.reels[k];
        const int len = static_cast<int>(strip.size());
        if (stops[k] < 0 || stops[k] >= len) throw std::invalid_argument(cat("stops must be between 0 and ", len - 1));
        GridScene s(1, len);
        for (int i = 0; i < len; ++i) s.set(0, i, g.symbols[static_cast<std::size_t>(strip[static_cast<std::size_t>(i)])].label).colHeader(i, std::to_string(i));
        std::string math;
        for (int row = 0; row < g.rows; ++row) {
            const int pos = (stops[k] + row) % len;
            s.mark(0, pos, row == 1 ? Mark::Found : Mark::Active);
            math += cat("(", stops[k], " + ", row, ") % ", len, " = ", pos, row + 1 < g.rows ? ",  " : "");
        }
        s.title(cat("reel ", k + 1, " strip (", len, " stops)"));
        rec.frame(s, cat("Reel ", k + 1, " stops at ", stops[k], ". Visible rows: ", math, ". The modulo wraps past the end back to 0."), "wrap");
    }
    const Screen screen = window(g, {stops[0], stops[1], stops[2]});
    rec.frame(screenGrid(g, screen).path({{1, 0}, {1, 1}, {1, 2}}, Mark::Pivot), "The 3x3 window the player sees. The gold line is the payline.", "");
    return {{"screen", screen}};
}

Json paylineTrace(const Json& input, Recorder& rec) {
    const ReelGame& g = lucky3();
    const auto stops = algo::intArray(input, "stops", 3);
    if (stops.size() != 3) throw std::invalid_argument("'stops' needs exactly 3 numbers (one per reel)");
    for (int s : stops)
        if (s < 0 || s >= 20) throw std::invalid_argument("stops must be between 0 and 19");
    const Screen screen = window(g, {stops[0], stops[1], stops[2]});
    const std::vector<int> centre = {1, 1, 1};
    const LineResult win = evaluateLine(g, screen, centre);
    auto base = [&] { return screenGrid(g, screen).path({{1, 0}, {1, 1}, {1, 2}}, Mark::Pivot); };
    const auto name = [&](int sym) { return g.symbols[static_cast<std::size_t>(sym)].label; };
    rec.frame(base().mark(1, 0, Mark::Active), cat("Reel 1 on the payline shows ", name(screen[1][0]), ". That's the symbol this line is about."), "pick_symbol");
    for (int k = 1; k < 3; ++k) {
        const bool match = k < win.count;
        auto s = base();
        for (int j = 0; j < k; ++j) s.mark(1, j, Mark::Ok);
        s.mark(1, k, match ? Mark::Ok : Mark::Error);
        rec.frame(s, match ? cat("Reel ", k + 1, " shows ", name(screen[1][k]), ": it matches. Chain = ", k + 1, ".") : cat("Reel ", k + 1, " shows ", name(screen[1][k]), ": no match. The chain stops at ", k, "."), "chain");
        if (!match) break;
    }
    auto last = base();
    for (int j = 0; j < win.count; ++j) last.mark(1, j, win.pay > 0 ? Mark::Found : Mark::Dim);
    last.var("pays", win.pay);
    rec.frame(last, win.pay > 0 ? cat(win.count, " x ", name(win.symbol), " pays ", win.pay, " credits for a 1-credit bet.") : "No win this time. In Lucky 3, only 3 in a row pays.", "pay");
    return {{"symbol", win.symbol}, {"count", win.count}, {"pay", win.pay}};
}

Json waysTrace(const Json& input, Recorder& rec) {
    const ReelGame& g = gemWays();
    const auto stops = algo::intArray(input, "stops", 5);
    if (stops.size() != 5) throw std::invalid_argument("'stops' needs exactly 5 numbers (one per reel)");
    for (int s : stops)
        if (s < 0 || s >= 15) throw std::invalid_argument("stops must be between 0 and 14");
    const Screen screen = window(g, stops);
    rec.frame(screenGrid(g, screen), "5 reels x 3 rows. No paylines: any symbol on reel 1 can connect to ANY matching position on reel 2, and so on. W is wild.", "");
    const auto wins = evaluateWays(g, screen);
    long long total = 0;
    for (const auto& w : wins) {
        auto s = screenGrid(g, screen);
        std::string product;
        for (int k = 0; k < w.reels; ++k) {
            int matches = 0;
            for (int r = 0; r < g.rows; ++r) {
                const int sym = screen[static_cast<std::size_t>(r)][static_cast<std::size_t>(k)];
                if (sym == w.symbol || sym == g.wild) {
                    s.mark(r, k, sym == g.wild ? Mark::Pivot : Mark::Found);
                    ++matches;
                }
            }
            product += cat(k ? " x " : "", matches);
        }
        total += w.pay;
        const auto& sym = g.symbols[static_cast<std::size_t>(w.symbol)];
        s.var("ways", w.ways).var("pays", w.pay);
        rec.frame(s, cat(sym.label, " appears on ", w.reels, " reels in a row. Ways = ", product, " = ", w.ways, ". Pay ", g.paytable[static_cast<std::size_t>(w.symbol)][static_cast<std::size_t>(w.reels)], " x ", w.ways, " = ", w.pay, "."), "multiply");
    }
    rec.frame(screenGrid(g, screen).var("total", total), wins.empty() ? "No symbol reaches 3 reels in a row: no win." : cat("Total win: ", total, ". Multiplying counts instead of listing all 243 paths is the whole trick."), "win");
    return {{"total", total}, {"wins", static_cast<int>(wins.size())}};
}

Json rtpExactTrace(Recorder& rec) {
    const ReelGame& g = lucky3();
    const int nsym = static_cast<int>(g.symbols.size());
    GridScene table(nsym + 1, 6);
    const char* heads[] = {"reel 1", "reel 2", "reel 3", "hits", "pays", "total"};
    for (int c = 0; c < 6; ++c) table.colHeader(c, heads[c]);
    for (int s = 0; s < nsym; ++s) table.rowHeader(s, g.symbols[static_cast<std::size_t>(s)].label);
    table.rowHeader(nsym, "sum");
    rec.frame(table, "Every stop is equally likely, so there are 20 x 20 x 20 = 8,000 equally likely spins. Count how many of them win each prize.", "");
    std::uint64_t hits = 0, pay = 0;
    for (int s = 0; s < nsym; ++s) {
        std::uint64_t h = 1;
        std::string product;
        for (int k = 0; k < 3; ++k) {
            const auto& strip = g.reels[static_cast<std::size_t>(k)];
            const auto c = std::count(strip.begin(), strip.end(), s);
            table.set(s, k, static_cast<int>(c));
            product += cat(k ? " x " : "", c);
            h *= static_cast<std::uint64_t>(c);
        }
        const int p = g.paytable[static_cast<std::size_t>(s)][3];
        table.set(s, 3, static_cast<int>(h)).set(s, 4, p).set(s, 5, static_cast<int>(h * static_cast<std::uint64_t>(p)));
        for (int c = 0; c < 6; ++c) table.mark(s, c, Mark::Active);
        hits += h;
        pay += h * static_cast<std::uint64_t>(p);
        rec.frame(table, cat(g.symbols[static_cast<std::size_t>(s)].label, " x3: ", product, " = ", h, " winning spins, paying ", p, " each."), "");
        for (int c = 0; c < 6; ++c) table.mark(s, c, Mark::None);
    }
    table.set(nsym, 3, static_cast<int>(hits)).set(nsym, 5, static_cast<int>(pay));
    const RtpReport brute = exactRtpCentreLine(g);
    table.mark(nsym, 3, Mark::Found).mark(nsym, 5, Mark::Found);
    table.var("RTP", pct(brute.rtp)).var("hit frequency", pct(brute.hitFrequency, 3)).var("std dev / spin", std::round(brute.stdDev * 100) / 100);
    rec.frame(table, cat("Total paid over all 8,000 spins: ", pay, ". RTP = ", pay, " / 8,000 = ", pct(brute.rtp), ". Hit frequency = ", hits, " / 8,000 = ", pct(brute.hitFrequency, 3), ". A brute-force loop over all 8,000 stop combinations gives exactly the same numbers."), "variance");
    return {{"rtp", brute.rtp}, {"hitFrequency", brute.hitFrequency}, {"stdDev", brute.stdDev}, {"totalPay", brute.totalPay}};
}

}  // namespace

// Records the convergence curve at 10, 20, 50, 100, 200, 500... spins and
// draws a frame at every power of ten (and at the end).
class SimViz {
public:
    SimViz(Recorder& rec, const ReelGame& g, std::uint64_t spins) : rec_(rec), spins_(spins) {
        const RtpReport exact = exactRtpCentreLine(g);
        rtp_ = exact.rtp;
        sd_ = exact.stdDev;
    }
    void point(std::uint64_t i, std::uint64_t paid) {
        if (i != next_ && i != spins_) return;
        const double x = std::log10(static_cast<double>(i));
        const double band = 1.96 * sd_ / std::sqrt(static_cast<double>(i));
        const double sim = static_cast<double>(paid) / static_cast<double>(i);
        curve_.emplace_back(x, 100 * sim);
        upper_.emplace_back(x, 100 * (rtp_ + band));
        lower_.emplace_back(x, 100 * (rtp_ - band));
        const bool decade = i == decade_;
        if (i == next_) {
            step_ = (step_ + 1) % 3;
            next_ = decade_ * (step_ == 0 ? 10 : step_ == 1 ? 2 : 5);
            if (step_ == 0) decade_ *= 10;
        }
        if (!decade && i != spins_) return;
        PlotScene p;
        p.title("running RTP vs number of spins").axes("spins (log10)", "RTP %");
        p.line("simulated", curve_, Mark::Active).line("95% band", upper_, Mark::Dim).line("", lower_, Mark::Dim);
        p.reference(100 * rtp_, cat("exact ", std::round(rtp_ * 10000) / 100, "%"));
        p.var("spins", i).var("simulated", cat(std::round(sim * 10000) / 100, "%"));
        const bool inside = std::fabs(sim - rtp_) <= band;
        rec_.frame(p, cat("After ", i, " spins: ", std::round(sim * 10000) / 100, "%. About 95% of runs land inside the band (+/- ", std::round(band * 10000) / 100, " points). ", inside ? "This one does." : "This one doesn't: about 1 run in 20 misses, because rare 800-credit jackpots make shorter runs swing."), "spin");
    }

private:
    Recorder& rec_;
    std::uint64_t spins_;
    double rtp_ = 0, sd_ = 0;
    std::uint64_t next_ = 10, decade_ = 10;
    int step_ = 0;
    std::vector<std::pair<double, double>> curve_, upper_, lower_;
};

// [snippet:slot.rtp_sim]
// Monte Carlo: play many random spins and average the pay.
// The error shrinks like 1 / sqrt(spins): 100x more spins = 10x more accurate.
double simulateRtp(const ReelGame& g, std::uint64_t spins, Pcg32& rng, Recorder& rec) {
    SimViz viz(rec, g, spins);  // @viz
    const std::vector<int> centre = {1, 1, 1};
    std::vector<int> stops(g.reels.size());
    std::uint64_t paid = 0;
    for (std::uint64_t i = 1; i <= spins; ++i) {
        for (std::size_t k = 0; k < stops.size(); ++k) {
            const auto len = static_cast<std::uint32_t>(g.reels[k].size());
            stops[k] = static_cast<int>(uniformBelow(rng, len));  // fair stop
        }
        paid += static_cast<std::uint64_t>(evaluateLine(g, window(g, stops), centre).pay);  // @line spin
        viz.point(i, paid);  // @viz
    }
    return static_cast<double>(paid) / static_cast<double>(spins);
}
// [/snippet]

namespace {

Json rtpSimTrace(const Json& input, Recorder& rec) {
    const int spins = algo::intValue(input, "spins", 100, 500000);
    const int seed = algo::intValue(input, "seed", 0, 1'000'000);
    const RtpReport exact = exactRtpCentreLine(lucky3());
    Pcg32 rng(static_cast<std::uint64_t>(seed), 1);
    const double rtp = simulateRtp(lucky3(), static_cast<std::uint64_t>(spins), rng, rec);
    const double needed = std::pow(1.96 * exact.stdDev / 0.001, 2);
    PlotScene p;
    p.title("How many spins for +/- 0.1%?").var("spins needed", static_cast<long long>(needed));
    rec.frame(p, cat("To pin the RTP within +/- 0.1 points (95% sure) you need about (1.96 x ", std::round(exact.stdDev * 100) / 100, " / 0.001)^2 = ", static_cast<long long>(needed), " spins. That's why studios compute RTP exactly when they can and run billions of simulated spins when they can't."), "");
    return {{"rtp", rtp}, {"exact", exact.rtp}};
}

// ------------------------------------------------------- cluster & cascade

Json clusterTrace(const Json& input, Recorder& rec) {
    const int seed = algo::intValue(input, "seed", 0, 1'000'000);
    Pcg32 rng(static_cast<std::uint64_t>(seed), 7);
    const Grid grid = randomGrid(6, 6, 4, rng);
    rec.frame(gemGrid(grid), "A 6x6 cluster-pays grid. Groups of 5 or more equal symbols touching up/down/left/right win.", "");
    // Visible BFS from every unvisited cell (same algorithm as findClusters).
    std::vector<std::vector<int>> state(6, std::vector<int>(6, 0));  // 0 new, 1 queued/visited
    std::vector<std::vector<bool>> winning(6, std::vector<bool>(6, false));
    const int dr[4] = {-1, 1, 0, 0}, dc[4] = {0, 0, -1, 1};
    int shown = 0;
    for (int r = 0; r < 6; ++r)
        for (int c = 0; c < 6; ++c) {
            if (state[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)]) continue;
            const int sym = grid[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)];
            std::vector<std::pair<int, int>> cluster, queue = {{r, c}};
            state[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)] = 1;
            for (std::size_t head = 0; head < queue.size(); ++head) {
                auto [cr, cc] = queue[head];
                cluster.push_back({cr, cc});
                for (int d = 0; d < 4; ++d) {
                    const int nr = cr + dr[d], nc = cc + dc[d];
                    if (nr < 0 || nr >= 6 || nc < 0 || nc >= 6) continue;
                    if (state[static_cast<std::size_t>(nr)][static_cast<std::size_t>(nc)] || grid[static_cast<std::size_t>(nr)][static_cast<std::size_t>(nc)] != sym) continue;
                    state[static_cast<std::size_t>(nr)][static_cast<std::size_t>(nc)] = 1;
                    queue.push_back({nr, nc});
                }
            }
            if (cluster.size() >= 3 && shown < 6) {  // animate only groups worth looking at
                ++shown;
                auto s = gemGrid(grid);
                for (auto [rr, cc] : cluster) s.mark(rr, cc, Mark::Visited);
                s.mark(r, c, Mark::Active).var("group size", static_cast<int>(cluster.size()));
                rec.frame(s, cat("BFS from row ", r, ", col ", c, " (", kGemLabels[static_cast<std::size_t>(sym)], "): visit it, queue its matching neighbours, visit them, repeat. Group size: ", cluster.size(), cluster.size() >= 5 ? ". That's a WIN." : ". Too small."), cluster.size() >= 5 ? "keep" : "visit");
            }
            if (cluster.size() >= 5)
                for (auto [rr, cc] : cluster) winning[static_cast<std::size_t>(rr)][static_cast<std::size_t>(cc)] = true;
        }
    const auto clusters = findClusters(grid, 5);
    auto final = gemGrid(grid);
    int cells = 0;
    for (const auto& cl : clusters)
        for (auto [rr, cc] : cl) {
            final.mark(rr, cc, Mark::Found);
            ++cells;
        }
    final.var("winning clusters", static_cast<int>(clusters.size())).var("cells", cells);
    rec.frame(final, clusters.empty() ? "No group reached 5. No win." : cat(clusters.size(), " winning cluster(s), ", cells, " cells. Every cell is visited exactly once: O(rows x cols)."), "keep");
    return {{"clusters", static_cast<int>(clusters.size())}, {"cells", cells}};
}

Json cascadeTrace(const Json& input, Recorder& rec) {
    const int seed = algo::intValue(input, "seed", 0, 1'000'000);
    Pcg32 rng(static_cast<std::uint64_t>(seed), 7);
    Grid grid = randomGrid(6, 6, 4, rng);
    rec.frame(gemGrid(grid), "Start of the spin.", "");
    int round = 0;
    int removed = 0;
    while (round < 6) {
        const auto clusters = findClusters(grid, 5);
        if (clusters.empty()) break;
        ++round;
        auto s = gemGrid(grid);
        for (const auto& cl : clusters)
            for (auto [r, c] : cl) s.mark(r, c, Mark::Found);
        rec.frame(s, cat("Cascade ", round, ": ", clusters.size(), " winning cluster(s). They pay and then vanish."), "");
        for (const auto& cl : clusters)
            for (auto [r, c] : cl) {
                grid[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)] = -1;
                ++removed;
            }
        rec.frame(gemGrid(grid), "Gaps where the winners were.", "");
        const int empty = applyGravity(grid);
        rec.frame(gemGrid(grid), cat("Gravity: every column slides down. ", empty, " empty cells are left at the top."), "fall");
        for (auto& row : grid)
            for (int& cell : row)
                if (cell < 0) cell = static_cast<int>(uniformBelow(rng, 4));
        rec.frame(gemGrid(grid), "New symbols drop in from above. Check for wins again...", "");
    }
    rec.frame(gemGrid(grid), round == 0 ? "No clusters: the spin is over." : cat("No more clusters after ", round, " cascade(s). One paid spin produced ", round, " wins in a row: that's the excitement of cascading games, and a headache for RTP maths."), "");
    return {{"cascades", round}, {"removed", removed}};
}

// ------------------------------------------------------------ state machine

GraphScene machineScene(State current, const SlotMachine& m, State previous) {
    GraphScene g;
    const State all[] = {State::Idle, State::Spinning, State::Evaluating, State::WinShow, State::FreeSpins};
    for (State s : all) g.node(toString(s), toString(s), s == current ? Mark::Active : Mark::None);
    g.at("IDLE", 0, 0).at("SPINNING", 2, 0).at("EVALUATING", 4, 0).at("WIN_SHOW", 4, 2).at("FREE_SPINS", 0, 2);
    // Highlight the transition just taken.
    auto e = [&](State a, State b, const char* label) {
        const bool taken = a == previous && b == current && a != b;
        g.edge(toString(a), toString(b), label, true, taken ? Mark::Compare : Mark::None);
    };
    e(State::Idle, State::Spinning, "spin");
    e(State::Spinning, State::Evaluating, "stop");
    e(State::Evaluating, State::WinShow, "win");
    e(State::Evaluating, State::Idle, "no win");
    e(State::Evaluating, State::FreeSpins, "bonus left");
    e(State::WinShow, State::Idle, "done");
    e(State::WinShow, State::FreeSpins, "bonus left");
    e(State::FreeSpins, State::Spinning, "auto spin");
    g.var("credits", m.credits()).var("free spins", m.freeSpins()).var("last win", m.lastWin());
    return g;
}

std::string lineText(const ReelGame& g, const Screen& s) {
    return cat(g.symbols[static_cast<std::size_t>(s[1][0])].label, " ", g.symbols[static_cast<std::size_t>(s[1][1])].label, " ", g.symbols[static_cast<std::size_t>(s[1][2])].label);
}

Json machineTrace(Recorder& rec) {
    const ReelGame& g = lucky3();
    SlotMachine m(g, 10);
    std::vector<std::string> events;
    m.subscribe([&events](const std::string& e) { events.push_back(e); });  // Observer
    State prev = m.state();
    auto step = [&](const std::string& caption, const char* line) {
        // Evaluating is passed through inside reelsStopped(); show where we came from.
        rec.frame(machineScene(m.state(), m, prev), caption, line);
        prev = m.state();
    };
    step("The machine waits in IDLE with 10 credits.", "");
    // Preset reel stops, so this short session visits every state:
    // miss, cherries, three bells (bonus), then 3 free spins: miss, lemons, miss.
    const std::vector<std::vector<int>> script = {{1, 2, 3}, {0, 13, 7}, {2, 3, 9}, {1, 2, 3}, {1, 0, 2}, {0, 0, 0}};
    bool showedIgnore = false;
    for (const auto& stops : script) {
        if (m.state() == State::Idle) {
            m.pressSpin(1);
            step("SPIN pressed: 1 credit is taken and the reels start.", "to_spinning");
            if (!showedIgnore) {
                showedIgnore = true;
                const bool accepted = m.pressSpin(1);
                rec.frame(machineScene(m.state(), m, m.state()), accepted ? "Unexpected: a second spin was accepted." : "The player taps SPIN again while the reels move. pressSpin returns false: the event is ignored in this state.", "ignore");
            }
        } else if (m.state() == State::FreeSpins) {
            m.pressSpin(1);
            step(cat("A free spin starts automatically. Free spins left after this one: ", m.freeSpins(), ". No credits taken."), "free_spin");
        }
        const std::size_t before = events.size();
        prev = State::Evaluating;
        m.reelsStopped(stops);
        bool bonus = false;
        for (std::size_t i = before; i < events.size(); ++i) bonus = bonus || events[i].find("bonus") != std::string::npos;
        if (m.state() == State::WinShow) {
            step(cat("Reels stopped on ", lineText(g, m.screen()), ". EVALUATING found a win of ", m.lastWin(), ".", bonus ? " Three bells also trigger 3 FREE SPINS." : ""), bonus ? "bonus" : "to_win");
            m.winShown();
            step(m.state() == State::FreeSpins ? "The celebration ends, and free spins are waiting." : "The celebration ends. Back to IDLE.", "finish");
        } else {
            step(cat("Reels stopped on ", lineText(g, m.screen()), ". EVALUATING found no win."), "finish");
        }
    }
    Json log = events;
    return {{"credits", m.credits()}, {"events", log}};
}

}  // namespace

void registerSlotLab(algo::Registry& r) {
    auto none = std::vector<Field>{};
    const Json noInput = Json::object();
    r.add({"slot.modulo_bias", "Modulo bias", "Slot Math Lab", "slot.uniform_below", "", "",
           "Why rng() % n is unfair, and the two-line fix every certified game uses.", none, noInput},
          [](const Json&, Recorder& rec) { return moduloBias(rec); });
    r.add({"slot.rng_quality", "Testing RNGs: chi-square", "Slot Math Lab", "slot.rng_pcg", "", "",
           "Histograms, the chi-square test, and the flaw a histogram can't see.",
           {{"draws", "int", "Draws per generator"}}, {{"draws", 100000}}},
          rngQuality);
    r.add({"slot.pick_linear", "Weighted pick: linear scan", "Slot Math Lab", "slot.pick_linear", "O(n)", "O(1)",
           "Walk the weights, subtracting, until the random number lands in a slice.",
           {{"weights", "int_array", "Weights"}, {"r", "int", "Random number (0 to total-1)"}},
           {{"weights", {40, 30, 20, 10}}, {"r", 75}}},
          pickLinearTrace);
    r.add({"slot.pick_binary", "Weighted pick: binary search", "Slot Math Lab", "slot.pick_binary", "O(log n)", "O(n)",
           "Store running totals once, then binary-search each random number.",
           {{"weights", "int_array", "Weights"}, {"r", "int", "Random number (0 to total-1)"}},
           {{"weights", {40, 30, 20, 10}}, {"r", 75}}},
          pickBinaryTrace);
    r.add({"slot.alias", "Weighted pick: alias method", "Slot Math Lab", "slot.alias", "O(1) per pick", "O(n)",
           "Vose's alias method: every pick is one column plus one coin flip.",
           {{"weights", "int_array", "Weights"}}, {{"weights", {40, 30, 20, 10}}}},
          aliasTrace);
    r.add({"slot.reel_window", "Reel strips and the window", "Slot Math Lab", "slot.window", "", "",
           "A reel strip is a circular array; % wraps it around.",
           {{"stops", "int_array", "Stop per reel (0-19)"}}, {{"stops", {3, 11, 18}}}},
          reelWindowTrace);
    r.add({"slot.payline", "Evaluating a payline", "Slot Math Lab", "slot.payline", "", "",
           "Read the line left to right and count the chain.",
           {{"stops", "int_array", "Stop per reel (0-19)"}}, {{"stops", {0, 13, 7}}}},
          paylineTrace);
    r.add({"slot.ways", "243 ways to win", "Slot Math Lab", "slot.ways", "", "",
           "No paylines: multiply matching positions per reel.",
           {{"stops", "int_array", "Stop per reel (0-14)"}}, {{"stops", {0, 0, 3, 1, 0}}}},
          waysTrace);
    r.add({"slot.rtp_exact", "RTP, exactly", "Slot Math Lab", "slot.rtp_exact", "", "",
           "Count every possible spin to get the true Return To Player, hit frequency and volatility.", none, noInput},
          [](const Json&, Recorder& rec) { return rtpExactTrace(rec); });
    r.add({"slot.rtp_sim", "RTP by simulation", "Slot Math Lab", "slot.rtp_sim", "", "",
           "Monte Carlo: watch a simulated RTP converge, and see how many spins accuracy costs.",
           {{"spins", "int", "Spins (100-500000)"}, {"seed", "int", "Seed"}}, {{"spins", 100000}, {"seed", 2026}}},
          rtpSimTrace);
    r.add({"slot.cluster", "Cluster pays (flood fill)", "Slot Math Lab", "slot.flood_fill", "O(rows x cols)", "O(rows x cols)",
           "Find touching groups of equal symbols with breadth-first search.",
           {{"seed", "int", "Seed"}}, {{"seed", 9}}},
          clusterTrace);
    r.add({"slot.cascade", "Cascades (tumbling reels)", "Slot Math Lab", "slot.gravity", "", "",
           "Winners vanish, symbols fall, new ones drop in, repeat.",
           {{"seed", "int", "Seed"}}, {{"seed", 4}}},
          cascadeTrace);
    r.add({"slot.state_machine", "Game flow as a state machine", "Slot Math Lab", "slot.state_machine", "", "",
           "IDLE, SPINNING, EVALUATING, WIN_SHOW, FREE_SPINS: and the events allowed in each.", none, noInput},
          [](const Json&, Recorder& rec) { return machineTrace(rec); });
}

}  // namespace forge::slot
