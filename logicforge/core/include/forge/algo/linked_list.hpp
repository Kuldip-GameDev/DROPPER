#pragma once
#include <vector>

#include "forge/viz/recorder.hpp"

namespace forge::algo {

// A singly linked list node. `next` is a NON-OWNING raw pointer: some
// other object (here, a std::vector<ListNode>) owns the memory. Knowing
// who owns memory is the most important question in C++.
struct ListNode {
    int value = 0;
    ListNode* next = nullptr;
};

ListNode* reverseList(ListNode* head, viz::Recorder& rec);
bool hasCycle(ListNode* head, viz::Recorder& rec);

// Test/visual helper: link pool[i] -> pool[i+1]; if cycleTo >= 0 the last
// node points back to pool[cycleTo]. Returns the head (or nullptr).
ListNode* linkPool(std::vector<ListNode>& pool, int cycleTo = -1);

}  // namespace forge::algo
