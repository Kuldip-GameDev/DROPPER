#include <doctest/doctest.h>

#include <cmath>
#include <map>

#include "forge/ai/ai.hpp"

using namespace forge::ai;

TEST_CASE("gradient descent: converges, oscillates or diverges depending on the step size") {
    CHECK(std::fabs(gradientDescent(-6, 0.2, 30).back() - 3) < 1e-3);
    const auto osc = gradientDescent(-6, 1.0, 6);  // x - 3 flips sign, same size
    for (double x : osc) CHECK(std::fabs(std::fabs(x - 3) - 9) < 1e-9);
    const auto div = gradientDescent(-6, 1.1, 6);
    CHECK(std::fabs(div.back() - 3) > std::fabs(div.front() - 3));
}

TEST_CASE("backprop matches a numerical gradient (finite differences)") {
    Net n = randomNet(42);
    const double x[2] = {1, 0};
    const double target = 1;
    auto loss = [&](const Net& m) {
        const double o = predict(m, x[0], x[1]);
        return 0.5 * (o - target) * (o - target);
    };
    // Numerical slope of the loss for a few weights.
    auto numeric = [&](auto get) {
        const double h = 1e-6;
        Net plus = n, minus = n;
        get(plus) += h;
        get(minus) -= h;
        return (loss(plus) - loss(minus)) / (2 * h);
    };
    const double gB2 = numeric([](Net& m) -> double& { return m.b2; });
    const double gW2 = numeric([](Net& m) -> double& { return m.w2[1]; });
    const double gW1 = numeric([](Net& m) -> double& { return m.w1[2][0]; });
    // One step with lr = 1 moves each weight by exactly -gradient.
    Net stepped = n;
    trainStep(stepped, x, target, 1.0);
    CHECK(n.b2 - stepped.b2 == doctest::Approx(gB2).epsilon(1e-5));
    CHECK(n.w2[1] - stepped.w2[1] == doctest::Approx(gW2).epsilon(1e-5));
    CHECK(n.w1[2][0] - stepped.w1[2][0] == doctest::Approx(gW1).epsilon(1e-5));
}

TEST_CASE("the network learns XOR from several random starts") {
    const double X[4][2] = {{0, 0}, {0, 1}, {1, 0}, {1, 1}};
    const double Y[4] = {0, 1, 1, 0};
    for (std::uint64_t seed = 1; seed <= 5; ++seed) {
        Net n = randomNet(seed);
        for (int e = 0; e < 5000; ++e)
            for (int i = 0; i < 4; ++i) trainStep(n, X[i], Y[i], 1.0);
        for (int i = 0; i < 4; ++i) CHECK(std::fabs(predict(n, X[i][0], X[i][1]) - Y[i]) < 0.1);
    }
}

TEST_CASE("bigram counts and sampling are consistent") {
    Bigram m;
    m.train(corpus());
    std::uint64_t total = 0;
    for (int i = 0; i < 27; ++i) total += m.total(Bigram::symbol(i));
    CHECK(total == corpus().size() - 1);
    // Every r in [0, total) maps to a character exactly as often as its count.
    const char c = 't';
    std::map<char, std::uint32_t> seen;
    for (std::uint32_t r = 0; r < m.total(c); ++r) ++seen[m.sample(c, r)];
    for (int i = 0; i < 27; ++i) {
        const auto expected = m.next(c)[static_cast<std::size_t>(i)];
        CHECK(seen[Bigram::symbol(i)] == expected);
    }
    CHECK(Bigram::index('Q') == 16);
    CHECK(Bigram::index('!') == 26);
}
