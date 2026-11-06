#pragma once
// "How AI works", from scratch. Tiny on purpose: every number fits on screen.
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace forge::ai {

// ---- gradient descent on f(x) = (x - 3)^2 + 1 ----
double f(double x);
double slope(double x);  // f'(x)
std::vector<double> gradientDescent(double start, double learningRate, int steps);

// ---- a 2-3-1 neural network learning XOR ----
struct Net {
    double w1[3][2];  // input -> hidden weights
    double b1[3];     // hidden biases
    double w2[3];     // hidden -> output weights
    double b2;        // output bias
};
Net randomNet(std::uint64_t seed);
double sigmoid(double z);
double predict(const Net& n, double x0, double x1, double hidden[3] = nullptr);
// One gradient step on one example; returns that example's loss before the step.
double trainStep(Net& n, const double x[2], double target, double lr);

// ---- a character bigram language model ----
class Bigram {
public:
    void train(const std::string& text);
    // Counts of each next character after `c` (only letters a-z and space are modelled).
    const std::array<std::uint32_t, 27>& next(char c) const;
    // Weighted pick of the next character, given a uniform random number r in [0, total).
    char sample(char current, std::uint64_t r) const;
    std::uint32_t total(char current) const;
    static int index(char c);  // 'a'..'z' -> 0..25, everything else -> 26 (space)
    static char symbol(int i);

private:
    std::array<std::array<std::uint32_t, 27>, 27> counts_{};
};

// The small training text, written for this app.
const std::string& corpus();

}  // namespace forge::ai
