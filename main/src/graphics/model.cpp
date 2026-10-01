#include "fates/graphics/model.hpp"

#include "fates/graphics/render_state.hpp"

Model::Model() {
    fates::decomp_detail::ResetModelRuntimeState(runtimeState_);
}

Model::~Model() {
    FreeModel();
}

void Model::ResetLoadedModel() {
    FreeModel();
}

void Model::FreeModel() {
    if (runtimeState_.content == nullptr) return;
    fates::decomp_detail::FreeModelExecutionState(runtimeState_);
    resourceFile_.Free();
    fates::decomp_detail::ResetModelRuntimeState(runtimeState_);
}

bool Model::LoadModel(const ResFile& resources, const char* identifier) {
    if (!resources.IsDone()) return false;
    int index = fates::decomp_detail::kInvalidResourceIndex;
    if (identifier == nullptr) {
        if (resources.GetModelCount() > 0) index = 0;
    } else {
        index = resources.FindModelIndex(identifier);
    }
    return LoadModel(resources, index);
}

bool Model::LoadModel(const ResFile& resources, int index) {
    ResetLoadedModel();
    if (!resources.IsDone() || index == fates::decomp_detail::kInvalidResourceIndex) return false;
    const auto* content = resources.GetModel(index);
    if (content == nullptr) return false;
    resourceFile_.ReplaceHandle(resources);
    runtimeState_.content = content;
    fates::decomp_detail::InitializeModelRuntimeState(runtimeState_, content);
    return true;
}

bool Model::ReplaceTexture(const nw::h3d::res::TextureContent* texture, const char* materialOrSampler) {
    if (runtimeState_.content == nullptr || texture == nullptr) return false;
    return fates::decomp_detail::ReplaceModelTexture(runtimeState_, texture, materialOrSampler);
}

bool Model::ReplaceTexture(const ResFile& resources) {
    if (runtimeState_.content == nullptr || !resources.IsDone()) return false;
    // New Pass-23 evidence sharpens the Pass-22 retained FileBase member: the
    // retail member is used through ResFile::Link when importing replacement
    // textures, so durable source now keeps the stronger ResFile type.
    resourceFile_.Link(resources);
    return fates::decomp_detail::ReplaceModelTexturesByName(runtimeState_, resources);
}

void Model::SetConstantColor(int index, const Color8& color, const char* material) {
    if (runtimeState_.content == nullptr) return;
    fates::decomp_detail::SetModelConstantColor(runtimeState_, index, color, material);
}

bool Model::SetMaterialVisible(const char* material, bool visible) {
    if (runtimeState_.content == nullptr || material == nullptr) return false;
    return fates::decomp_detail::SetModelMaterialVisible(runtimeState_, material, visible);
}

void Model::Update(const ICamera* camera, const nn::math::MTX34& world) {
    if (runtimeState_.content == nullptr) return;
    // Retail updates skeleton/billboard state and then refreshes hierarchical
    // mesh AABBs. Raw H3D state arrays remain vendor-facing runtime detail.
    fates::decomp_detail::UpdateModelExecutionState(runtimeState_, camera, world);
}

void Model::DrawOpa(RenderState& renderState, const nn::math::MTX34& world) {
    if (!fates::decomp_detail::IsModelDrawable(runtimeState_) ||
        !fates::decomp_detail::DrawsOpaqueSeparately(runtimeState_)) return;
    const bool persistent = fates::decomp_detail::UsesPersistentDrawState(runtimeState_);
    renderState.BeginDraw(persistent);
    if (fates::decomp_detail::HasModelLayer(runtimeState_, 0)) {
        DrawLayer(renderState, static_cast<nw::h3d::TranslucencyLayer::enum_t>(0), world);
    }
    renderState.EndDraw(persistent);
}

void Model::DrawXlu(RenderState& renderState, const nn::math::MTX34& world) {
    if (!fates::decomp_detail::IsModelDrawable(runtimeState_)) return;
    const bool persistent = fates::decomp_detail::UsesPersistentDrawState(runtimeState_);
    renderState.BeginDraw(persistent);
    const bool opaqueSeparate = fates::decomp_detail::DrawsOpaqueSeparately(runtimeState_);
    if (!opaqueSeparate && fates::decomp_detail::HasModelLayer(runtimeState_, 0))
        DrawLayer(renderState, static_cast<nw::h3d::TranslucencyLayer::enum_t>(0), world);
    if (fates::decomp_detail::HasModelLayer(runtimeState_, 1))
        DrawLayer(renderState, static_cast<nw::h3d::TranslucencyLayer::enum_t>(1), world);
    if (fates::decomp_detail::HasModelLayer(runtimeState_, 2))
        DrawLayer(renderState, static_cast<nw::h3d::TranslucencyLayer::enum_t>(2), world);
    if (fates::decomp_detail::HasModelLayer(runtimeState_, 3))
        DrawLayer(renderState, static_cast<nw::h3d::TranslucencyLayer::enum_t>(3), world);
    renderState.EndDraw(persistent);
}

void Model::DrawMesh(RenderState& renderState, nw::h3d::TranslucencyLayer::enum_t layer, const nw::h3d::res::MeshData& mesh, const nn::math::MTX34& world) {
    fates::decomp_detail::DrawModelMesh(runtimeState_, renderState, layer, mesh, world);
}

void Model::DrawLayer(RenderState& renderState, nw::h3d::TranslucencyLayer::enum_t layer, const nn::math::MTX34& world) {
    // Retail sorts/filters visible meshes for the requested translucency layer,
    // then feeds each surviving mesh through DrawMesh. Keep H3D mesh-array and
    // camera-distance bookkeeping behind the adapter while preserving layer policy.
    fates::decomp_detail::DrawModelLayer(runtimeState_, renderState, static_cast<int>(layer), world);
}

const char* Model::GetBoneName(int index) const { return fates::decomp_detail::GetModelBoneName(runtimeState_, index); }
int Model::GetBoneCount() const { return fates::decomp_detail::GetModelBoneCount(runtimeState_); }
int Model::GetBoneIndex(const char* name) const { return fates::decomp_detail::GetModelBoneIndex(runtimeState_, name); }
AABB Model::GetModelAABB() const { return fates::decomp_detail::CalculateModelAABB(runtimeState_); }
const nn::math::MTX34& Model::GetBoneMatrix(const char* name) const { return GetBoneMatrix(GetBoneIndex(name)); }
const nn::math::MTX34& Model::GetBoneMatrix(int index) const { return fates::decomp_detail::GetModelBoneMatrix(runtimeState_, index); }
const nn::math::MTX34& Model::GetModelMatrix() const { return fates::decomp_detail::GetModelMatrix(runtimeState_); }
nn::math::VEC3 Model::GetBoneTranslate(const char* name) const { return GetBoneTranslate(GetBoneIndex(name)); }
nn::math::VEC3 Model::GetBoneTranslate(int index) const { return fates::decomp_detail::GetModelBoneTranslate(runtimeState_, index); }
int Model::GetMaterialCount() const { return fates::decomp_detail::GetModelMaterialCount(runtimeState_); }
int Model::GetMaterialIndex(const char* name) const { return fates::decomp_detail::GetModelMaterialIndex(runtimeState_, name); }
