#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <mutex>

namespace ism {

class HeapBase {
public:
    virtual ~HeapBase() = default;
    virtual void* Malloc(unsigned int size, unsigned short alignment) = 0;
    virtual void Free(void* memory) = 0;

    // Retail heap backends return the resulting usable block size, or zero on
    // failure. IAllocator::Resize is the layer that converts this to bool.
    virtual unsigned int ResizeMemBlock(void* memory, unsigned int size) = 0;

    virtual unsigned int GetMemBlockSize(const void* memory) const = 0;
    virtual void* GetStartAddress() const = 0;
    virtual unsigned int GetTotalSize() const = 0;
    virtual unsigned int GetTotalFreeSize() const = 0;
    virtual unsigned int GetTotalUsableSize() const = 0;
};

namespace detail {

enum class HeapOption : unsigned int {};

struct UnifiedHeapChunk {
    std::uintptr_t address{};
    unsigned int size{};
    bool large{};
};

struct NonUnifiedHeapChunk {
    std::uintptr_t address{};
    unsigned int size{};
    bool large{};
};

} // namespace detail

// Retail GlobalHeap is a thin operator-new/operator-delete facade. It tracks
// active allocations but does not expose a bounded address range or block-size
// accounting through the HeapBase query API.
class GlobalHeap final : public HeapBase {
public:
    GlobalHeap() = default;
    ~GlobalHeap() override;

    void* Malloc(unsigned int size, unsigned short alignment) override;
    void Free(void* memory) override;
    unsigned int ResizeMemBlock(void* memory, unsigned int size) override;

    unsigned int GetMemBlockSize(const void* memory) const override;
    void* GetStartAddress() const override;
    unsigned int GetTotalSize() const override;
    unsigned int GetTotalFreeSize() const override;
    unsigned int GetTotalUsableSize() const override;

private:
    unsigned int activeAllocations_{};
};

class UnifiedHeap final : public HeapBase {
public:
    struct Impl {
        void* allocateChunk(detail::UnifiedHeapChunk* chunk, unsigned int size, unsigned int alignment);
        void* allocateLarge(unsigned int size, unsigned int alignment, unsigned int option);
        detail::UnifiedHeapChunk* getAlignedChunk(detail::UnifiedHeapChunk* chunk, unsigned int alignment);
        unsigned int resize(detail::UnifiedHeapChunk* chunk, unsigned int size);
        void* allocate(unsigned int size, unsigned int alignment);
        detail::UnifiedHeapChunk* popLarge(detail::UnifiedHeapChunk* chunk);
        void deallocate(detail::UnifiedHeapChunk* chunk);
        void pushLarge(detail::UnifiedHeapChunk* chunk, unsigned int size);

        std::uintptr_t start{};
        unsigned int totalSize{};
        unsigned int usableSize{};
        bool threadSafe{};
        mutable std::recursive_mutex mutex;
        std::map<std::uintptr_t, detail::UnifiedHeapChunk> blocks;
    };

    UnifiedHeap() = default;
    ~UnifiedHeap() override;

    void Initialize(void* memory, unsigned int size, int option);
    void Finalize();

    void* Malloc(unsigned int size, unsigned short alignment) override;
    void Free(void* memory) override;
    unsigned int ResizeMemBlock(void* memory, unsigned int size) override;

    unsigned int GetMemBlockSize(const void* memory) const override;
    void* GetStartAddress() const override;
    unsigned int GetTotalSize() const override;
    unsigned int GetTotalFreeSize() const override;
    unsigned int GetTotalUsableSize() const override;

private:
    Impl impl_{};
    bool initialized_{};
};

class NonUnifiedHeap final : public HeapBase {
public:
    struct Impl {
        bool initialize(void* memory, unsigned int size, unsigned int unitCount,
                        detail::HeapOption option, HeapBase* backing);
        void* allocateChunk(detail::NonUnifiedHeapChunk* chunk, unsigned int size, unsigned int alignment);
        detail::NonUnifiedHeapChunk* getAlignedChunk(detail::NonUnifiedHeapChunk* chunk, unsigned int alignment);
        unsigned int resize(detail::NonUnifiedHeapChunk* chunk, unsigned int size);
        void* allocate(unsigned int size, unsigned int alignment);
        detail::NonUnifiedHeapChunk* popLarge(detail::NonUnifiedHeapChunk* chunk);
        void deallocate(detail::NonUnifiedHeapChunk* chunk);
        void pushLarge(detail::NonUnifiedHeapChunk* chunk, unsigned int size);

        std::uintptr_t start{};
        unsigned int totalSize{};
        unsigned int usableSize{};
        HeapBase* backing{};
        bool threadSafe{};
        mutable std::recursive_mutex mutex;
        std::map<std::uintptr_t, detail::NonUnifiedHeapChunk> blocks;
    };

    NonUnifiedHeap() = default;
    ~NonUnifiedHeap() override;

    bool Initialize(void* memory, unsigned int size, int option, HeapBase* backing);
    void Finalize();

    void* Malloc(unsigned int size, unsigned short alignment) override;
    void Free(void* memory) override;
    unsigned int ResizeMemBlock(void* memory, unsigned int size) override;

    unsigned int GetMemBlockSize(const void* memory) const override;
    void* GetStartAddress() const override;
    unsigned int GetTotalSize() const override;
    unsigned int GetTotalFreeSize() const override;
    unsigned int GetTotalUsableSize() const override;

private:
    Impl impl_{};
    bool initialized_{};
};

} // namespace ism
