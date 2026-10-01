#pragma once
#include <array>
#include <cstddef>
#include "fates/graphics/basic_types.hpp"
namespace nw::h3d { struct SceneState; namespace res { struct LightContent; } }
namespace nn::gr::CTR { class CommandBufferJumpHelper; struct BindSymbolVSInteger; struct BindSymbolVSFloat; }
namespace fates::decomp_detail {
int GetLightKind(const nw::h3d::res::LightContent* light);
bool IsLightEnabled(const nw::h3d::res::LightContent* light);
Color8 GetHemisphereSkyColor(const nw::h3d::res::LightContent* light);
Color8 GetHemisphereGroundColor(const nw::h3d::res::LightContent* light);
float GetHemisphereSkyWeight(const nw::h3d::res::LightContent* light);
Color8 GetAmbientLightColor(const nw::h3d::res::LightContent* light);
Color8 GetDirectionalAmbientColor(const nw::h3d::res::LightContent* light);
Color8 GetDirectionalDiffuseColor(const nw::h3d::res::LightContent* light);
void AccumulateLightColor(Color8& destination,const Color8& source);
struct LightSetCommandState { std::size_t attitudeWordCount{}; std::array<std::uint32_t,256> viewCommands{}; std::array<std::uint32_t,256> worldCommands{}; };
void CommitLightSet(nw::h3d::SceneState& scene,unsigned int setIndex,const nw::h3d::res::LightContent* hemisphere,const nw::h3d::res::LightContent* ambient,const nw::h3d::res::LightContent* const* vertex,int vertexCount,const nw::h3d::res::LightContent* const* fragment,int fragmentCount,const nn::math::MTX34& view,const nn::math::MTX34& world,LightSetCommandState& commands);
void EmitHemisphereUniformCommand(nn::gr::CTR::CommandBufferJumpHelper&,const nw::h3d::res::LightContent*,const nn::gr::CTR::BindSymbolVSInteger&,const nn::gr::CTR::BindSymbolVSFloat&,const nn::gr::CTR::BindSymbolVSFloat&,const nn::gr::CTR::BindSymbolVSFloat&,const nn::gr::CTR::BindSymbolVSFloat&);
void EmitLightAttitudeCommand(nn::gr::CTR::CommandBufferJumpHelper&,const LightSetCommandState& commands);
}
