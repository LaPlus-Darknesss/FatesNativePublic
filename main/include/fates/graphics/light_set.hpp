#pragma once
#include <array>
#include "fates/detail/light_runtime.hpp"
class LightSet {
public:
    LightSet();
    ~LightSet();
    bool Add(const nw::h3d::res::LightContent* light);
    void Reset();
    void Commit(nw::h3d::SceneState& scene,const nn::math::MTX34& view,const nn::math::MTX34& world);
    bool Remove(const nw::h3d::res::LightContent* light);
    Color8 GetSkyColor() const;
    float GetSkyWeight() const;
    Color8 GetGroundColor() const;
    Color8 GetAmbientColor() const;
    Color8 GetDiffuseColor() const;
    const nw::h3d::res::LightContent* GetDirectionaLight() const;
    void MakeUniformCommand(nn::gr::CTR::CommandBufferJumpHelper&,const nn::gr::CTR::BindSymbolVSInteger&,const nn::gr::CTR::BindSymbolVSFloat&,const nn::gr::CTR::BindSymbolVSFloat&,const nn::gr::CTR::BindSymbolVSFloat&,const nn::gr::CTR::BindSymbolVSFloat&) const;
    void MakeAttitudeCommand(nn::gr::CTR::CommandBufferJumpHelper&) const;
    bool IsExist() const;
private:
    unsigned int setIndex_{};
    const nw::h3d::res::LightContent* hemisphere_{};
    const nw::h3d::res::LightContent* ambient_{};
    std::array<const nw::h3d::res::LightContent*,8> vertex_{};
    std::array<const nw::h3d::res::LightContent*,8> fragment_{};
    int vertexCount_{};
    int fragmentCount_{};
    fates::decomp_detail::LightSetCommandState commands_{};
};
