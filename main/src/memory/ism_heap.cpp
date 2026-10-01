#include "fates/memory/ism_heap.hpp"

#include <algorithm>
#include <new>
#include <cstdint>

namespace {

std::uintptr_t AlignUp(std::uintptr_t value, unsigned int alignment) {
    const std::uintptr_t a = alignment == 0 ? 1u : alignment;
    return (value + a - 1u) & ~(a - 1u);
}

template <class Map>
unsigned int SumUsed(const Map& blocks) {
    unsigned int used = 0;
    for (const auto& [_, block] : blocks) {
        used += block.size;
    }
    return used;
}

} // namespace

namespace ism {

GlobalHeap::~GlobalHeap() = default;

void* GlobalHeap::Malloc(unsigned int size, unsigned short) {
    void* memory = ::operator new(size, std::nothrow);
    if (memory) ++activeAllocations_;
    return memory;
}

void GlobalHeap::Free(void* memory) {
    if (!memory) return;
    ::operator delete(memory);
    if (activeAllocations_ != 0) --activeAllocations_;
}

unsigned int GlobalHeap::ResizeMemBlock(void*, unsigned int) { return 0; }
unsigned int GlobalHeap::GetMemBlockSize(const void*) const { return 0; }
void* GlobalHeap::GetStartAddress() const { return nullptr; }
unsigned int GlobalHeap::GetTotalSize() const { return 0; }
unsigned int GlobalHeap::GetTotalFreeSize() const { return 0; }
unsigned int GlobalHeap::GetTotalUsableSize() const { return 0; }


UnifiedHeap::~UnifiedHeap() {
    Finalize();
}

void UnifiedHeap::Initialize(void* memory, unsigned int size, int option) {
    if (initialized_) return;
    impl_.start = reinterpret_cast<std::uintptr_t>(memory);
    impl_.totalSize = size;
    // Retail reserves 0x1B8 bytes without a critical section and 0x1C0 with one.
    const unsigned int header = (option & 1) ? 0x1C0u : 0x1B8u;
    impl_.usableSize = size > header ? size - header : 0;
    impl_.threadSafe = (option & 1) != 0;
    impl_.blocks.clear();
    initialized_ = true;
}

void UnifiedHeap::Finalize() {
    if (!initialized_) return;
    std::lock_guard<std::recursive_mutex> lock(impl_.mutex);
    impl_.blocks.clear();
    impl_.start = 0;
    impl_.totalSize = 0;
    impl_.usableSize = 0;
    initialized_ = false;
}

void* UnifiedHeap::Malloc(unsigned int size, unsigned short alignment) {
    return impl_.allocate(size, alignment);
}

void UnifiedHeap::Free(void* memory) {
    if (!memory) return;
    std::lock_guard<std::recursive_mutex> lock(impl_.mutex);
    const auto it = impl_.blocks.find(reinterpret_cast<std::uintptr_t>(memory));
    if (it != impl_.blocks.end()) {
        detail::UnifiedHeapChunk copy = it->second;
        impl_.deallocate(&copy);
    }
}

unsigned int UnifiedHeap::ResizeMemBlock(void* memory, unsigned int size) {
    if (!memory) return 0;
    std::lock_guard<std::recursive_mutex> lock(impl_.mutex);
    auto it = impl_.blocks.find(reinterpret_cast<std::uintptr_t>(memory));
    if (it == impl_.blocks.end()) return 0;
    return impl_.resize(&it->second, size);
}

unsigned int UnifiedHeap::GetMemBlockSize(const void* memory) const {
    if (!memory) return 0;
    std::lock_guard<std::recursive_mutex> lock(impl_.mutex);
    const auto it = impl_.blocks.find(reinterpret_cast<std::uintptr_t>(memory));
    return it == impl_.blocks.end() ? 0u : it->second.size;
}

void* UnifiedHeap::GetStartAddress() const {
    return reinterpret_cast<void*>(impl_.start);
}

unsigned int UnifiedHeap::GetTotalSize() const { return impl_.totalSize; }
unsigned int UnifiedHeap::GetTotalUsableSize() const { return impl_.usableSize; }

unsigned int UnifiedHeap::GetTotalFreeSize() const {
    std::lock_guard<std::recursive_mutex> lock(impl_.mutex);
    const unsigned int used = SumUsed(impl_.blocks);
    return used < impl_.usableSize ? impl_.usableSize - used : 0;
}

void* UnifiedHeap::Impl::allocateChunk(detail::UnifiedHeapChunk* chunk, unsigned int size,
                                       unsigned int alignment) {
    if (!chunk) return nullptr;
    chunk->address = AlignUp(chunk->address, alignment);
    chunk->size = size;
    blocks[chunk->address] = *chunk;
    return reinterpret_cast<void*>(chunk->address);
}

void* UnifiedHeap::Impl::allocateLarge(unsigned int size, unsigned int alignment, unsigned int) {
    detail::UnifiedHeapChunk candidate{};
    candidate.address = start;
    if (!blocks.empty()) {
        const auto& last = blocks.rbegin()->second;
        candidate.address = last.address + last.size;
    }
    candidate.address = AlignUp(candidate.address, alignment);
    candidate.size = size;
    candidate.large = true;
    if (candidate.address + size > start + usableSize) return nullptr;
    blocks[candidate.address] = candidate;
    return reinterpret_cast<void*>(candidate.address);
}

detail::UnifiedHeapChunk* UnifiedHeap::Impl::getAlignedChunk(detail::UnifiedHeapChunk* chunk,
                                                             unsigned int alignment) {
    if (!chunk) return nullptr;
    chunk->address = AlignUp(chunk->address, alignment);
    return chunk;
}

unsigned int UnifiedHeap::Impl::resize(detail::UnifiedHeapChunk* chunk, unsigned int size) {
    if (!chunk) return 0;
    // Retail stores an 8-byte chunk header before the user region. The public
    // resize result is the normalized usable size, not a boolean.
    const unsigned int normalized =
        (size + 8u < 0x11u) ? 8u : (((size + 15u) & ~7u) - 8u);
    const unsigned int old = chunk->size;
    const unsigned int used = SumUsed(blocks);
    if (normalized > old && used + (normalized - old) > usableSize) return 0;
    chunk->size = normalized;
    blocks[chunk->address] = *chunk;
    return normalized;
}

void* UnifiedHeap::Impl::allocate(unsigned int size, unsigned int alignment) {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if (size == 0 || start == 0) return nullptr;
    detail::UnifiedHeapChunk candidate{};
    candidate.address = start;
    for (const auto& [_, block] : blocks) {
        const auto aligned = AlignUp(candidate.address, alignment);
        if (aligned + size <= block.address) {
            candidate.address = aligned;
            candidate.size = size;
            blocks[candidate.address] = candidate;
            return reinterpret_cast<void*>(candidate.address);
        }
        candidate.address = std::max(candidate.address, block.address + block.size);
    }
    candidate.address = AlignUp(candidate.address, alignment);
    if (candidate.address + size > start + usableSize) return nullptr;
    candidate.size = size;
    blocks[candidate.address] = candidate;
    return reinterpret_cast<void*>(candidate.address);
}

detail::UnifiedHeapChunk* UnifiedHeap::Impl::popLarge(detail::UnifiedHeapChunk* chunk) {
    return chunk;
}

void UnifiedHeap::Impl::deallocate(detail::UnifiedHeapChunk* chunk) {
    if (!chunk) return;
    blocks.erase(chunk->address);
}

void UnifiedHeap::Impl::pushLarge(detail::UnifiedHeapChunk* chunk, unsigned int size) {
    if (!chunk) return;
    chunk->size = size;
    chunk->large = true;
    blocks[chunk->address] = *chunk;
}

NonUnifiedHeap::~NonUnifiedHeap() {
    Finalize();
}

bool NonUnifiedHeap::Initialize(void* memory, unsigned int size, int option, HeapBase* backing) {
    if (initialized_) return true;
    const auto flags = static_cast<detail::HeapOption>(
        static_cast<unsigned int>(option) | 0x40u | (backing ? 0u : 0x20u));
    initialized_ = impl_.initialize(memory, size, 0, flags, backing);
    return initialized_;
}

void NonUnifiedHeap::Finalize() {
    if (!initialized_) return;
    std::lock_guard<std::recursive_mutex> lock(impl_.mutex);
    impl_.blocks.clear();
    impl_.start = 0;
    impl_.totalSize = 0;
    impl_.usableSize = 0;
    initialized_ = false;
}

void* NonUnifiedHeap::Malloc(unsigned int size, unsigned short alignment) {
    return impl_.allocate(size, alignment);
}

void NonUnifiedHeap::Free(void* memory) {
    if (!memory) return;
    std::lock_guard<std::recursive_mutex> lock(impl_.mutex);
    const auto it = impl_.blocks.find(reinterpret_cast<std::uintptr_t>(memory));
    if (it != impl_.blocks.end()) {
        detail::NonUnifiedHeapChunk copy = it->second;
        impl_.deallocate(&copy);
    }
}

unsigned int NonUnifiedHeap::ResizeMemBlock(void* memory, unsigned int size) {
    if (!memory) return 0;
    std::lock_guard<std::recursive_mutex> lock(impl_.mutex);
    auto it = impl_.blocks.find(reinterpret_cast<std::uintptr_t>(memory));
    return it == impl_.blocks.end() ? 0u : impl_.resize(&it->second, size);
}

unsigned int NonUnifiedHeap::GetMemBlockSize(const void* memory) const {
    if (!memory) return 0;
    std::lock_guard<std::recursive_mutex> lock(impl_.mutex);
    const auto it = impl_.blocks.find(reinterpret_cast<std::uintptr_t>(memory));
    return it == impl_.blocks.end() ? 0u : it->second.size;
}

void* NonUnifiedHeap::GetStartAddress() const { return reinterpret_cast<void*>(impl_.start); }
unsigned int NonUnifiedHeap::GetTotalSize() const { return impl_.totalSize; }
unsigned int NonUnifiedHeap::GetTotalUsableSize() const { return impl_.usableSize; }

unsigned int NonUnifiedHeap::GetTotalFreeSize() const {
    std::lock_guard<std::recursive_mutex> lock(impl_.mutex);
    const unsigned int used = SumUsed(impl_.blocks);
    return used < impl_.usableSize ? impl_.usableSize - used : 0;
}

bool NonUnifiedHeap::Impl::initialize(void* memory, unsigned int size, unsigned int,
                                      detail::HeapOption option, HeapBase* backingHeap) {
    if (start != 0) return false;
    start = reinterpret_cast<std::uintptr_t>(memory);
    totalSize = size;
    usableSize = size;
    backing = backingHeap;
    threadSafe = (static_cast<unsigned int>(option) & 1u) != 0;
    blocks.clear();
    return start != 0 && size != 0;
}

void* NonUnifiedHeap::Impl::allocateChunk(detail::NonUnifiedHeapChunk* chunk, unsigned int size,
                                          unsigned int alignment) {
    if (!chunk) return nullptr;
    chunk->address = AlignUp(chunk->address, alignment);
    chunk->size = size;
    blocks[chunk->address] = *chunk;
    return reinterpret_cast<void*>(chunk->address);
}

detail::NonUnifiedHeapChunk* NonUnifiedHeap::Impl::getAlignedChunk(
    detail::NonUnifiedHeapChunk* chunk, unsigned int alignment) {
    if (!chunk) return nullptr;
    chunk->address = AlignUp(chunk->address, alignment);
    return chunk;
}

unsigned int NonUnifiedHeap::Impl::resize(detail::NonUnifiedHeapChunk* chunk, unsigned int size) {
    if (!chunk) return 0;
    const unsigned int normalized = (size + 7u) & ~7u;
    const unsigned int old = chunk->size;
    const unsigned int used = SumUsed(blocks);
    if (normalized > old && used + (normalized - old) > usableSize) return 0;
    chunk->size = normalized;
    blocks[chunk->address] = *chunk;
    return normalized;
}

void* NonUnifiedHeap::Impl::allocate(unsigned int size, unsigned int alignment) {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if (size == 0 || start == 0) return nullptr;
    detail::NonUnifiedHeapChunk candidate{};
    candidate.address = start;
    for (const auto& [_, block] : blocks) {
        const auto aligned = AlignUp(candidate.address, alignment);
        if (aligned + size <= block.address) {
            candidate.address = aligned;
            candidate.size = size;
            blocks[candidate.address] = candidate;
            return reinterpret_cast<void*>(candidate.address);
        }
        candidate.address = std::max(candidate.address, block.address + block.size);
    }
    candidate.address = AlignUp(candidate.address, alignment);
    if (candidate.address + size > start + usableSize) return nullptr;
    candidate.size = size;
    blocks[candidate.address] = candidate;
    return reinterpret_cast<void*>(candidate.address);
}

detail::NonUnifiedHeapChunk* NonUnifiedHeap::Impl::popLarge(detail::NonUnifiedHeapChunk* chunk) {
    return chunk;
}

void NonUnifiedHeap::Impl::deallocate(detail::NonUnifiedHeapChunk* chunk) {
    if (!chunk) return;
    blocks.erase(chunk->address);
}

void NonUnifiedHeap::Impl::pushLarge(detail::NonUnifiedHeapChunk* chunk, unsigned int size) {
    if (!chunk) return;
    chunk->size = size;
    chunk->large = true;
    blocks[chunk->address] = *chunk;
}

} // namespace ism
