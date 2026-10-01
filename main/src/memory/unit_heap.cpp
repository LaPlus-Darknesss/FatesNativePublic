#include "fates/memory/unit_heap.hpp"

#include "fates/detail/unit_heap_runtime.hpp"

#include <algorithm>
#include <cstdint>

namespace {

unsigned int RoundUpToFour(unsigned int value) {
    return (value + 3u) & ~3u;
}

unsigned int RoundUpMultiple(unsigned int value, unsigned int multiple) {
    return ((value + multiple - 1u) / multiple) * multiple;
}

std::uintptr_t RoundUpAddress(std::uintptr_t value, unsigned int multiple) {
    return ((value + multiple - 1u) / multiple) * multiple;
}

template <class Mutex>
std::unique_lock<Mutex> ConditionalLock(Mutex& mutex, bool enabled) {
    std::unique_lock<Mutex> lock(mutex, std::defer_lock);
    if (enabled) lock.lock();
    return lock;
}

} // namespace

namespace ism {

unsigned int ExpandableUnitHeap::CalcPageSize(
    unsigned int unitCount,
    unsigned int unitSize,
    unsigned short alignment) {

    const unsigned int normalizedAlignment = RoundUpToFour(alignment);
    const unsigned int unitStride = RoundUpMultiple(unitSize, normalizedAlignment);
    const unsigned int pageHeader = RoundUpMultiple(0x28u, normalizedAlignment);
    return unitCount * unitStride + pageHeader;
}

bool ExpandableUnitHeap::Initialize(
    unsigned int unitSize,
    unsigned short alignment,
    unsigned int pageSize,
    int option,
    HeapBase* backing) {

    if (impl_.has_value()) return false;

    unsigned int flags = static_cast<unsigned int>(option);
    if (!backing) {
        flags |= 0x20u;
        backing = fates::decomp_detail::GetDefaultIsmHeap();
    }
    if (!backing) return false;

    const bool threadSafe = (flags & 1u) != 0;
    const unsigned int controlSize = threadSafe ? 0x48u : 0x3Cu;
    void* control = backing->Malloc(controlSize, 8);
    if (!control) return false;

    impl_.emplace();
    Impl& impl = *impl_;
    impl.backing = backing;
    impl.controlAllocation = control;
    impl.option = flags;
    impl.unitStride = RoundUpMultiple(unitSize, RoundUpToFour(alignment));
    impl.alignment = RoundUpToFour(alignment);
    impl.pageSize = RoundUpToFour(pageSize);
    impl.controlSize = controlSize;
    impl.threadSafe = threadSafe;

    if (!impl.insertPage()) {
        backing->Free(control);
        impl_.reset();
        return false;
    }
    return true;
}

ExpandableUnitHeap::Impl::Page* ExpandableUnitHeap::Impl::insertPage() {
    if (!backing || pageSize == 0 || unitStride == 0 || alignment == 0) return nullptr;

    void* storage = backing->Malloc(pageSize, 8);
    if (!storage) return nullptr;

    const std::uintptr_t pageStart = reinterpret_cast<std::uintptr_t>(storage);
    const std::uintptr_t firstUnit = RoundUpAddress(pageStart + 0x28u, alignment);
    const std::uintptr_t pageEnd = pageStart + pageSize;
    if (firstUnit >= pageEnd) {
        backing->Free(storage);
        return nullptr;
    }

    const unsigned int units = static_cast<unsigned int>((pageEnd - firstUnit) / unitStride);
    if (units == 0) {
        backing->Free(storage);
        return nullptr;
    }

    Page page{};
    page.storage = storage;
    page.firstUnit = firstUnit;
    page.totalUnits = units;
    page.freeUnits = units;
    for (unsigned int i = 0; i < units; ++i) {
        page.freeList.push_back(firstUnit + static_cast<std::uintptr_t>(i) * unitStride);
    }

    auto [it, inserted] = pages.emplace(pageStart, std::move(page));
    if (!inserted) {
        backing->Free(storage);
        return nullptr;
    }

    availablePages.push_back(pageStart);
    totalUnits += units;
    freeUnits += units;
    return &it->second;
}

void* ExpandableUnitHeap::Impl::allocate(unsigned int size, unsigned short) {
    if (size > unitStride) return nullptr;
    if (freeUnits == 0 && !insertPage()) return nullptr;

    while (!availablePages.empty()) {
        const std::uintptr_t key = availablePages.front();
        auto it = pages.find(key);
        if (it == pages.end() || it->second.freeUnits == 0 || it->second.freeList.empty()) {
            availablePages.pop_front();
            continue;
        }

        Page& page = it->second;
        const std::uintptr_t address = page.freeList.front();
        page.freeList.pop_front();
        --page.freeUnits;
        --freeUnits;
        if (page.freeUnits == 0) availablePages.pop_front();
        return reinterpret_cast<void*>(address);
    }
    return nullptr;
}

void ExpandableUnitHeap::Impl::deallocate(detail::ExpandableUnitHeapChunk* chunk) {
    if (!chunk || pages.empty()) return;
    const std::uintptr_t address = reinterpret_cast<std::uintptr_t>(chunk);

    auto it = pages.upper_bound(address);
    if (it == pages.begin()) return;
    --it;
    Page& page = it->second;
    const std::uintptr_t end = page.firstUnit +
        static_cast<std::uintptr_t>(page.totalUnits) * unitStride;
    if (address < page.firstUnit || address >= end ||
        ((address - page.firstUnit) % unitStride) != 0) {
        return;
    }

    const bool wasFull = page.freeUnits == 0;
    page.freeList.push_front(address);
    ++page.freeUnits;
    ++freeUnits;
    if (wasFull) availablePages.push_back(it->first);

    // Retail keeps spare completely-free pages only while global free capacity
    // is within roughly 4/3 of the average page capacity.  Above that pressure
    // threshold it releases whole pages back to the backing HeapBase.
    while (pages.size() > 1) {
        const unsigned int averageUnits = totalUnits / static_cast<unsigned int>(pages.size());
        const unsigned int freeThreshold = (averageUnits * 4u) / 3u;
        if (freeUnits <= freeThreshold) break;

        auto empty = std::find_if(pages.begin(), pages.end(), [](const auto& kv) {
            return kv.second.freeUnits == kv.second.totalUnits;
        });
        if (empty == pages.end()) break;

        const std::uintptr_t key = empty->first;
        availablePages.erase(
            std::remove(availablePages.begin(), availablePages.end(), key),
            availablePages.end());
        totalUnits -= empty->second.totalUnits;
        freeUnits -= empty->second.totalUnits;
        void* storage = empty->second.storage;
        pages.erase(empty);
        backing->Free(storage);
    }
}

void* ExpandableUnitHeap::Malloc(unsigned int size, unsigned short alignment) {
    if (!impl_) return nullptr;
    auto lock = ConditionalLock(impl_->mutex, impl_->threadSafe);
    return impl_->allocate(size, alignment);
}

void ExpandableUnitHeap::Free(void* memory) {
    if (!impl_ || !memory) return;
    auto lock = ConditionalLock(impl_->mutex, impl_->threadSafe);
    impl_->deallocate(reinterpret_cast<detail::ExpandableUnitHeapChunk*>(memory));
}

unsigned int ExpandableUnitHeap::ResizeMemBlock(void*, unsigned int size) {
    if (!impl_ || size > impl_->unitStride) return 0;
    return impl_->unitStride;
}

unsigned int ExpandableUnitHeap::GetMemBlockSize(const void*) const {
    return impl_ ? impl_->unitStride : 0u;
}

void* ExpandableUnitHeap::GetStartAddress() const {
    // ExpandableUnitHeap owns discontiguous backing pages; retail returns null.
    return nullptr;
}

unsigned int ExpandableUnitHeap::GetTotalSize() const {
    if (!impl_) return 0;
    auto lock = ConditionalLock(impl_->mutex, impl_->threadSafe);
    unsigned int controlBytes = impl_->backing->GetMemBlockSize(impl_->controlAllocation);
    if (controlBytes == 0) controlBytes = impl_->controlSize;
    return controlBytes + impl_->unitStride * impl_->totalUnits;
}

unsigned int ExpandableUnitHeap::GetTotalFreeSize() const {
    return impl_ ? impl_->unitStride * impl_->freeUnits : 0u;
}

unsigned int ExpandableUnitHeap::GetTotalUsableSize() const {
    return impl_ ? impl_->unitStride * impl_->totalUnits : 0u;
}

ExpandableUnitHeap::~ExpandableUnitHeap() {
    if (!impl_) return;
    Impl& impl = *impl_;
    HeapBase* backing = impl.backing;
    void* control = impl.controlAllocation;

    {
        auto lock = ConditionalLock(impl.mutex, impl.threadSafe);
        for (auto& [_, page] : impl.pages) backing->Free(page.storage);
        impl.pages.clear();
        impl.availablePages.clear();
        impl.totalUnits = 0;
        impl.freeUnits = 0;
    }

    if (control) backing->Free(control);
    impl_.reset();
}

unsigned int UnitHeap::CalcHeapSize(
    unsigned int unitCount,
    unsigned int unitSize,
    unsigned short alignment,
    bool threadSafe) {

    const unsigned int normalizedAlignment = RoundUpToFour(alignment);
    const unsigned int unitStride = RoundUpMultiple(unitSize, normalizedAlignment);
    const unsigned int headerBytes = threadSafe ? 0x28u : 0x1Cu;
    const unsigned int alignedHeader = RoundUpMultiple(headerBytes, alignment);
    return unitCount * unitStride + alignedHeader;
}

void UnitHeap::Initialize(
    void* memory,
    unsigned int heapSize,
    unsigned int unitSize,
    unsigned short alignment,
    int option) {

    if (state_) return;

    const unsigned int flags = static_cast<unsigned int>(option);
    const bool threadSafe = (flags & 1u) != 0;
    const unsigned int normalizedAlignment = RoundUpToFour(alignment);
    const unsigned int unitStride = RoundUpMultiple(unitSize, normalizedAlignment);
    const unsigned int alignedHeapSize = RoundUpToFour(heapSize);
    const unsigned int headerBytes = threadSafe ? 0x28u : 0x1Cu;

    const std::uintptr_t storage = reinterpret_cast<std::uintptr_t>(memory);
    const std::uintptr_t firstUnit = RoundUpAddress(storage + headerBytes, normalizedAlignment);
    const unsigned int totalUnits = (alignedHeapSize - headerBytes) / unitStride;

    state_.emplace();
    State& state = *state_;
    state.storage = memory;
    state.heapSize = alignedHeapSize;
    state.unitStride = unitStride;
    state.alignment = normalizedAlignment;
    state.totalUnits = totalUnits;
    state.freeUnits = totalUnits;
    state.threadSafe = threadSafe;
    for (unsigned int i = 0; i < totalUnits; ++i) {
        state.freeList.push_back(firstUnit + static_cast<std::uintptr_t>(i) * unitStride);
    }
}

void* UnitHeap::Malloc(unsigned int size, unsigned short) {
    if (!state_) return nullptr;
    auto lock = ConditionalLock(mutex_, state_->threadSafe);
    if (size > state_->unitStride || state_->freeList.empty()) return nullptr;
    const std::uintptr_t address = state_->freeList.front();
    state_->freeList.pop_front();
    --state_->freeUnits;
    return reinterpret_cast<void*>(address);
}

void UnitHeap::Free(void* memory) {
    if (!state_ || !memory) return;
    auto lock = ConditionalLock(mutex_, state_->threadSafe);
    state_->freeList.push_front(reinterpret_cast<std::uintptr_t>(memory));
    ++state_->freeUnits;
}

unsigned int UnitHeap::ResizeMemBlock(void*, unsigned int size) {
    if (!state_ || size > state_->unitStride) return 0;
    return state_->unitStride;
}

unsigned int UnitHeap::GetMemBlockSize(const void*) const {
    return state_ ? state_->unitStride : 0u;
}

void* UnitHeap::GetStartAddress() const {
    return state_ ? state_->storage : nullptr;
}

unsigned int UnitHeap::GetTotalSize() const {
    return state_ ? state_->heapSize : 0u;
}

unsigned int UnitHeap::GetTotalFreeSize() const {
    return state_ ? state_->unitStride * state_->freeUnits : 0u;
}

unsigned int UnitHeap::GetTotalUsableSize() const {
    return state_ ? state_->unitStride * state_->totalUnits : 0u;
}

UnitHeap::~UnitHeap() {
    if (!state_) return;
    auto lock = ConditionalLock(mutex_, state_->threadSafe);
    state_.reset();
}

} // namespace ism
