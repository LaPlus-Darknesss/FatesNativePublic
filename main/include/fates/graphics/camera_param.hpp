#pragma once
#include "fates/graphics/basic_types.hpp"
struct CameraParam {
    nn::math::VEC3 eye{}; nn::math::VEC3 at{}; nn::math::VEC3 up{0.0f,1.0f,0.0f};
    float fovyRad{}; float nearClip{}; float farClip{}; float depthLevel{}; float depthFactor{}; float extra{};
    CameraParam(); ~CameraParam();
    void BlendParam(const CameraParam* from,const CameraParam* to,float t);
    void BlendParam(const CameraParam* from,const CameraParam* to,float t,float eyeT,float atT,float scalarT);
    void ResetParam(); void SetFovyDeg(float degrees); float GetFovyDeg() const;
    void RotationPos(const nn::math::VEC3& rotation);
    void SetLookAtTarget(const nn::math::VEC3& target,const nn::math::VEC3& rotation,float distance);
    void SetLookAtPos(const nn::math::VEC3& pos,const nn::math::VEC3& rotation,float distance);
    void CopyParam(const CameraParam* other);
};
