#pragma once

#include <cstddef>
#include <cstdint>

namespace fates::decomp_detail {

// CTR process/device-memory queries remain platform services. Human source owns
// the retail partitioning policy and allocator-slot topology.
std::uint32_t GetApplicationMemorySize();
std::uint32_t GetUsingMemorySize();
void SetProcessHeapSize(std::uint32_t size);
void SetDeviceMemorySize(std::uint32_t size);
std::uintptr_t GetDeviceMemoryAddress();

void InitializeUnifiedAllocatorSlot(
    std::size_t slot,
    std::uintptr_t start,
    std::uint32_t size);

std::uint32_t GetVramSize(std::uint32_t bank);
std::uintptr_t GetVramStartAddress(std::uint32_t bank);
void InitializeNonUnifiedAllocatorSlot(
    std::size_t slot,
    std::uintptr_t start,
    std::uint32_t size);

} // namespace fates::decomp_detail
