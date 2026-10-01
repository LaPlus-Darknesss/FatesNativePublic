#pragma once
#include <cstddef>
#include <cstdint>

namespace fates::decomp_detail {
struct CommandBufferRuntime {
    std::uint32_t handle{};
    std::uintptr_t storageBase{};
};
void InitializeVendorCommandBuffer(CommandBufferRuntime& state);
std::uint32_t BindVendorCommandBuffer(CommandBufferRuntime& state);
std::uint32_t QueryUsedRequestCount();
std::uint32_t QueryUsedBufferOffset();
std::uintptr_t QueryCurrentBufferAddress();
void CopyVendorCommandBytes(std::uintptr_t destination, std::uint32_t sourceOffset, std::uint32_t byteCount);
void ReplacePicaCommand(std::uintptr_t destination, int matrixSlot, const void* data);
void ReplaceRawCommand(std::uintptr_t destination, const void* data, std::size_t bytes);
}
