#include "fates/memory/memory.hpp"

#include "fates/detail/allocator_runtime.hpp"
#include "fates/detail/memory_runtime.hpp"
#include "fates/memory/iallocator.hpp"

#include <cstddef>
#include <cstdint>

namespace {

constexpr std::uint32_t kApplicationHeapStart = 0x08000000u;
constexpr std::uint32_t kApplicationHeapSize = 0x00180000u;
constexpr std::uint32_t kPrimaryDeviceHeapSize = 0x01800000u;
constexpr std::uint32_t kPageMask = 0xFFFFF000u;

constexpr std::uint32_t kVramBankA = 0x00020000u;
constexpr std::uint32_t kVramBankB = 0x00030000u;
constexpr std::uintptr_t kVramAlignMask = ~std::uintptr_t{7};

constexpr std::size_t kApplicationAllocator = 0;
constexpr std::size_t kDeviceAllocator = 1;
constexpr std::size_t kDeviceTailAllocator = 2;
constexpr std::size_t kVramABAllocator = 3;
constexpr std::size_t kVramBAAllocator = 4;

} // namespace

namespace Memory {

void InitializeStartUp() {
    const std::uint32_t applicationMemory =
        fates::decomp_detail::GetApplicationMemorySize();
    const std::uint32_t usingMemory =
        fates::decomp_detail::GetUsingMemorySize();

    // Exact retail arithmetic: reserve 0x180000 bytes for the application
    // heap and page-align the remaining device-memory budget downward.
    const std::uint32_t deviceMemorySize =
        (applicationMemory - usingMemory - kApplicationHeapSize) & kPageMask;
    const std::uint32_t deviceTailSize =
        deviceMemorySize - kPrimaryDeviceHeapSize;

    fates::decomp_detail::SetProcessHeapSize(kApplicationHeapSize);
    fates::decomp_detail::SetDeviceMemorySize(deviceMemorySize);

    const std::uintptr_t deviceMemory =
        fates::decomp_detail::GetDeviceMemoryAddress();

    fates::decomp_detail::InitializeUnifiedAllocatorSlot(
        kApplicationAllocator,
        kApplicationHeapStart,
        kApplicationHeapSize);
    fates::decomp_detail::InitializeUnifiedAllocatorSlot(
        kDeviceAllocator,
        deviceMemory,
        kPrimaryDeviceHeapSize);
    fates::decomp_detail::InitializeUnifiedAllocatorSlot(
        kDeviceTailAllocator,
        deviceMemory + kPrimaryDeviceHeapSize,
        deviceTailSize);
}

void InitializeVRAM() {
    const std::uint32_t vramASize =
        fates::decomp_detail::GetVramSize(kVramBankA) & ~7u;
    const std::uint32_t vramBSize =
        fates::decomp_detail::GetVramSize(kVramBankB) & ~7u;

    const std::uintptr_t vramAStart =
        (fates::decomp_detail::GetVramStartAddress(kVramBankA) + 7u) &
        kVramAlignMask;
    const std::uintptr_t vramBStart =
        (fates::decomp_detail::GetVramStartAddress(kVramBankB) + 7u) &
        kVramAlignMask;

    fates::decomp_detail::InitializeNonUnifiedAllocatorSlot(
        kVramABAllocator,
        vramAStart,
        vramASize);
    fates::decomp_detail::InitializeNonUnifiedAllocatorSlot(
        kVramBAAllocator,
        vramBStart,
        vramBSize);
}

IAllocator* GetAllocator(const void* address) {
    for (std::size_t i = 0;
         i < fates::decomp_detail::kAllocatorRoutingCount;
         ++i) {
        IAllocator* const allocator =
            fates::decomp_detail::gAllocatorRoutingTable[i];
        if (allocator != nullptr && allocator->HasAddress(address)) {
            return allocator;
        }
    }
    return nullptr;
}

} // namespace Memory
