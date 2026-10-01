#pragma once

#include <cstdint>
#include "fates/graphics/basic_types.hpp"
#include "fates/detail/resource_file_runtime.hpp"

class ICamera;
class RenderState;
class ResFile;

namespace nw::h3d {
namespace TranslucencyLayer { enum enum_t : int; }
namespace res { struct MeshData; struct TextureContent; }
}

namespace fates::decomp_detail {

// The methods in this adapter are the H3D/PICA-facing half of first-party
// Model orchestration.  Model.cpp retains the game-owned policy: lifetime,
// layer selection, visibility, resource retention and public query semantics.
void FreeModelExecutionState(ModelRuntimeState& state);
bool ReplaceModelTexture(
    ModelRuntimeState& state,
    const nw::h3d::res::TextureContent* texture,
    const char* materialOrSampler);
bool ReplaceModelTexturesByName(ModelRuntimeState& state, const ResFile& resources);
void SetModelConstantColor(ModelRuntimeState& state, int index, const Color8& color, const char* material);
bool SetModelMaterialVisible(ModelRuntimeState& state, const char* material, bool visible);
void UpdateModelExecutionState(ModelRuntimeState& state, const ICamera* camera, const nn::math::MTX34& world);

bool IsModelDrawable(const ModelRuntimeState& state);
bool DrawsOpaqueSeparately(const ModelRuntimeState& state);
bool UsesPersistentDrawState(const ModelRuntimeState& state);
bool HasModelLayer(const ModelRuntimeState& state, int layer);
void DrawModelLayer(ModelRuntimeState& state, RenderState& renderState, int layer, const nn::math::MTX34& world);
void DrawModelMesh(
    ModelRuntimeState& state,
    RenderState& renderState,
    nw::h3d::TranslucencyLayer::enum_t layer,
    const nw::h3d::res::MeshData& mesh,
    const nn::math::MTX34& world);

const char* GetModelBoneName(const ModelRuntimeState& state, int index);
int GetModelBoneCount(const ModelRuntimeState& state);
int GetModelBoneIndex(const ModelRuntimeState& state, const char* name);
AABB CalculateModelAABB(const ModelRuntimeState& state);
const nn::math::MTX34& GetModelBoneMatrix(const ModelRuntimeState& state, int index);
const nn::math::MTX34& GetModelMatrix(const ModelRuntimeState& state);
nn::math::VEC3 GetModelBoneTranslate(const ModelRuntimeState& state, int index);
int GetModelMaterialCount(const ModelRuntimeState& state);
int GetModelMaterialIndex(const ModelRuntimeState& state, const char* name);

} // namespace fates::decomp_detail
