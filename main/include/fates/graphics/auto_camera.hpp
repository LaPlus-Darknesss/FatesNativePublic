#pragma once
#include "fates/graphics/camera_state.hpp"
class AutoCamera : public CameraState {
public:
    AutoCamera(); ~AutoCamera() override; void ChangeToFocus(int) override; void ChangeToNormal() override; void ChangeToWinCut(int) override; void ChangeToDeathCut(int) override; void ChangeCut() override; void Tick() override; void Enter() override; void Leave() override;
    int GetModeType() const; bool CanChangeMode() const; bool CanLookAround() const; bool IsHitPos() const; bool IsHitArea() const; bool IsHitFovy() const;
private: nn::math::VEC3 facing_{}; nn::math::VEC3 rotation_{}; bool flipped_{}; int focusUnit_{-1}; int cutKind_{}; bool recalc_{true}; const char* cameraName_{};
};
