#pragma once
#include "fates/graphics/basic_types.hpp"
class ICamera; class CameraParam; class CameraState; class PostEffectContext; class ITexture; class GameEffect;
namespace fates::decomp_detail {
void CameraResetRuntime(ICamera& camera);
void CameraSetupOrthoRuntime(ICamera& camera,int displayType);
void CameraUpdateProjection(ICamera& camera);
void CameraUpdateView(ICamera& camera);
void CameraUpdateWorld(ICamera& camera);
void CameraUpdateStereo(ICamera& camera);
void CameraUpdateFrustum(ICamera& camera);
void CameraCopyRuntime(ICamera& dst,const ICamera& src);
float QueryStereoParallax(const ICamera& camera,float depth);
bool QueryStereoEnabled(const ICamera& camera);
nn::math::VEC3 WorldToScreen(const ICamera& camera,const nn::math::VEC3& world);
void CameraStateHitCheck(CameraState& state,CameraParam& param);
void* AllocatePostEffectBuffer(unsigned int size,unsigned int alignment);
void FreePostEffectBuffer(void* p);
void PostEffectDefaultSetup(PostEffectContext& ctx);
void PostEffectTextureAssign(PostEffectContext& ctx,int slot,const ITexture* texture,unsigned int sampler);
void PostEffectTextureAssignRaw(PostEffectContext& ctx,int slot,unsigned int address,unsigned int width,unsigned int height,unsigned int format);
void PostEffectInitBeforeDraw(PostEffectContext& ctx,int width,int height);
void PostEffectGaussPass1(PostEffectContext& ctx,int a,int b,int c);
void PostEffectGaussPass2(PostEffectContext& ctx,int a,int b,int c);
void PostEffectDrawQuads(PostEffectContext& ctx,int width,int height);
void PostEffectShaderSetup(void* shaderOwner);
void PrimPostEffectDraw(void* callback);
void PrimPostEffectBegin(PostEffectContext& ctx);
void PrimPostEffectEnd();
float GetWorldTimeRate();
void ProcAdvance(void* proc);
void ProcDelete(void* proc);
void* ResolveTrackingObject(void* handle);
void ApplyTrackingEffect(GameEffect effect,void* trackedObject);
void CleanupTrackingEffect(GameEffect effect);
void* ResolveBattleObject(void* handle);
void ApplyBattleEffect(GameEffect effect,void* object,unsigned int flags,const nn::math::VEC3* extraTranslate);
class SceneSystem* GetFieldScene();
const char* GetSecondaryEffectLabel(const GameEffect& effect);
}
