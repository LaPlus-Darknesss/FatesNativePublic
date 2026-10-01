#pragma once
#include <cstddef>
#include <cstdint>
#include "fates/graphics/basic_types.hpp"

class Model;
namespace nw::h3d { struct AnimBlendState; namespace res { struct AnimContent; struct CameraContent; struct LightContent; } }

namespace fates::decomp_detail {
struct AnimationBindingState {
    void* deviceMemory{};
    void* vendorBinding{};
    const Model* boundModel{};
};
struct AnimationBlendState {
    void* vendorState{};
    const Model* model{};
};
struct CameraAnimationState {
    const nw::h3d::res::CameraContent* camera{};
    nn::math::VEC3 pivotOffset{};
    nn::math::VEC3 translationOffset{};
    nn::math::VEC3 rotationOffset{};
    float evaluated[13]{};
};
struct LightAnimationState {
    void* mutableLight{};
    nn::math::VEC3 basePosition{};
};

float GetAnimationEndFrame(const nw::h3d::res::AnimContent* content);
bool IsAnimationLooping(const nw::h3d::res::AnimContent* content);
void SetAnimationLooping(const nw::h3d::res::AnimContent* content,bool loop);
bool InitializeAnimationBinding(AnimationBindingState& state,const nw::h3d::res::AnimContent* content);
void DestroyAnimationBinding(AnimationBindingState& state);
float RandomAnimationFrame(float endFrame);
const char* GetCameraResourceName(const nw::h3d::res::CameraContent* camera);
const char* GetLightResourceName(const nw::h3d::res::LightContent* light);

void EnsureAnimationBlendState(AnimationBlendState& state,Model& model);
void DestroyAnimationBlendState(AnimationBlendState& state);
void ResetModelAnimationState(Model& model);
void NormalizeSkeletalBlend(AnimationBlendState& state,Model& model);
void ApplySkeletalAnimation(const nw::h3d::res::AnimContent* content,AnimationBindingState& binding,Model& model,float frame,float weight,AnimationBlendState* blend);
void ApplyMaterialAnimation(const nw::h3d::res::AnimContent* content,AnimationBindingState& binding,Model& model,float frame);
void ApplyVisibilityAnimation(const nw::h3d::res::AnimContent* content,AnimationBindingState& binding,Model& model,float frame);

void ResetCameraAnimationState(CameraAnimationState& state);
bool EvaluateCameraAnimation(CameraAnimationState& state,const nw::h3d::res::AnimContent* content,AnimationBindingState& binding,float frame);

bool CloneLightForAnimation(LightAnimationState& state,const nw::h3d::res::LightContent* light);
void DestroyLightAnimationState(LightAnimationState& state);
bool EvaluateLightAnimation(LightAnimationState& state,const nw::h3d::res::AnimContent* content,AnimationBindingState& binding,float frame);
void TransformAnimatedLight(LightAnimationState& state,const nn::math::MTX34& transform);
}
