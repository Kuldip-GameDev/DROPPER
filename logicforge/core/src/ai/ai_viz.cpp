// Visual traces for "How AI works".
#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "forge/ai/ai.hpp"
#include "forge/algo/input.hpp"
#include "forge/algo/registry.hpp"
#include "forge/slot/rng.hpp"
#include "forge/util/cat.hpp"
#include "forge/viz/scene.hpp"

namespace forge::ai {

using viz::GraphScene;
using viz::Mark;
using viz::PlotScene;
using viz::Recorder;

namespace {

double round2(double v) { return std::round(v * 100) / 100; }

Json gradientTrace(const Json& input, Recorder& rec) {
    const double lr = algo::intValue(input, "lr", 1, 120) / 100.0;
    const double start = algo::intValue(input, "start", -10, 10);
    const auto xs = gradientDescent(start, lr, 12);
    const double lo = std::min({start, 3.0, *std::min_element(xs.begin(), xs.end())}) - 2;
    const double hi = std::max({start, 3.0, *std::max_element(xs.begin(), xs.end())}) + 2;
    std::vector<std::pair<double, double>> curve;
    for (int i = 0; i <= 40; ++i) {
        const double x = lo + (hi - lo) * i / 40.0;
        curve.emplace_back(x, f(x));
    }
    std::vector<std::pair<double, double>> path;
    for (std::size_t i = 0; i < xs.size(); ++i) {
        path.emplace_back(xs[i], f(xs[i]));
        PlotScene p;
        p.title("f(x) = (x - 3)^2 + 1").axes("x", "f(x)").line("f", curve, Mark::Dim).line("steps", path, Mark::Active).reference(1, "lowest point");
        p.var("x", round2(xs[i])).var("slope", round2(slope(xs[i]))).var("learning rate", lr);
        const bool exploding = std::fabs(xs[i] - 3) > std::fabs(start - 3) + 1e-9 && i > 0;
        rec.frame(p, i == 0 ? cat("Start at x = ", round2(xs[0]), ". The slope there is ", round2(slope(xs[0])), ".")
                            : cat("Step ", i, ": x -= ", lr, " x slope, giving x = ", round2(xs[i]), ". ",
                                  exploding ? "The steps are too big and overshoot further each time: training DIVERGES." : std::fabs(xs[i] - 3) < 0.05 ? "Almost at the bottom (x = 3)." : "Downhill again."),
                  i == 0 ? "" : "step");
    }
    return {{"final", xs.back()}};
}

GraphScene netScene(const Net& n, double x0, double x1) {
    double h[3];
    const double out = predict(n, x0, x1, h);
    GraphScene g;
    g.node("x1", "x1").node("x2", "x2").node("h1", "h1").node("h2", "h2").node("h3", "h3").node("out", "out", Mark::Active);
    g.at("x1", 0, 0.5).at("x2", 0, 1.5).at("h1", 2, 0).at("h2", 2, 1).at("h3", 2, 2).at("out", 4, 1);
    g.sub("x1", cat("= ", x0)).sub("x2", cat("= ", x1)).sub("out", cat("= ", round2(out)));
    const char* hid[3] = {"h1", "h2", "h3"};
    for (int j = 0; j < 3; ++j) {
        g.sub(hid[j], cat("= ", round2(h[j])));
        g.edge("x1", hid[j], cat(round2(n.w1[j][0])));
        g.edge("x2", hid[j], cat(round2(n.w1[j][1])));
        g.edge(hid[j], "out", cat(round2(n.w2[j])));
    }
    return g;
}

const double kX[4][2] = {{0, 0}, {0, 1}, {1, 0}, {1, 1}};
const double kY[4] = {0, 1, 1, 0};

Json xorTrace(const Json& input, Recorder& rec) {
    const int epochs = algo::intValue(input, "epochs", 10, 20000);
    const double lr = algo::intValue(input, "lr", 1, 300) / 100.0;
    const int seed = algo::intValue(input, "seed", 0, 1'000'000);
    Net n = randomNet(static_cast<std::uint64_t>(seed));
    auto predictions = [&](GraphScene g) {
        for (int i = 0; i < 4; ++i) g.var(cat(kX[i][0], " xor ", kX[i][1]), round2(predict(n, kX[i][0], kX[i][1])));
        return g;
    };
    rec.frame(predictions(netScene(n, 1, 0)), "A brand-new network with random weights (edge labels). It knows nothing: every answer is a guess near 0.5. The node values shown are for the input (1, 0).", "");
    std::vector<std::pair<double, double>> lossCurve;
    int nextShow = 10;
    for (int e = 1; e <= epochs; ++e) {
        double loss = 0;
        for (int i = 0; i < 4; ++i) loss += trainStep(n, kX[i], kY[i], lr);
        if (e % std::max(1, epochs / 60) == 0 || e == 1) lossCurve.emplace_back(e, loss);
        if (e == nextShow || e == epochs) {
            rec.frame(predictions(netScene(n, 1, 0)), cat("After ", e, " passes over the 4 examples, total loss = ", round2(loss), ". ", loss < 0.05 ? "Every answer is close to the truth: it LEARNED xor." : "The weights keep shifting towards the right answers."), "delta_hidden");
            nextShow *= 4;
        }
    }
    PlotScene p;
    p.title("loss (how wrong) over training").axes("passes over the data", "loss").line("loss", lossCurve, Mark::Active);
    rec.frame(p, "The loss curve: big at first, then it drops as gradient descent tunes all 13 numbers at once. Large models do exactly this, with billions of numbers.", "");
    double finalLoss = 0;
    for (int i = 0; i < 4; ++i) {
        const double o = predict(n, kX[i][0], kX[i][1]);
        finalLoss += 0.5 * (o - kY[i]) * (o - kY[i]);
    }
    return {{"loss", finalLoss}};
}

Json bigramTrace(const Json& input, Recorder& rec) {
    const auto startText = algo::textValue(input, "start", 1);
    const int length = algo::intValue(input, "length", 5, 80);
    const int seed = algo::intValue(input, "seed", 0, 1'000'000);
    Bigram model;
    model.train(corpus());
    char current = startText.empty() ? 't' : startText[0];
    if (Bigram::index(current) == 26 && current != ' ') current = 't';
    auto dist = [&](char c, char picked) {
        const auto& row = model.next(c);
        std::vector<int> order(27);
        for (int i = 0; i < 27; ++i) order[static_cast<std::size_t>(i)] = i;
        std::sort(order.begin(), order.end(), [&row](int a, int b) { return row[static_cast<std::size_t>(a)] > row[static_cast<std::size_t>(b)]; });
        PlotScene p;
        p.title(cat("what follows '", c == ' ' ? std::string("space") : std::string(1, c), "' in the training text")).axes("next character", "count");
        for (int k = 0; k < 8; ++k) {
            const int i = order[static_cast<std::size_t>(k)];
            if (row[static_cast<std::size_t>(i)] == 0) break;
            const char s = Bigram::symbol(i);
            p.bar(s == ' ' ? "␣" : std::string(1, s), row[static_cast<std::size_t>(i)], s == picked ? Mark::Found : Mark::None);
        }
        return p;
    };
    rec.frame(dist(current, '\0'), cat("Training = counting. The model read ", corpus().size(), " characters and counted which character follows which. Here are the counts after '", current, "'."), "count");
    slot::Pcg32 rng(static_cast<std::uint64_t>(seed), 9);
    std::string out(1, current);
    for (int i = 0; i < length; ++i) {
        const auto total = model.total(current);
        if (total == 0) break;
        const char next = model.sample(current, slot::uniformBelow(rng, total));
        if (i < 8) {
            auto p = dist(current, next);
            p.var("text so far", out + next);
            rec.frame(p, cat("Pick the next character at random, weighted by the counts (a weighted pick, like a slot reel): '", next == ' ' ? std::string("space") : std::string(1, next), "'."), "pick");
        }
        out += next;
        current = next;
    }
    PlotScene done;
    done.title("generated text").var("output", out);
    rec.frame(done, cat("\"", out, "\". Word-like pieces, no meaning: it only ever looks ONE character back. An LLM is the same game (predict the next token, sample it) with a neural network that looks thousands of tokens back."), "pick");
    return {{"text", out}};
}

}  // namespace

void registerAi(algo::Registry& r) {
    r.add({"ai.gradient_descent", "Gradient descent", "How AI works", "ai.gradient_descent", "", "",
           "How machines \"learn\": measure the slope of the error and step downhill. Try a learning rate of 105.",
           {{"lr", "int", "Learning rate x100 (1-120)"}, {"start", "int", "Start x (-10..10)"}},
           {{"lr", 20}, {"start", -6}}},
          gradientTrace);
    r.add({"ai.xor_network", "A neural network learns XOR", "How AI works", "ai.backprop", "", "",
           "A 2-3-1 network trained with backpropagation, every weight visible.",
           {{"epochs", "int", "Training passes"}, {"lr", "int", "Learning rate x100"}, {"seed", "int", "Random seed"}},
           {{"epochs", 5000}, {"lr", 100}, {"seed", 7}}},
          xorTrace);
    r.add({"ai.bigram", "A tiny language model", "How AI works", "ai.bigram", "", "",
           "Count what follows what, then generate text one weighted pick at a time.",
           {{"start", "text", "First letter"}, {"length", "int", "Characters to generate"}, {"seed", "int", "Random seed"}},
           {{"start", "t"}, {"length", 60}, {"seed", 3}}},
          bigramTrace);
}

}  // namespace forge::ai
