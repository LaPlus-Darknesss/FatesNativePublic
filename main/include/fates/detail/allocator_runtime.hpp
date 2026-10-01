#pragma once

#include <cstddef>
#include <cstdint>

class IAllocator;

namespace fates::decomp_detail {

constexpr std::size_t kAllocatorRoutingCount = 5;

// Provisional readable name for the five IAllocator* entries reached through
// the retail global pointer literal 0x0070403C.
extern IAllocator* gAllocatorRoutingTable[kAllocatorRoutingCount];

// These isolate the still-unpromoted backend/vtable object stored through
// IAllocator's retail +0x04 field.
std::uint32_t GetAllocatorStartAddress32(const IAllocator& allocator);
std::uint32_t GetAllocatorTotalSize(const IAllocator& allocator);

void* AllocateThroughAllocatorBackend(
    IAllocator& allocator,
    unsigned int size,
    unsigned short alignment);
IAllocator* GetAllocationFallback(const IAllocator& allocator);
void ApplyAllocationDebugFill(IAllocator& allocator, void* memory);

void FreeThroughAllocatorBackend(IAllocator& allocator, void* memory);
bool ResizeThroughAllocatorBackend(
    IAllocator& allocator,
    void* memory,
    unsigned int size);

} // namespace fates::decomp_detail
