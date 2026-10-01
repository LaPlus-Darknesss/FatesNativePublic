#pragma once
#include "fates/graphics/camera_state.hpp"
class FixedCamera : public CameraState {
public:
    FixedCamera(); ~FixedCamera() override; void ChangeToFocus(int) override; void Tick() override; void Enter() override; void Leave() override; void UpdateParam(const CameraParam&); void UpdateDirection(); int GetModeType() const; bool CanChangeMode() const; bool CanLookAround() const; bool IsHitPos() const; bool IsHitArea() const; bool IsHitFovy() const;
private: nn::math::VEC3 direction_{}; bool flipped_{}; int focusUnit_{};
};
