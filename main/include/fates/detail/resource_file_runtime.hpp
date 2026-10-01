#pragma once

#include <cstdint>

class FileObject;
class ShaderObj;

namespace nw::h3d::res {
struct ModelContent;
struct TextureContent;
}

namespace fates::decomp_detail {

// H3D is Nintendo middleware, not Fates-owned source.  The retail executable
// proves which ResourceBinary dictionaries/tables the Fates ResFile facade
// consumes, but durable game-facing source keeps those binary layouts behind
// this narrow adapter rather than copying SDK offsets into ResFile.
enum class ResourceCategory : std::uint8_t {
    Model,
    Texture,
    Light,
    Camera,
    Fog,
    SkeletalAnimation,
    MaterialAnimation,
    VisibilityAnimation,
    LightAnimation,
    CameraAnimation,
    FogAnimation,
    Scene,
};

constexpr int kInvalidResourceIndex = 0xFFFF;

bool IsLinkedResourceFileObject(const FileObject* object);
void LinkResourceFileObjects(FileObject* destination, const FileObject* source);
int GetResourceCount(const FileObject* object, ResourceCategory category);
int FindResourceIndex(
    const FileObject* object,
    ResourceCategory category,
    const char* identifier);
const void* GetResourceAt(
    const FileObject* object,
    ResourceCategory category,
    int index);
bool HasAnyModelAnimation(const FileObject* object, const char* identifier);

inline const nw::h3d::res::ModelContent* GetModelContent(
    const FileObject* object,
    int index) {
    return static_cast<const nw::h3d::res::ModelContent*>(
        GetResourceAt(object, ResourceCategory::Model, index));
}

inline const nw::h3d::res::TextureContent* GetTextureContent(
    const FileObject* object,
    int index) {
    return static_cast<const nw::h3d::res::TextureContent*>(
        GetResourceAt(object, ResourceCategory::Texture, index));
}

// Fates' Model class owns lifecycle and FileBase binding.  Nintendo H3D state
// sizing/initialization, PICA shader-symbol binding, and the opaque state-array
// layouts remain a vendor-runtime contract until/if the project deliberately
// reconstructs SDK internals in a separate middleware lane.
struct ModelRuntimeState {
    const nw::h3d::res::ModelContent* content{};
    void* deviceState{};
    void* materialState{};
    void* shaderBindings{};
    void* visibilityBits{};
    std::uint32_t shaderBindingCount{};
    std::uint32_t visibilityCount{};
    std::uint16_t flags{};
};

void ResetModelRuntimeState(ModelRuntimeState& state);
void InitializeModelRuntimeState(
    ModelRuntimeState& state,
    const nw::h3d::res::ModelContent* content);

// ShaderObj::Transfer is first-party orchestration over nn::gr command makers.
// Keep the command-buffer/PICA implementation in the graphics runtime boundary.
void TransferShaderCommands(const ShaderObj& shader);

} // namespace fates::decomp_detail
