#include "fates/graphics/command_buffer.hpp"
#include "fates/graphics/command_alter.hpp"

CommandBuffer::CommandBuffer() = default;
void CommandBuffer::Initialize(){ fates::decomp_detail::InitializeVendorCommandBuffer(runtimeState_); }
std::uint32_t CommandBuffer::Bind(){ return fates::decomp_detail::BindVendorCommandBuffer(runtimeState_); }
void CommandBuffer::Copy(unsigned int sourceOffset,unsigned int byteCount){
    const auto dst=GetCurrentBufferAddress();
    fates::decomp_detail::CopyVendorCommandBytes(dst,sourceOffset,byteCount);
    CommandAlter::Execute(static_cast<unsigned int>(dst),sourceOffset,static_cast<unsigned int>(dst-runtimeState_.storageBase));
}
std::uint32_t CommandBuffer::GetUsedRequestCount(){ return fates::decomp_detail::QueryUsedRequestCount(); }
std::uint32_t CommandBuffer::GetUsedBufferOffset(){ return fates::decomp_detail::QueryUsedBufferOffset(); }
std::uintptr_t CommandBuffer::GetCurrentBufferAddress(){ return fates::decomp_detail::QueryCurrentBufferAddress(); }
