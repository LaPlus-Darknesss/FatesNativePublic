#pragma once
#include <cstdint>
#include "fates/graphics/basic_types.hpp"

class ICamera; class ResFile; class SceneNode; class SceneSystem; class RenderState;
namespace fates::decomp_detail {
struct SceneRuntimeState { const void* sceneContent{}; const ICamera* camera{}; void* vendorSceneState{}; };
void InitializeSceneRuntime(SceneRuntimeState& state);
void DestroySceneRuntime(SceneRuntimeState& state);
bool LoadSceneResourceFromPath(ResFile& target,const char* path);
const ResFile* GetDefaultSceneResource();
bool SelectScene(SceneRuntimeState& state,const ResFile& resources,const char* identifier);
void ClearSelectedScene(SceneRuntimeState& state);
void UpdateSceneFrame(SceneRuntimeState& state,const ResFile& resources,float frame);
std::uint32_t GetSceneDeltaTick();
void UpdateSceneCameraRuntime(SceneRuntimeState& state);
void SubmitSceneDrawCallback(SceneSystem& scene);
void LoadSceneLights(SceneRuntimeState& state,const ResFile& resources);
void ResetSceneLights(SceneRuntimeState& state);
void CommitSceneFogAndLights(SceneRuntimeState& state,const ResFile& resources);
bool IsSceneRenderable(const SceneRuntimeState& state);
void UpdateSceneNodeVirtual(SceneNode& node,RenderState& state);
void DrawSceneNodeOpaqueVirtual(SceneNode& node,RenderState& state);
void DrawSceneNodeTranslucentVirtual(SceneNode& node,RenderState& state);
float SceneNodeSquaredDistance(const ICamera* camera,const AABB& worldBox);
bool SceneNodeRejected(const ICamera* camera,const AABB& worldBox);
AABB TransformAABB(const AABB& local,const nn::math::MTX34& world);
}
