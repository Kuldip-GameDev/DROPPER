// Exercise 05: reverse a singly linked list.
//
// Each node points to the next one. Make each one point to the previous one
// instead, and return the new head.
//
//   1 -> 2 -> 3 -> null     becomes     3 -> 2 -> 1 -> null
//
// Hint: you need three pointers: prev, curr and next. The order matters:
//   1. save curr->next   (or you lose the rest of the list!)
//   2. flip curr->next to prev
//   3. move prev and curr one step forward
// Watch "Reverse a linked list" in the app first.
#include <doctest/doctest.h>

#include <vector>

struct Node {
    int value = 0;
    Node* next = nullptr;
};

Node* reverseList(Node* head) {
    // TODO
    return head;
}

namespace {
// The vector OWNS the nodes; the list only links them with raw pointers.
Node* build(std::vector<Node>& pool) {
    for (std::size_t i = 0; i + 1 < pool.size(); ++i) pool[i].next = &pool[i + 1];
    return pool.empty() ? nullptr : &pool[0];
}
std::vector<int> values(Node* head) {
    std::vector<int> out;
    for (Node* p = head; p != nullptr; p = p->next) out.push_back(p->value);
    return out;
}
}  // namespace

TEST_CASE("reverse three nodes") {
    std::vector<Node> pool = {{1}, {2}, {3}};
    CHECK(values(reverseList(build(pool))) == std::vector<int>{3, 2, 1});
}

TEST_CASE("reverse empty and single") {
    CHECK(reverseList(nullptr) == nullptr);
    std::vector<Node> one = {{9}};
    Node* head = reverseList(build(one));
    REQUIRE(head != nullptr);
    CHECK(head->value == 9);
    CHECK(head->next == nullptr);
}
