#pragma once

#include <cstdint>

class IAllocator;

namespace nw::h3d::res {
struct ResourceBinary;
struct TextureContent;
}

namespace fates::decomp_detail {

// Readable source keeps the Nintendo H3D binary object opaque while making
// the Fates-owned setup/link/cleanup state machine explicit.
struct ResourceObjectState {
    std::uint32_t flags{};
    nw::h3d::res::ResourceBinary* resourceBinary{};
    void* relocationA{};
    void* relocationB{};
};

constexpr std::uint32_t kResourceSetupFlag = 0x00004000u;
constexpr std::uint32_t kResourceDelayedRelocationFlag = 0x00008000u;
constexpr std::uint32_t kResourceRelocationRequestedFlag = 0x00000040u;

bool HasSupportedResourceHeader(const void* data);
void PrepareResourceStorage(
    ResourceObjectState& state,
    const void* data,
    std::uint32_t sourceFlags);
bool TryInitializeResourceBinary(ResourceObjectState& state, const void* data);
void LinkResourceContents(
    ResourceObjectState& state,
    nw::h3d::res::ResourceBinary* resourceBinary);
void FinalizeResourcePlacement(ResourceObjectState& state, const void* data);
void TryUninitializeResourceBinary(ResourceObjectState& state);
void ReleaseResourceStorage(ResourceObjectState& state);
unsigned int GetRelocatableResourceSize(
    const ResourceObjectState& state,
    const void* data);
void FinishResourceRelocation(void* object);

IAllocator* GetResourceAllocator();

int GetResourceTextureCount(const ResourceObjectState& state);
const nw::h3d::res::TextureContent* GetResourceTextureContent(
    const ResourceObjectState& state,
    int index);
int FindResourceTextureIndex(
    const ResourceObjectState& state,
    const char* identifier);

} // namespace fates::decomp_detail
