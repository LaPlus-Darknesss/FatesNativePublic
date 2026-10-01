#pragma once
#include <cstdint>
#include "fates/detail/command_runtime.hpp"

class CommandBuffer {
public:
    CommandBuffer();
    void Initialize();
    std::uint32_t Bind();
    void Copy(unsigned int sourceOffset, unsigned int byteCount);
    static std::uint32_t GetUsedRequestCount();
    static std::uint32_t GetUsedBufferOffset();
    static std::uintptr_t GetCurrentBufferAddress();
private:
    fates::decomp_detail::CommandBufferRuntime runtimeState_{};
};
