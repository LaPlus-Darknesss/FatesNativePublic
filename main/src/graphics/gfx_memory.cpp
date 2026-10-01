#include "fates/graphics/gfx_memory.hpp"

#include "fates/detail/allocator_runtime.hpp"
#include "fates/detail/gfx_memory_runtime.hpp"
#include "fates/io/delay_manager.hpp"
#include "fates/memory/iallocator.hpp"

#include <cstddef>

namespace {

constexpr std::size_t kDeviceAllocator = 1;
constexpr std::size_t kVramABAllocator = 3;
constexpr std::size_t kVramBAAllocator = 4;

void* TryAllocate(
    std::size_t allocatorIndex,
    unsigned int size,
    int alignment) {
    if (size == 0) {
        return nullptr;
    }

    IAllocator* const allocator =
        fates::decomp_detail::gAllocatorRoutingTable[allocatorIndex];
    if (allocator == nullptr) {
        return nullptr;
    }

    // Retail truncates the int alignment argument to 16 bits before tail-
    // calling IAllocator::TryMalloc.
    return allocator->TryMalloc(
        size,
        static_cast<unsigned short>(alignment));
}

} // namespace

namespace GfxMemory {

void Initialize() {
    if (!fates::decomp_detail::InitializeGraphicsMemoryBackend()) {
        fates::decomp_detail::PanicGraphicsMemoryInitialization();
    }
    fates::decomp_detail::ResetGraphicsMemoryTracking();
}

void FreeAddress(void* address) {
    // Retail is a direct tail branch to DelayManager::Free.
    DelayManager::Free(address);
}

void* TryAllocDevice(unsigned int size, int alignment) {
    return TryAllocate(kDeviceAllocator, size, alignment);
}

void* TryAllocVramAB(unsigned int size, int alignment) {
    return TryAllocate(kVramABAllocator, size, alignment);
}

void* TryAllocVramBA(unsigned int size, int alignment) {
    return TryAllocate(kVramBAAllocator, size, alignment);
}

} // namespace GfxMemory
