// Exercise 06: ownership with std::unique_ptr.
//
// A bag OWNS its items. Nobody calls delete: unique_ptr does it for you.
//
// 1. craft(name, power)  -> create an Item on the heap with std::make_unique.
// 2. store(bag, item)    -> move the item into the bag. After this, the
//                           caller's pointer is empty (ownership moved).
// 3. totalPower(bag)     -> add up item->power for every item.
//
// Hints:
//   * A unique_ptr can't be copied, only moved: bag.push_back(std::move(item)).
//   * Use -> to reach a member through a pointer: item->power.
//   * Watch "unique_ptr and moving ownership" in the Pointer Lab first.
#include <doctest/doctest.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

struct Item {
    std::string name;
    int power = 0;
};

using Bag = std::vector<std::unique_ptr<Item>>;

std::unique_ptr<Item> craft(const std::string& name, int power) {
    // TODO
    (void)name;
    (void)power;
    return nullptr;
}

void store(Bag& bag, std::unique_ptr<Item> item) {
    // TODO
    (void)bag;
    (void)item;
}

int totalPower(const Bag& bag) {
    // TODO
    (void)bag;
    return -1;
}

TEST_CASE("craft makes a heap item") {
    auto sword = craft("sword", 7);
    REQUIRE(sword != nullptr);
    CHECK(sword->name == "sword");
    CHECK(sword->power == 7);
}

TEST_CASE("store moves ownership into the bag") {
    Bag bag;
    auto shield = craft("shield", 3);
    REQUIRE(shield != nullptr);
    Item* raw = shield.get();  // non-owning: just to check WHERE it went
    store(bag, std::move(shield));
    CHECK(shield == nullptr);  // moved-from unique_ptr is empty
    REQUIRE(bag.size() == 1);
    CHECK(bag[0].get() == raw);  // same object, new owner, no copy
}

TEST_CASE("totalPower adds every item") {
    Bag bag;
    CHECK(totalPower(bag) == 0);
    store(bag, craft("a", 2));
    store(bag, craft("b", 5));
    CHECK(totalPower(bag) == 7);
}
