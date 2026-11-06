// Pointer Lab scenarios.
//
// Every scenario below is ordinary C++ that really runs. The // @viz lines
// read the real variables and real pointer values to draw the picture.
// `out` is the scenario's console: it works exactly like std::cout.
#include <memory>
#include <ostream>

#include "forge/algo/registry.hpp"
#include "forge/memlab/lab.hpp"

namespace forge::memlab {

using viz::MemoryScene;

// ----------------------------------------------------------------------------
// [snippet:mem.address_of]
void addressOf() {
    auto frame = lab().enter("main");  // @viz
    std::ostream& out = lab().out;  // @viz
    int score = 42;  // @line declare
    lab().local("score", "int", score);  // @viz
    lab().snap("score is a box on the stack. It holds 42, and the box has an address.", "declare", {&score});  // @viz
    int* p = &score;  // p stores the ADDRESS of score  // @line take_address
    lab().pointer("p", "int*", p);  // @viz
    lab().snap("&score means \"the address of score\". p holds that address: p points to score.", "take_address", {&p});  // @viz
    out << *p << '\n';  // *p means "the int that p points to"  // @line read
    lab().snap("*p follows the arrow and reads the int at the end: 42.", "read", {&score});  // @viz
    *p = 100;  // write THROUGH the pointer  // @line write
    lab().snap("*p = 100 follows the arrow and writes there. score itself changed!", "write", {&score});  // @viz
    out << score << '\n';  // @line print
    lab().snap("score is 100, even though the code never says score = 100.", "print");  // @viz
}
// [/snippet]

// ----------------------------------------------------------------------------
// [snippet:mem.reference]
void reference() {
    auto frame = lab().enter("main");  // @viz
    int hp = 10;  // @line declare
    lab().local("hp", "int", hp);  // @viz
    lab().snap("hp is a normal int on the stack.", "declare", {&hp});  // @viz
    int& ref = hp;  // ref is ANOTHER NAME for hp, not a new box  // @line bind
    lab().alias("ref", &ref);  // @viz
    lab().snap("A reference gets no box of its own. hp and ref are two names for one int.", "bind", {&hp});  // @viz
    ref -= 3;  // this changes hp  // @line through_ref
    lab().snap("ref -= 3 changes hp, because they are the same int. hp is 7.", "through_ref", {&hp});  // @viz
    int* ptr = &hp;  // a pointer IS a separate box that holds an address  // @line pointer
    lab().pointer("ptr", "int*", ptr);  // @viz
    lab().snap("A pointer is different: it is its own variable, with its own address, holding hp's address.", "pointer", {&ptr});  // @viz
    *ptr -= 2;  // @line through_ptr
    lab().snap("*ptr -= 2 also changes hp. hp is 5.", "through_ptr", {&hp});  // @viz
    // A reference must be initialised, can't be null and can't be re-pointed.
    // A pointer can be nullptr and can be changed to point somewhere else.
}
// [/snippet]

// ----------------------------------------------------------------------------
// [snippet:mem.swap]
void swapByValue(int a, int b) {  // @line copy_args
    auto frame = lab().enter("swapByValue");  // @viz
    lab().local("a", "int", a); lab().local("b", "int", b);  // @viz
    lab().snap("Calling a function pushes a new stack frame. a and b are COPIES of x and y.", "copy_args");  // @viz
    int tmp = a;
    lab().local("tmp", "int", tmp);  // @viz
    a = b;
    b = tmp;  // @line swap_copies
    lab().snap("The copies are swapped... but x and y down in main are untouched.", "swap_copies", {&a, &b});  // @viz
}  // the frame, and the copies in it, disappear here

void swapByPointer(int* a, int* b) {  // @line receive_ptrs
    auto frame = lab().enter("swapByPointer");  // @viz
    lab().pointer("a", "int*", a); lab().pointer("b", "int*", b);  // @viz
    lab().snap("This time a and b hold the ADDRESSES of x and y.", "receive_ptrs");  // @viz
    int tmp = *a;
    lab().local("tmp", "int", tmp);  // @viz
    *a = *b;
    *b = tmp;  // @line swap_through
    lab().snap("Writing through the pointers changes x and y themselves.", "swap_through", {a, b});  // @viz
}

void passByDemo() {
    auto frame = lab().enter("main");  // @viz
    int x = 1, y = 2;  // @line start
    lab().local("x", "int", x); lab().local("y", "int", y);  // @viz
    lab().snap("main has x = 1 and y = 2. Let's try to swap them two ways.", "start");  // @viz
    swapByValue(x, y);  // @line call_value
    lab().snap("Back in main: x is still 1 and y is still 2. The swap happened to copies.", "call_value", {&x, &y});  // @viz
    swapByPointer(&x, &y);  // @line call_pointer
    lab().snap("x = 2, y = 1. Passing addresses let the function change main's variables.", "call_pointer", {&x, &y});  // @viz
}
// [/snippet]

// ----------------------------------------------------------------------------
// [snippet:mem.array_pointer]
void arrayPointer() {
    auto frame = lab().enter("main");  // @viz
    std::ostream& out = lab().out;  // @viz
    int reel[4] = {7, 3, 9, 5};  // four ints, side by side in memory  // @line declare
    lab().array("reel", "int", reel);  // @viz
    lab().snap(cat("An array is boxes in a row. Each int is ", sizeof(int), " bytes, so the addresses go up by ", sizeof(int), "."), "declare");  // @viz
    int* p = reel;  // the array name turns into a pointer to its FIRST element  // @line decay
    lab().pointer("p", "int*", p);  // @viz
    lab().snap("int* p = reel; makes p point at reel[0]. This is called array-to-pointer decay.", "decay", {p});  // @viz
    out << *p << '\n';  // 7  // @line read_first
    p = p + 2;  // moves 2 INTS forward, i.e. 2 * sizeof(int) bytes  // @line advance
    lab().snap(cat("p + 2 does not add 2 bytes. It adds 2 ints = ", 2 * sizeof(int), " bytes. Now p points at reel[2]."), "advance", {p});  // @viz
    out << *p << '\n';     // 9
    out << p[1] << '\n';   // p[1] means *(p + 1), which is reel[3] = 5  // @line index
    lab().snap("p[1] is just shorthand for *(p + 1): one int after p, which is reel[3].", "index", {p + 1});  // @viz
    // p + 2 would be "one past the end": you may point there, but never read it.
}
// [/snippet]

// ----------------------------------------------------------------------------
// [snippet:mem.pointer_to_pointer]
void pointerToPointer() {
    auto frame = lab().enter("main");  // @viz
    int lives = 3;
    int* p = &lives;  // @line p
    lab().local("lives", "int", lives); lab().pointer("p", "int*", p);  // @viz
    lab().snap("p points at lives, as usual.", "p");  // @viz
    int** pp = &p;  // pp points at p, which points at lives  // @line pp
    lab().pointer("pp", "int**", pp);  // @viz
    lab().snap("int** is a pointer to a pointer. pp holds the address of p.", "pp", {&pp});  // @viz
    **pp = 5;  // follow TWO arrows: lives becomes 5  // @line write
    lab().snap("**pp follows two arrows (pp to p, p to lives) and writes 5 into lives.", "write", {&lives});  // @viz
    int extra = 9;
    lab().local("extra", "int", extra);  // @viz
    *pp = &extra;  // change WHERE p points, through pp  // @line redirect
    lab().snap("*pp = &extra changes p itself. p now points at extra. Functions use this trick to hand back a pointer.", "redirect", {&p});  // @viz
}
// [/snippet]

// ----------------------------------------------------------------------------
// [snippet:mem.heap]
void heapBasics() {
    auto frame = lab().enter("main");  // @viz
    int* coins = new int(250);  // a box on the HEAP; coins holds its address  // @line new
    lab().pointer("coins", "int*", coins); lab().heapAlloc(coins, sizeof(int), "new int", [coins](MemoryScene& m) { m.cell("", "int", cat(*coins), lab().book.of(coins)); });  // @viz
    lab().snap("new int(250) asks the heap for space and returns its address. The heap box outlives this function until you delete it.", "new", {coins});  // @viz
    *coins += 50;  // @line use
    lab().snap("Use it through the pointer like any other: 300.", "use", {coins});  // @viz
    delete coins;  // give the memory back  // @line delete
    lab().heapFree(coins);  // @viz
    lab().snap("delete frees the heap box. But coins STILL holds the old address. It's now a dangling pointer (red).", "delete", {&coins});  // @viz
    coins = nullptr;  // good habit: now it clearly points at nothing  // @line reset
    lab().snap("Setting it to nullptr makes the danger visible: if (coins) is false, and *coins crashes loudly instead of silently corrupting memory.", "reset");  // @viz
}
// [/snippet]

// ----------------------------------------------------------------------------
// [snippet:mem.leak]
void leakDemo() {
    auto frame = lab().enter("main");  // @viz
    int* bonus = new int(10);  // @line first_new
    lab().pointer("bonus", "int*", bonus); lab().heapAlloc(bonus, sizeof(int), "new int", [p = bonus](MemoryScene& m) { m.cell("", "int", cat(*p), lab().book.of(p)); });  // @viz
    lab().snap("bonus points at a heap int holding 10.", "first_new");  // @viz
    int* lost = bonus;  // @viz
    bonus = new int(20);  // the first box is now unreachable: LEAKED  // @line overwrite
    lab().heapAlloc(bonus, sizeof(int), "new int", [p = bonus](MemoryScene& m) { m.cell("", "int", cat(*p), lab().book.of(p)); });  // @viz
    lab().snap("bonus now points at a NEW box. Nothing points at the 10 any more, so nobody can ever delete it: a memory leak.", "overwrite");  // @viz
    delete bonus;  // frees only the second box  // @line delete_second
    lab().heapFree(bonus);  // @viz
    lab().snap("delete frees the 20. The 10 stays allocated until the program ends. In a game that leaks every frame, memory runs out.", "delete_second");  // @viz
    delete lost;  // @viz
}
// [/snippet]

// ----------------------------------------------------------------------------
// [snippet:mem.unique_ptr]
void uniquePtrDemo() {
    auto frame = lab().enter("main");  // @viz
    const int* raw = nullptr;  // @viz
    {
        auto block = lab().scope();  // @viz
        std::unique_ptr<int> gold = std::make_unique<int>(500);  // gold OWNS the int  // @line make
        raw = gold.get(); lab().smart("gold", "unique_ptr<int>", gold); lab().heapAlloc(raw, sizeof(int), "int (owned)", [raw](MemoryScene& m) { m.cell("", "int", cat(*raw), lab().book.of(raw)); });  // @viz
        lab().snap("std::make_unique puts an int on the heap. gold is its one and only owner.", "make", {&gold});  // @viz
        std::unique_ptr<int> vault = std::move(gold);  // ownership MOVES; gold is now empty  // @line move
        lab().smart("vault", "unique_ptr<int>", vault);  // @viz
        lab().snap("std::move hands ownership to vault. gold becomes empty (nullptr). There is still exactly one owner.", "move", {&vault});  // @viz
        // std::unique_ptr<int> copy = vault;  // won't compile: an owner can't be copied
    }  // vault is destroyed here, and its destructor deletes the int  // @line scope_end
    lab().heapFree(raw);  // @viz
    lab().snap("The block ended. vault's destructor deleted the int automatically. No delete, no leak. This is RAII.", "scope_end");  // @viz
}
// [/snippet]

// ----------------------------------------------------------------------------
namespace {
Lab::Drawer sharedCells(const std::shared_ptr<int>& sp) {
    std::weak_ptr<int> w = sp;  // weak: watching must not change the count
    return [w](MemoryScene& m) {
        const long count = w.use_count();
        if (auto alive = w.lock()) {
            m.cell("value", "int", cat(*alive), lab().book.of(alive.get()));
            m.cell("owners", "use_count", cat(count), "");
        }
    };
}
}  // namespace

// [snippet:mem.shared_ptr]
void sharedPtrDemo() {
    auto frame = lab().enter("main");  // @viz
    auto texture = std::make_shared<int>(64);  // one owner  // @line make
    const int* raw = texture.get(); lab().smart("texture", "shared_ptr<int>", texture); lab().heapAlloc(raw, sizeof(int), "shared int", sharedCells(texture));  // @viz
    lab().snap(cat("make_shared creates the int plus a counter of owners. use_count() = ", texture.use_count(), "."), "make");  // @viz
    {
        auto block = lab().scope();  // @viz
        std::shared_ptr<int> sprite = texture;  // a second owner  // @line copy
        lab().smart("sprite", "shared_ptr<int>", sprite);  // @viz
        lab().snap(cat("Copying a shared_ptr adds an owner. use_count() = ", texture.use_count(), ". Both point at the same int."), "copy", {&sprite});  // @viz
    }  // sprite is destroyed: one owner left  // @line scope_end
    lab().snap(cat("sprite died at the end of the block. use_count() = ", texture.use_count(), ". The int lives on."), "scope_end");  // @viz
    texture.reset();  // the last owner lets go, so the int is deleted  // @line reset
    lab().heapFree(raw);  // @viz
    lab().snap("When the count reaches 0, the int is deleted automatically.", "reset");  // @viz
}
// [/snippet]

// ----------------------------------------------------------------------------
// [snippet:mem.weak_ptr]
struct Hero {
    int id = 0;
    std::shared_ptr<Hero> ally;  // owning link
    std::weak_ptr<Hero> rival;   // NON-owning link
};

void weakPtrDemo() {
    auto frame = lab().enter("main");  // @viz
    const Hero* rawA = nullptr; const Hero* rawB = nullptr;  // @viz
    {
        auto block = lab().scope();  // @viz
        auto a = std::make_shared<Hero>();
        auto b = std::make_shared<Hero>();
        a->id = 1;
        b->id = 2;  // @line make
        rawA = a.get(); rawB = b.get(); lab().smart("a", "shared_ptr<Hero>", a); lab().smart("b", "shared_ptr<Hero>", b);  // @viz
        for (const auto* sp : {&a, &b}) { std::weak_ptr<Hero> w = *sp; lab().heapAlloc(sp->get(), sizeof(Hero), "Hero", [w](MemoryScene& m) { const long n = w.use_count(); auto h = w.lock(); if (!h) return; m.cell("id", "int", cat(h->id), lab().book.of(&h->id)); lab().drawPointerCell(m, "ally", "shared_ptr", lab().book.of(&h->ally), h->ally.get()); auto r = h->rival.lock(); m.cell("rival", "weak_ptr", r ? lab().book.of(r.get()) : "empty", lab().book.of(&h->rival)); if (r) m.arrow(lab().book.of(&h->rival), lab().book.of(r.get()), viz::Mark::Dim, "weak"); m.cell("owners", "use_count", cat(n), ""); }); }  // @viz
        lab().snap("Two heroes on the heap, each with one owner.", "make");  // @viz
        a->ally = b;  // b now has 2 owners  // @line ally
        lab().snap(cat("a->ally = b adds an owner to b: b has ", b.use_count(), " owners."), "ally");  // @viz
        b->rival = a;  // weak: a still has just 1 owner  // @line rival
        lab().snap(cat("b->rival is a weak_ptr. It points at a but does NOT own it: a still has ", a.use_count(), " owner."), "rival");  // @viz
        // If rival were a shared_ptr, a and b would own each other.
        // Their counts could never reach 0, so neither would ever be freed: a leak.
    }  // a's count hits 0, a is freed, which releases b, so b is freed too  // @line scope_end
    lab().heapFree(rawA); lab().heapFree(rawB);  // @viz
    lab().snap("The block ended: a's count dropped to 0, a was destroyed, its ally link released b, and b was destroyed too. weak_ptr broke the cycle.", "scope_end");  // @viz
}
// [/snippet]

// ----------------------------------------------------------------------------

void registerPointerLab(algo::Registry& r) {
    auto add = [&r](const char* id, const char* title, const char* summary, void (*scenario)()) {
        r.add({id, title, "Pointer Lab", id, "", "", summary, {}, Json::object()},
              [scenario](const Json&, viz::Recorder& rec) -> Json {
                  Lab session(rec);
                  scenario();
                  return {{"output", session.lines()}};
              });
    };
    add("mem.address_of", "Address-of & and dereference *", "A pointer is a variable that holds an address.", addressOf);
    add("mem.reference", "References vs pointers", "A reference is another name; a pointer is a separate box.", reference);
    add("mem.swap", "Pass by value vs by pointer", "Why a swap function needs pointers (or references).", passByDemo);
    add("mem.array_pointer", "Arrays and pointer arithmetic", "p + 1 moves one ELEMENT, not one byte.", arrayPointer);
    add("mem.pointer_to_pointer", "Pointer to pointer (T**)", "Two arrows: change a value, or change where a pointer points.", pointerToPointer);
    add("mem.heap", "new, delete and dangling pointers", "The heap, freeing memory, and the pointer that's left behind.", heapBasics);
    add("mem.leak", "Memory leaks", "Lose the last pointer to a heap box and it can never be freed.", leakDemo);
    add("mem.unique_ptr", "unique_ptr and moving ownership", "One owner, automatic delete, no leaks.", uniquePtrDemo);
    add("mem.shared_ptr", "shared_ptr and use_count", "Many owners; the last one out deletes the object.", sharedPtrDemo);
    add("mem.weak_ptr", "weak_ptr breaks cycles", "Non-owning links stop two objects keeping each other alive forever.", weakPtrDemo);
}

}  // namespace forge::memlab
