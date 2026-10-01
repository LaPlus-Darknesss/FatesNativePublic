#pragma once
#include "fates/graphics/camera_state.hpp"
#include "fates/graphics/camera_motion.hpp"
class EventCamera : public CameraState {
public:
    explicit EventCamera(const CameraTrack*); ~EventCamera() override; void Tick() override; void Enter() override; void Leave() override; void GetInitialEyeAt(nn::math::VEC3&,nn::math::VEC3&); static void SetFarClipExpandPoint(const nn::math::VEC3&,float); static void ResetFarClipExpandPoint();
    int GetModeType() const; bool CanChangeMode() const; bool CanLookAround() const; bool IsHitPos() const; bool IsHitArea() const; bool IsHitFovy() const; bool IsPlaying() const;
private: const CameraTrack* track_{}; float frame_{}; float endFrame_{}; float step_{1.0f}; bool loop_{}; bool canLookAround_{}; bool hitPos_{};
};
