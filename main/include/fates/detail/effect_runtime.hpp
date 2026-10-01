#pragma once
#include <cstddef>
#include <cstdint>
#include "fates/graphics/basic_types.hpp"

class IAllocator;
class RenderState;
class SceneSystem;
class EffectObject;
class EffectEmitter;
class EffectNode;
class GameEffectObject;
namespace nw::h3d::res { struct LightContent; }

namespace fates::decomp_detail {
struct EffectVendorSystem;
struct EffectResourceHandle { int slot{-1}; const void* data{}; };
struct EffectEmitterHandle { void* vendorEmitter{}; std::uint32_t generation{}; };
struct GameEffectRuntimeData { const void* definition{}; const char* label{}; };

bool InitializeEffectVendorSystem();
void ResetEffectVendorFrameState();
EffectVendorSystem* GetEffectVendorSystem();
IAllocator* GetEffectObjectAllocator();
int AcquireEffectResourceSlot();
void ReleaseEffectResourceSlot(int slot);
void RegisterEffectResource(EffectResourceHandle& out,const void* data,int slot);
void UnregisterEffectResource(EffectResourceHandle& handle);

bool CreateEmitter(EffectEmitterHandle& out,const EffectObject& resource,const char* name);
bool CreateEmitter(EffectEmitterHandle& out,const EffectObject& resource,int index);
void KillEmitter(EffectEmitterHandle& emitter);
bool IsEmitterAlive(const EffectEmitterHandle& emitter);
void FadeEmitter(EffectEmitterHandle& emitter);
void SetEmitterMatrix(EffectEmitterHandle& emitter,const nn::math::MTX34& world);
void SetEmitterColor(EffectEmitterHandle& emitter,const Color8& color);
void SetEmitterRatio(EffectEmitterHandle& emitter,float ratio);
void ForceEmitterCalc(EffectEmitterHandle& emitter,int frames);
void DrawEmitter(EffectEmitterHandle& emitter,RenderState& renderState,float frameRemainder);

const char* GetDefaultEffectModelEntry();
const char* GetDefaultEffectEmitterEntry();
const char* GetEffectCacheFilePath(const char* label);
void TouchEffectCacheEntry(const char* label);

GameEffectObject* ResolveGameEffect(std::uint32_t handle);
const GameEffectObject* ResolveGameEffect(std::uint32_t handle,const void* tag);
std::uint32_t RegisterGameEffectObject(GameEffectObject* object);
void UnregisterGameEffectObject(GameEffectObject* object);
void DeleteGameEffectObject(GameEffectObject* object);
void* GetGameEffectManagerSingleton();
void SetGameEffectManagerSingleton(void* manager);

bool ShouldSkipEffectForBlackout();
bool ShouldSkipEffectForInput();
void ApplyEffectDefinitionInit(GameEffectObject& object);
void ApplyEffectDefinitionTick(GameEffectObject& object,float deltaFrame);
void ApplyEffectDefinitionLighting(GameEffectObject& object,SceneSystem* scene);
void RemoveEffectDefinitionLighting(GameEffectObject& object,SceneSystem* scene);
void UpdateEffectAttachment(GameEffectObject& object);
void UpdateEffectCallback(GameEffectObject& object);
}
