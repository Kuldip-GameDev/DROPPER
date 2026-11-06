#include "forge/algo/linked_list.hpp"

#include <algorithm>
#include <functional>
#include <initializer_list>
#include <set>
#include <utility>

#include "forge/algo/input.hpp"
#include "forge/algo/registry.hpp"
#include "forge/util/cat.hpp"
#include "forge/viz/scene.hpp"

namespace forge::algo {

using viz::GraphScene;
using viz::Mark;
using viz::Recorder;

namespace {

using Named = std::pair<const char*, ListNode*>;

// Draw every node reachable from any of the named pointers.
// Nodes live in one contiguous array (a vector), so "node - base" gives a
// stable index for each node. Subtracting pointers is only legal inside
// the same array, which is exactly our situation.
GraphScene listScene(std::initializer_list<Named> named) {
    std::set<ListNode*, std::less<>> nodes;  // std::less<> orders any pointers safely
    for (const auto& [name, start] : named) {
        for (ListNode* p = start; p != nullptr && !nodes.contains(p); p = p->next) nodes.insert(p);
    }
    GraphScene g;
    if (nodes.empty()) return g;
    ListNode* base = *nodes.begin();
    auto id = [base](const ListNode* p) { return "n" + std::to_string(p - base); };

    for (ListNode* p : nodes) {
        g.node(id(p), std::to_string(p->value));
        g.at(id(p), static_cast<double>(p - base), 0);
        if (p->next == nullptr) g.sub(id(p), "next = null");
    }
    for (ListNode* p : nodes) {
        if (p->next != nullptr) g.edge(id(p), id(p->next));
    }
    for (const auto& [name, ptr] : named) {
        if (ptr != nullptr) {
            g.pointer(name, id(ptr));
        } else {
            g.var(name, "nullptr");
        }
    }
    return g;
}

}  // namespace

ListNode* linkPool(std::vector<ListNode>& pool, int cycleTo) {
    if (pool.empty()) return nullptr;
    for (std::size_t i = 0; i + 1 < pool.size(); ++i) pool[i].next = &pool[i + 1];
    pool.back().next = nullptr;
    if (cycleTo >= 0 && cycleTo < static_cast<int>(pool.size())) {
        pool.back().next = &pool[static_cast<std::size_t>(cycleTo)];
    }
    return &pool[0];
}

// [snippet:list.reverse]
ListNode* reverseList(ListNode* head, Recorder& rec) {
    ListNode* prev = nullptr;  // the reversed part starts empty
    ListNode* curr = head;     // the part still to reverse
    while (curr != nullptr) {
        ListNode* next = curr->next;  // 1. remember the rest of the list  // @line save_next
        rec.frame(listScene({{"prev", prev}, {"curr", curr}, {"next", next}}), "Save curr->next before we break that link.", "save_next"); // @viz
        curr->next = prev;  // 2. flip the arrow backwards  // @line flip
        rec.frame(listScene({{"prev", prev}, {"curr", curr}, {"next", next}}), cat("Flip: node ", curr->value, " now points back to ", prev ? std::to_string(prev->value) : "nullptr", "."), "flip"); // @viz
        prev = curr;  // 3. move prev forward  // @line advance_prev
        curr = next;  // 4. move curr forward  // @line advance_curr
        rec.frame(listScene({{"prev", prev}, {"curr", curr}}), "Step both pointers forward.", "advance_curr"); // @viz
    }
    rec.frame(listScene({{"head", prev}}), "curr fell off the end. prev is the new head.", "done"); // @viz
    return prev;  // the old tail is the new head  // @line done
}
// [/snippet]

// [snippet:list.cycle]
// Floyd's "tortoise and hare": if there is a loop, the fast pointer
// eventually laps the slow one, like runners on a circular track.
bool hasCycle(ListNode* head, Recorder& rec) {
    ListNode* slow = head;
    ListNode* fast = head;
    while (fast != nullptr && fast->next != nullptr) {
        slow = slow->next;        // 1 step  // @line slow_step
        fast = fast->next->next;  // 2 steps  // @line fast_step
        rec.frame(listScene({{"head", head}, {"slow", slow}, {"fast", fast}}), slow == fast ? "slow and fast are on the same node: there is a loop!" : "slow moves 1 step, fast moves 2 steps.", slow == fast ? "met" : "fast_step"); // @viz
        if (slow == fast) return true;  // they met: there is a loop  // @line met
    }
    rec.frame(listScene({{"head", head}, {"slow", slow}, {"fast", fast}}), "fast reached the end (nullptr), so there is no loop.", "no_cycle"); // @viz
    return false;  // fast reached the end: no loop  // @line no_cycle
}
// [/snippet]

void registerLinkedList(Registry& r) {
    r.add({"list.reverse", "Reverse a linked list", "Linked lists", "list.reverse", "O(n)", "O(1)",
           "Walk the list once, flipping each arrow to point backwards. Three pointers do all the work.",
           {{"array", "int_array", "Node values"}},
           {{"array", {1, 2, 3, 4, 5}}}},
          [](const Json& input, Recorder& rec) -> Json {
              auto values = intArray(input, "array", 12);
              std::vector<ListNode> pool(values.size());  // owns every node
              for (std::size_t i = 0; i < values.size(); ++i) pool[i].value = values[i];
              ListNode* head = linkPool(pool);
              rec.frame(listScene({{"head", head}}), "A linked list: each node points to the next one.", "");
              ListNode* newHead = reverseList(head, rec);
              Json out = Json::array();
              for (ListNode* p = newHead; p != nullptr; p = p->next) out.push_back(p->value);
              return {{"reversed", out}};
          });

    r.add({"list.cycle", "Detect a loop (Floyd)", "Linked lists", "list.cycle", "O(n)", "O(1)",
           "Send a slow pointer and a fast pointer. If they ever meet, the list loops.",
           {{"array", "int_array", "Node values"}, {"cycleTo", "int", "Last node points to (-1 = none)"}},
           {{"array", {3, 2, 0, -4, 7, 9}}, {"cycleTo", 1}}},
          [](const Json& input, Recorder& rec) -> Json {
              auto values = intArray(input, "array", 12);
              const int n = static_cast<int>(values.size());
              const int cycleTo = intValue(input, "cycleTo", -1, std::max(n - 1, -1));
              std::vector<ListNode> pool(values.size());
              for (std::size_t i = 0; i < values.size(); ++i) pool[i].value = values[i];
              ListNode* head = linkPool(pool, cycleTo);
              rec.frame(listScene({{"head", head}}),
                        cycleTo >= 0 ? "The last node points back into the list, making a loop."
                                     : "A list that ends with nullptr.",
                        "");
              return {{"hasCycle", hasCycle(head, rec)}};
          });
}

}  // namespace forge::algo
