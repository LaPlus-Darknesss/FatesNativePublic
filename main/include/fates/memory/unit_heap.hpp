#pragma once

#include "fates/memory/ism_heap.hpp"

#include <cstdint>
#include <deque>
#include <map>
#include <mutex>
#include <optional>

namespace ism {

namespace detail {
// Retail names a freed/allocated unit as ExpandableUnitHeapChunk even though
// the chunk lives directly in the page's fixed-unit storage.
struct ExpandableUnitHeapChunk {};
}

class UnitHeap final : public HeapBase {
public:
    UnitHeap() = default;
    ~UnitHeap() override;

    void Initialize(
        void* memory,
        unsigned int heapSize,
        unsigned int unitSize,
        unsigned short alignment,
        int option);

    static unsigned int CalcHeapSize(
        unsigned int unitCount,
        unsigned int unitSize,
        unsigned short alignment,
        bool threadSafe);

    void* Malloc(unsigned int size, unsigned short alignment) override;
    void Free(void* memory) override;
    unsigned int ResizeMemBlock(void* memory, unsigned int size) override;
    unsigned int GetMemBlockSize(const void* memory) const override;
    void* GetStartAddress() const override;
    unsigned int GetTotalSize() const override;
    unsigned int GetTotalFreeSize() const override;
    unsigned int GetTotalUsableSize() const override;

private:
    struct State {
        void* storage{};
        unsigned int heapSize{};
        unsigned int unitStride{};
        unsigned int alignment{};
        unsigned int totalUnits{};
        unsigned int freeUnits{};
        bool threadSafe{};
        std::deque<std::uintptr_t> freeList;
    };

    std::optional<State> state_;
    mutable std::recursive_mutex mutex_;
};

class ExpandableUnitHeap final : public HeapBase {
public:
    struct Impl {
        struct Page {
            void* storage{};
            std::uintptr_t firstUnit{};
            unsigned int totalUnits{};
            unsigned int freeUnits{};
            std::deque<std::uintptr_t> freeList;
        };

        Page* insertPage();
        void deallocate(detail::ExpandableUnitHeapChunk* chunk);
        void* allocate(unsigned int size, unsigned short alignment);

        HeapBase* backing{};
        void* controlAllocation{};
        unsigned int option{};
        unsigned int unitStride{};
        unsigned int alignment{};
        unsigned int pageSize{};
        unsigned int controlSize{};
        unsigned int totalUnits{};
        unsigned int freeUnits{};
        bool threadSafe{};
        mutable std::recursive_mutex mutex;
        std::map<std::uintptr_t, Page> pages;
        std::deque<std::uintptr_t> availablePages;
    };

    ExpandableUnitHeap() = default;
    ~ExpandableUnitHeap() override;

    bool Initialize(
        unsigned int unitSize,
        unsigned short alignment,
        unsigned int pageSize,
        int option,
        HeapBase* backing);

    static unsigned int CalcPageSize(
        unsigned int unitCount,
        unsigned int unitSize,
        unsigned short alignment);

    void* Malloc(unsigned int size, unsigned short alignment) override;
    void Free(void* memory) override;
    unsigned int ResizeMemBlock(void* memory, unsigned int size) override;
    unsigned int GetMemBlockSize(const void* memory) const override;
    void* GetStartAddress() const override;
    unsigned int GetTotalSize() const override;
    unsigned int GetTotalFreeSize() const override;
    unsigned int GetTotalUsableSize() const override;

private:
    std::optional<Impl> impl_;
};

} // namespace ism
