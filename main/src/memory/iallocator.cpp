#include "fates/memory/iallocator.hpp"

#include "fates/detail/allocator_runtime.hpp"

#include <cstddef>
#include <cstdint>

namespace {

std::uint32_t Address32(const void* memory) {
    return static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(memory));
}

IAllocator* FindOwningAllocator(const void* memory) {
    for (std::size_t i = 0;
         i < fates::decomp_detail::kAllocatorRoutingCount;
         ++i) {
        IAllocator* const candidate =
            fates::decomp_detail::gAllocatorRoutingTable[i];
        if (candidate != nullptr && candidate->HasAddress(memory)) {
            return candidate;
        }
    }
    return nullptr;
}

IAllocator* ResolveAllocator(IAllocator* preferred, const void* memory) {
    if (preferred == nullptr) {
        return FindOwningAllocator(memory);
    }
    if (memory == nullptr || preferred->HasAddress(memory)) {
        return preferred;
    }
    return FindOwningAllocator(memory);
}

void* AllocateWithFallback(
    IAllocator& preferred,
    unsigned int size,
    unsigned short alignment) {
    IAllocator* owner = &preferred;
    void* memory = fates::decomp_detail::AllocateThroughAllocatorBackend(
        *owner, size, alignment);

    if (memory == nullptr) {
        owner = fates::decomp_detail::GetAllocationFallback(preferred);
        if (owner != nullptr) {
            memory = fates::decomp_detail::AllocateThroughAllocatorBackend(
                *owner, size, alignment);
        }
    }

    if (memory != nullptr && owner != nullptr) {
        fates::decomp_detail::ApplyAllocationDebugFill(*owner, memory);
    }
    return memory;
}

} // namespace

void* IAllocator::Alloc(unsigned int size, unsigned char alignment) {
    // Retail's 0x0011F5A8 entry is a four-byte fall-through veneer into
    // IAllocator::Malloc.
    return Malloc(size, alignment);
}

void* IAllocator::Malloc(unsigned int size, unsigned short alignment) {
    return AllocateWithFallback(*this, size, alignment);
}

void* IAllocator::TryMalloc(unsigned int size, unsigned short alignment) {
    // The retail wrappers share the same routing/fallback body. Any backend
    // distinction remains behind AllocateThroughAllocatorBackend.
    return AllocateWithFallback(*this, size, alignment);
}

bool IAllocator::HasAddress(const void* memory) const {
    const std::uint32_t start =
        fates::decomp_detail::GetAllocatorStartAddress32(*this);
    const std::uint32_t end =
        start + fates::decomp_detail::GetAllocatorTotalSize(*this);
    const std::uint32_t address = Address32(memory);

    // Exact retail unsigned ARM32 half-open test from 0x00506F98:
    // start <= address && address < start + size.
    return start <= address && address < end;
}

void IAllocator::Free(void* memory) {
    IAllocator* const owner = ResolveAllocator(this, memory);
    if (owner == nullptr) {
        return;
    }
    fates::decomp_detail::FreeThroughAllocatorBackend(*owner, memory);
}

bool IAllocator::Resize(void* memory, unsigned int size) {
    IAllocator* const owner = ResolveAllocator(this, memory);
    if (owner == nullptr) {
        return false;
    }
    return fates::decomp_detail::ResizeThroughAllocatorBackend(
        *owner, memory, size);
}
