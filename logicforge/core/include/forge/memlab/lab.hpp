#pragma once
// The Pointer & Lambda Lab runtime.
//
// Scenarios are REAL C++ that really runs. The pictures are built from
// the program's actual state: values are read from the variables, arrows
// come from actual pointer values. Only the addresses are renamed:
// 0x7ffd5e3a9c4c becomes 0x7fec. Offsets inside an object are kept,
// so pointer arithmetic still adds up (p + 1 is 4 bytes after p for an int).
//
// Undefined behaviour (using a dangling pointer, double delete...) is
// never executed. Those scenarios show the memory at the moment just
// before the bug, and the code comments say what would go wrong.
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

#include "forge/util/cat.hpp"
#include "forge/viz/recorder.hpp"
#include "forge/viz/scene.hpp"

namespace forge::memlab {

// Real address -> short readable address, keeping offsets within an object.
class AddressBook {
public:
    std::string stack(const void* p, std::size_t size);
    std::string heap(const void* p, std::size_t size);
    std::string of(const void* p) const;  // "nullptr" for null, "?" if unknown
    void markFreed(const void* p);
    bool freed(const void* p) const;
    void forget(const void* p);

private:
    struct Region {
        std::uintptr_t start;
        std::size_t size;
        std::uintptr_t friendly;
        bool freed;
    };
    const Region* find(const void* p) const;
    std::string add(const void* p, std::size_t size, std::uintptr_t& cursor, bool down);
    std::vector<Region> regions_;
    std::uintptr_t stackCursor_ = 0x7ff0;
    std::uintptr_t heapCursor_ = 0x5000;
};

template <typename T>
std::string display(const T& v) {
    if constexpr (std::is_same_v<T, bool>) {
        return v ? "true" : "false";
    } else if constexpr (std::is_same_v<T, char>) {
        return std::string("'") + v + "'";
    } else if constexpr (std::is_same_v<T, std::string>) {
        return "\"" + v + "\"";
    } else {
        return cat(v);
    }
}

class Lab {
public:
    using Drawer = std::function<void(viz::MemoryScene&)>;

    explicit Lab(viz::Recorder& rec);
    ~Lab();
    Lab(const Lab&) = delete;
    Lab& operator=(const Lab&) = delete;

    // The lab running on this thread (set by the constructor).
    static Lab& current();

    // RAII: entering a function pushes a stack frame; the guard pops it
    // (and forgets its variables) when the function returns.
    class FrameGuard {
    public:
        explicit FrameGuard(Lab& lab) : lab_(&lab) {}
        FrameGuard(FrameGuard&& other) noexcept : lab_(other.lab_) { other.lab_ = nullptr; }
        FrameGuard(const FrameGuard&) = delete;
        FrameGuard& operator=(const FrameGuard&) = delete;
        FrameGuard& operator=(FrameGuard&&) = delete;
        ~FrameGuard() {
            if (lab_ != nullptr) lab_->leave();
        }

    private:
        Lab* lab_;
    };
    [[nodiscard]] FrameGuard enter(std::string function);

    // Show a plain variable in the current frame. Its value is read every snapshot.
    template <typename T>
    void local(std::string name, std::string type, const T& var) {
        const std::string addr = book.stack(&var, sizeof(T));
        track(&var);
        draw([name = std::move(name), type = std::move(type), &var, addr](viz::MemoryScene& m) {
            m.local(name, type, display(var), addr);
        });
    }

    // Show a raw pointer variable; draws an arrow to whatever it points at right now.
    template <typename T>
    void pointer(std::string name, std::string type, T* const& ptr) {
        const std::string addr = book.stack(&ptr, sizeof(T*));
        track(&ptr);
        draw([this, name = std::move(name), type = std::move(type), &ptr, addr](viz::MemoryScene& m) {
            drawPointer(m, name, type, addr, ptr);
        });
    }

    // A C array: one box per element with real offsets (p + 1 really is sizeof(T) bytes further).
    template <typename T, std::size_t N>
    void array(std::string name, std::string type, const T (&arr)[N]) {
        book.stack(&arr, sizeof arr);
        track(&arr);
        draw([this, name = std::move(name), type = std::move(type), &arr](viz::MemoryScene& m) {
            for (std::size_t i = 0; i < N; ++i) {
                m.local(cat(name, "[", i, "]"), type, display(arr[i]), book.of(&arr[i]));
            }
        });
    }

    // unique_ptr / shared_ptr: drawn like a pointer to whatever .get() returns now.
    template <typename Smart>
    void smart(std::string name, std::string type, const Smart& sp) {
        const std::string addr = book.stack(&sp, sizeof(Smart));
        track(&sp);
        draw([this, name = std::move(name), type = std::move(type), &sp, addr](viz::MemoryScene& m) {
            drawPointer(m, name, type, addr, sp.get());
        });
    }

    // RAII guard for a { block }: variables declared inside disappear when it closes.
    class ScopeGuard {
    public:
        ScopeGuard(Lab& lab, std::size_t drawers, std::size_t owned)
            : lab_(&lab), drawers_(drawers), owned_(owned) {}
        ScopeGuard(ScopeGuard&& o) noexcept : lab_(o.lab_), drawers_(o.drawers_), owned_(o.owned_) {
            o.lab_ = nullptr;
        }
        ScopeGuard(const ScopeGuard&) = delete;
        ScopeGuard& operator=(const ScopeGuard&) = delete;
        ScopeGuard& operator=(ScopeGuard&&) = delete;
        ~ScopeGuard() {
            if (lab_ != nullptr) lab_->closeScope(drawers_, owned_);
        }

    private:
        Lab* lab_;
        std::size_t drawers_;
        std::size_t owned_;
    };
    [[nodiscard]] ScopeGuard scope();

    // A reference has no box of its own: it is another name for an existing object.
    void alias(const std::string& name, const void* target) { aliases_.emplace_back(name, target); }

    // Any custom drawing in the current frame (closures, smart pointers...).
    void draw(Drawer d);
    void drawPointer(viz::MemoryScene& m, const std::string& name, const std::string& type,
                     const std::string& selfAddr, const void* target) const;
    // Same, for a pointer that lives inside a heap block (e.g. a struct member).
    void drawPointerCell(viz::MemoryScene& m, const std::string& name, const std::string& type,
                         const std::string& selfAddr, const void* target) const;

    // Heap blocks. A block's drawer runs until heapFree(); after that only a "freed" box is shown.
    void heapAlloc(const void* p, std::size_t size, std::string label, Drawer cells);
    void heapFree(const void* p);

    // Record one frame. `highlight` marks cells at these real addresses.
    void snap(std::string caption, std::string line, std::vector<const void*> highlight = {});

    std::vector<std::string> lines() const;

    AddressBook book;
    std::ostringstream out;  // the scenario's console, shown under the picture

private:
    struct StackFrame {
        std::string function;
        std::vector<Drawer> drawers;
        std::vector<const void*> owned;
    };
    struct HeapBlock {
        const void* key;
        std::string addr;
        std::string label;
        Drawer cells;
        bool freed = false;
    };
    void leave();
    void closeScope(std::size_t drawers, std::size_t owned);
    void track(const void* p);

    viz::Recorder& rec_;
    Lab* previous_;
    std::vector<StackFrame> frames_;
    std::vector<HeapBlock> heap_;
    std::vector<std::pair<std::string, const void*>> aliases_;
};

inline Lab& lab() { return Lab::current(); }

}  // namespace forge::memlab
