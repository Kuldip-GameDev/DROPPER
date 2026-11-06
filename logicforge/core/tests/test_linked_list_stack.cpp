#include <doctest/doctest.h>

#include <vector>

#include "forge/algo/linked_list.hpp"
#include "forge/algo/stack.hpp"

using namespace forge;

namespace {
std::vector<algo::ListNode> makePool(std::vector<int> values) {
    std::vector<algo::ListNode> pool(values.size());
    for (std::size_t i = 0; i < values.size(); ++i) pool[i].value = values[i];
    return pool;
}
std::vector<int> toVector(const algo::ListNode* head) {
    std::vector<int> out;
    for (auto* p = head; p != nullptr; p = p->next) out.push_back(p->value);
    return out;
}
}  // namespace

TEST_CASE("reverse linked list") {
    for (const auto& values : std::vector<std::vector<int>>{{}, {1}, {1, 2}, {1, 2, 3, 4, 5}}) {
        auto pool = makePool(values);
        viz::Recorder rec;
        auto* head = algo::reverseList(algo::linkPool(pool), rec);
        const std::vector<int> expected(values.rbegin(), values.rend());
        CHECK(toVector(head) == expected);
    }
}

TEST_CASE("Floyd cycle detection") {
    for (int n = 0; n <= 6; ++n) {
        std::vector<int> values(static_cast<std::size_t>(n), 0);
        for (int cycleTo = -1; cycleTo < n; ++cycleTo) {
            auto pool = makePool(values);
            viz::Recorder rec;
            CAPTURE(n);
            CAPTURE(cycleTo);
            CHECK(algo::hasCycle(algo::linkPool(pool, cycleTo), rec) == (cycleTo >= 0));
        }
    }
}

TEST_CASE("balanced brackets") {
    viz::Recorder rec;
    CHECK(algo::isBalanced("", rec));
    CHECK(algo::isBalanced("()[]{}", rec));
    CHECK(algo::isBalanced("{[()()]}", rec));
    CHECK(algo::isBalanced("a(b)c", rec));
    CHECK_FALSE(algo::isBalanced("(", rec));
    CHECK_FALSE(algo::isBalanced(")", rec));
    CHECK_FALSE(algo::isBalanced("(]", rec));
    CHECK_FALSE(algo::isBalanced("([)]", rec));
}
