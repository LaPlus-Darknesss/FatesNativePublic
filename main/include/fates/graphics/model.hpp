#pragma once

#include "fates/detail/model_execution_runtime.hpp"
#include "fates/io/res_file.hpp"
#include "fates/graphics/model_state.hpp"

class ICamera;
class RenderState;

class Model : public ModelState {
public:
    Model();
    ~Model();

    bool LoadModel(const ResFile& resources, const char* identifier);
    bool LoadModel(const ResFile& resources, int index);
    void FreeModel();

    bool ReplaceTexture(const nw::h3d::res::TextureContent* texture, const char* materialOrSampler = nullptr);
    bool ReplaceTexture(const ResFile& resources);
    void SetConstantColor(int index, const Color8& color, const char* material = nullptr);
    bool SetMaterialVisible(const char* material, bool visible);

    void Update(const ICamera* camera, const nn::math::MTX34& world);
    void DrawOpa(RenderState& renderState, const nn::math::MTX34& world);
    void DrawXlu(RenderState& renderState, const nn::math::MTX34& world);

    const char* GetBoneName(int index) const;
    int GetBoneCount() const;
    int GetBoneIndex(const char* name) const;
    AABB GetModelAABB() const;
    const nn::math::MTX34& GetBoneMatrix(const char* name) const;
    const nn::math::MTX34& GetBoneMatrix(int index) const;
    const nn::math::MTX34& GetModelMatrix() const;
    nn::math::VEC3 GetBoneTranslate(const char* name) const;
    nn::math::VEC3 GetBoneTranslate(int index) const;
    int GetMaterialCount() const;
    int GetMaterialIndex(const char* name) const;

private:
    void ResetLoadedModel();
    void DrawMesh(RenderState& renderState, nw::h3d::TranslucencyLayer::enum_t layer, const nw::h3d::res::MeshData& mesh, const nn::math::MTX34& world);
    void DrawLayer(RenderState& renderState, nw::h3d::TranslucencyLayer::enum_t layer, const nn::math::MTX34& world);

    ResFile resourceFile_{};
    fates::decomp_detail::ModelRuntimeState runtimeState_{};
};
