#include "forge/ai/ai.hpp"

#include <cctype>
#include <cmath>
#include <numeric>

#include "forge/slot/rng.hpp"

namespace forge::ai {

double f(double x) { return (x - 3) * (x - 3) + 1; }
double slope(double x) { return 2 * (x - 3); }

// [snippet:ai.gradient_descent]
// To find the lowest point of a curve: look at the slope where you stand,
// take a step DOWNHILL, repeat. The learning rate is the step size.
std::vector<double> gradientDescent(double start, double learningRate, int steps) {
    std::vector<double> xs = {start};
    double x = start;
    for (int i = 0; i < steps; ++i) {
        x -= learningRate * slope(x);  // slope > 0: go left; slope < 0: go right  // @line step
        xs.push_back(x);
    }
    return xs;
}
// [/snippet]

double sigmoid(double z) { return 1.0 / (1.0 + std::exp(-z)); }

Net randomNet(std::uint64_t seed) {
    slot::Pcg32 rng(seed, 3);
    auto r = [&rng] { return (static_cast<double>(rng.next()) / 4294967296.0) * 2.0 - 1.0; };  // [-1, 1)
    Net n{};
    for (auto& row : n.w1) for (double& w : row) w = r();
    for (double& b : n.b1) b = r();
    for (double& w : n.w2) w = r();
    n.b2 = r();
    return n;
}

double predict(const Net& n, double x0, double x1, double hidden[3]) {
    double h[3];
    for (int j = 0; j < 3; ++j) h[j] = sigmoid(n.w1[j][0] * x0 + n.w1[j][1] * x1 + n.b1[j]);
    if (hidden) for (int j = 0; j < 3; ++j) hidden[j] = h[j];
    return sigmoid(n.w2[0] * h[0] + n.w2[1] * h[1] + n.w2[2] * h[2] + n.b2);
}

// [snippet:ai.backprop]
// One training step = forward pass (make a guess) + backward pass
// (work out how much each weight is to blame, nudge it the other way).
double trainStep(Net& n, const double x[2], double target, double lr) {
    double h[3];
    for (int j = 0; j < 3; ++j) {
        h[j] = sigmoid(n.w1[j][0] * x[0] + n.w1[j][1] * x[1] + n.b1[j]);  // hidden layer  // @line hidden
    }
    const double out = sigmoid(n.w2[0] * h[0] + n.w2[1] * h[1] + n.w2[2] * h[2] + n.b2);  // @line output
    // Chain rule: error times the slope of the sigmoid, sigmoid(z)' = s * (1 - s).
    const double dOut = (out - target) * out * (1 - out);  // @line delta_out
    for (int j = 0; j < 3; ++j) {
        const double dHidden = dOut * n.w2[j] * h[j] * (1 - h[j]);  // blame flows back  // @line delta_hidden
        n.w2[j] -= lr * dOut * h[j];  // gradient descent on every weight
        n.w1[j][0] -= lr * dHidden * x[0];
        n.w1[j][1] -= lr * dHidden * x[1];
        n.b1[j] -= lr * dHidden;
    }
    n.b2 -= lr * dOut;
    return 0.5 * (out - target) * (out - target);
}
// [/snippet]

int Bigram::index(char c) {
    const auto u = static_cast<unsigned char>(c);
    if (std::isalpha(u)) return std::tolower(u) - 'a';
    return 26;
}

char Bigram::symbol(int i) { return i == 26 ? ' ' : static_cast<char>('a' + i); }

// [snippet:ai.bigram]
// A language model in its simplest form: count which character follows
// which. "Predict the next character" = a weighted pick from those counts.
void Bigram::train(const std::string& text) {
    for (std::size_t i = 0; i + 1 < text.size(); ++i) {
        ++counts_[index(text[i])][index(text[i + 1])];  // "after this, came that"  // @line count
    }
}

char Bigram::sample(char current, std::uint64_t r) const {
    const auto& row = counts_[index(current)];
    for (int i = 0; i < 27; ++i) {  // the same linear weighted pick as a slot reel
        if (r < row[i]) return symbol(i);  // @line pick
        r -= row[i];
    }
    return ' ';
}
// [/snippet]

const std::array<std::uint32_t, 27>& Bigram::next(char c) const { return counts_[index(c)]; }

std::uint32_t Bigram::total(char current) const {
    const auto& row = counts_[index(current)];
    return std::accumulate(row.begin(), row.end(), 0u);
}

const std::string& corpus() {
    static const std::string text =
        "the reels spin and the player waits for the lights to settle on a line "
        "a good game feels fair because the maths is fair and the code is tested "
        "every spin is random and every result is counted so the return to player "
        "is the same for everyone the designer sets the weights and the engineer "
        "writes the code that turns a random number into a symbol on the screen "
        "learning to code means reading code writing code and testing code again "
        "pointers point at memory and lambdas capture what they need to remember "
        "a tree has roots and leaves a graph has nodes and edges and a heap keeps "
        "the smallest thing at the top so the next thing to do is always ready ";
    return text;
}

}  // namespace forge::ai
