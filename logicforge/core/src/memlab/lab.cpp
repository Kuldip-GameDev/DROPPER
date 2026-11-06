#include "forge/memlab/lab.hpp"

#include <algorithm>
#include <cstdio>
#include <stdexcept>

namespace forge::memlab {

namespace {
thread_local Lab* tlCurrent = nullptr;

std::string hex(std::uintptr_t v) {
    char buf[32];
    std::snprintf(buf, sizeof buf, "0x%04llx", static_cast<unsigned long long>(v));
    return buf;
}
}  // namespace

// ---------------------------------------------------------------- AddressBook

const AddressBook::Region* AddressBook::find(const void* p) const {
    const auto a = reinterpret_cast<std::uintptr_t>(p);
    // Newest first, so a reused address finds its latest owner.
    for (auto it = regions_.rbegin(); it != regions_.rend(); ++it) {
        if (a >= it->start && a < it->start + std::max<std::size_t>(it->size, 1)) return &*it;
    }
    // One-past-the-end of an object is a valid pointer value (e.g. end of an array).
    for (auto it = regions_.rbegin(); it != regions_.rend(); ++it) {
        if (a == it->start + it->size) return &*it;
    }
    return nullptr;
}

std::string AddressBook::add(const void* p, std::size_t size, std::uintptr_t& cursor, bool down) {
    const auto a = reinterpret_cast<std::uintptr_t>(p);
    for (auto& r : regions_) {
        if (r.start == a && !r.freed && r.size == size) return hex(r.friendly);
    }
    // Same start but freed (the allocator reused the memory): take over its friendly address.
    for (auto& r : regions_) {
        if (r.start == a && r.freed) {
            r.freed = false;
            r.size = size;
            return hex(r.friendly);
        }
    }
    const std::size_t aligned = (std::max<std::size_t>(size, 1) + 3) / 4 * 4;
    std::uintptr_t friendly;
    if (down) {
        cursor -= aligned;
        friendly = cursor;
    } else {
        friendly = cursor;
        cursor += aligned + 8;  // a little gap, like real allocator headers
    }
    regions_.push_back(Region{a, size, friendly, false});
    return hex(friendly);
}

std::string AddressBook::stack(const void* p, std::size_t size) { return add(p, size, stackCursor_, true); }

std::string AddressBook::heap(const void* p, std::size_t size) { return add(p, size, heapCursor_, false); }

std::string AddressBook::of(const void* p) const {
    if (p == nullptr) return "nullptr";
    const Region* r = find(p);
    if (r == nullptr) return "?";
    return hex(r->friendly + (reinterpret_cast<std::uintptr_t>(p) - r->start));
}

void AddressBook::markFreed(const void* p) {
    const auto a = reinterpret_cast<std::uintptr_t>(p);
    for (auto it = regions_.rbegin(); it != regions_.rend(); ++it) {
        if (it->start == a) {
            it->freed = true;
            return;
        }
    }
}

bool AddressBook::freed(const void* p) const {
    const Region* r = find(p);
    return r != nullptr && r->freed;
}

void AddressBook::forget(const void* p) {
    const auto a = reinterpret_cast<std::uintptr_t>(p);
    regions_.erase(std::remove_if(regions_.begin(), regions_.end(),
                                  [a](const Region& r) { return r.start == a && !r.freed; }),
                   regions_.end());
}

// ------------------------------------------------------------------------ Lab

Lab::Lab(viz::Recorder& rec) : rec_(rec), previous_(tlCurrent) { tlCurrent = this; }

Lab::~Lab() { tlCurrent = previous_; }

Lab& Lab::current() {
    if (tlCurrent == nullptr) throw std::logic_error("no memlab::Lab is running on this thread");
    return *tlCurrent;
}

Lab::FrameGuard Lab::enter(std::string function) {
    frames_.push_back(StackFrame{std::move(function), {}, {}});
    return FrameGuard(*this);
}

// When a function returns or a block closes, its variables die. Their
// addresses are marked dead (not forgotten), so a pointer that still
// holds one is drawn as dangling.
void Lab::leave() {
    if (frames_.empty()) return;
    closeScope(0, 0);
    frames_.pop_back();
}

Lab::ScopeGuard Lab::scope() {
    if (frames_.empty()) throw std::logic_error("Lab: scope() before enter()");
    return ScopeGuard(*this, frames_.back().drawers.size(), frames_.back().owned.size());
}

void Lab::closeScope(std::size_t drawers, std::size_t owned) {
    if (frames_.empty()) return;
    auto& f = frames_.back();
    for (std::size_t i = owned; i < f.owned.size(); ++i) {
        const void* p = f.owned[i];
        book.markFreed(p);
        aliases_.erase(std::remove_if(aliases_.begin(), aliases_.end(),
                                      [p](const auto& a) { return a.second == p; }),
                       aliases_.end());
    }
    if (f.owned.size() > owned) f.owned.resize(owned);
    if (f.drawers.size() > drawers) f.drawers.resize(drawers);
}

void Lab::track(const void* p) {
    if (frames_.empty()) throw std::logic_error("Lab: declare variables after enter()");
    frames_.back().owned.push_back(p);
}

void Lab::draw(Drawer d) {
    if (frames_.empty()) throw std::logic_error("Lab: draw() before enter()");
    frames_.back().drawers.push_back(std::move(d));
}

void Lab::drawPointer(viz::MemoryScene& m, const std::string& name, const std::string& type,
                      const std::string& selfAddr, const void* target) const {
    const bool dangling = target != nullptr && book.freed(target);
    m.local(name, type, book.of(target), selfAddr, dangling ? viz::Mark::Error : viz::Mark::None);
    if (target != nullptr) {
        m.arrow(selfAddr, book.of(target), dangling ? viz::Mark::Error : viz::Mark::None,
                dangling ? "dangling" : "");
    }
}

void Lab::drawPointerCell(viz::MemoryScene& m, const std::string& name, const std::string& type,
                          const std::string& selfAddr, const void* target) const {
    const bool dangling = target != nullptr && book.freed(target);
    m.cell(name, type, book.of(target), selfAddr, dangling ? viz::Mark::Error : viz::Mark::None);
    if (target != nullptr) {
        m.arrow(selfAddr, book.of(target), dangling ? viz::Mark::Error : viz::Mark::None,
                dangling ? "dangling" : "");
    }
}

void Lab::heapAlloc(const void* p, std::size_t size, std::string label, Drawer cells) {
    const std::string addr = book.heap(p, size);
    heap_.push_back(HeapBlock{p, addr, std::move(label), std::move(cells), false});
}

void Lab::heapFree(const void* p) {
    book.markFreed(p);
    for (auto& b : heap_) {
        if (b.key == p && !b.freed) {
            b.freed = true;
            b.cells = nullptr;  // never read freed memory
        }
    }
}

std::vector<std::string> Lab::lines() const {
    std::vector<std::string> result;
    std::istringstream in(out.str());
    for (std::string line; std::getline(in, line);) result.push_back(line);
    return result;
}

void Lab::snap(std::string caption, std::string line, std::vector<const void*> highlight) {
    viz::MemoryScene m;
    for (const auto& f : frames_) {
        m.frame(f.function);
        for (const auto& d : f.drawers) d(m);
    }
    for (const auto& b : heap_) {
        m.block(b.addr, b.freed ? b.label + " (freed)" : b.label, b.freed ? viz::Mark::Dim : viz::Mark::None, b.freed);
        if (b.cells) b.cells(m);
    }
    m.output(lines());
    Json j = m.toJson();

    // Post-process: aliases (references) and highlights.
    std::vector<std::string> hot;
    for (const void* p : highlight) hot.push_back(book.of(p));
    auto touch = [&](Json& c) {
        const auto addr = c["addr"].get<std::string>();
        for (const auto& [name, target] : aliases_) {
            if (book.of(target) == addr) c["name"] = c["name"].get<std::string>() + " / " + name;
        }
        if (std::find(hot.begin(), hot.end(), addr) != hot.end()) c["mark"] = "active";
    };
    for (auto& f : j["stack"]) for (auto& c : f["locals"]) touch(c);
    for (auto& b : j["heap"]) for (auto& c : b["cells"]) touch(c);

    rec_.frame(std::move(j), std::move(caption), std::move(line));
}

}  // namespace forge::memlab
