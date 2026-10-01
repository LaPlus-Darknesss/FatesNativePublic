#pragma once
#include "fates/graphics/icamera.hpp"
#include "fates/graphics/camera_param.hpp"
class ProcInst;
namespace game::graphics {
class CameraBase : public ICamera {
public:
    CameraBase(); ~CameraBase() override; void StartBlend(int); void StartShake(ProcInst*,float,int); void TickTarget(); void UpdateAngle(); void TickDistance(); void SetDestinationF32(float,float); void UpdateTarget(); void CalculateDraw(); void InstantConfig(); void InstantTarget(); void ShakeForCastle(ProcInst*,float,int,int,float); void UpdateDistance(); void SetDestinationInt(int,int); float GetGameConfigAngle(int); void SetGameConfigAll(); void SetGameConfigAngle(); void SetFromCastleCamera(ICamera*); float GetGameConfigDistance(int); void SetGameConfigDistance(); void Tick(); void Shake(ProcInst*,float,int,int,float); void Update(); void Instant(); void StopShake(int,float); void TickAngle(); void TickBlend(); bool IsBlending() const; float GetMoveRatio(int) const; nn::math::VEC3 GetLimitTarget(const nn::math::VEC3&,float) const; bool IsAngleProlixity() const; bool IsScrollProlixity() const; bool IsDistanceProlixity() const; bool IsScroll() const;
private:
    CameraParam current_{}; CameraParam blendFrom_{}; nn::math::VEC3 destination_{}; nn::math::VEC3 currentTarget_{}; float distance_{}; float desiredDistance_{}; float yaw_{}; float pitch_{}; float desiredYaw_{}; float desiredPitch_{}; int targetFrame_{}; int distanceFrame_{}; int yawFrame_{}; int pitchFrame_{}; int blendFrame_{}; int blendFrames_{}; bool fastMove_{};
};
}
