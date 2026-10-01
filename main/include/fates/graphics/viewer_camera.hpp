#pragma once
#include <array>
#include "fates/graphics/camera_state.hpp"
class UnitViewerCamera : public CameraState {
public:
    struct ZoomParam { ZoomParam(); float value{}; float target{}; float speed{}; float pad{}; };
    UnitViewerCamera(); ~UnitViewerCamera() override; void Tick() override; void Enter() override; void Leave() override;
    int GetModeType() const; bool CanChangeMode() const; bool CanLookAround() const; nn::math::VEC3 GetTrueTarget() const; bool IsHitPos() const; bool IsHitArea() const; bool IsHitFovy() const;
private: std::array<ZoomParam,3> zoom_{}; nn::math::VEC3 rotation_{}; bool active_{true};
};
class BossWaitingCamera : public CameraState { public: BossWaitingCamera(); ~BossWaitingCamera() override; void ChangeCamera(int,const char*) override; void ChangeToNormal() override; void ChangeToDeathCut(int) override; void Tick() override; void Enter() override; void Leave() override; int GetModeType() const; bool CanChangeMode() const; bool CanLookAround() const; bool IsHitPos() const; bool IsHitArea() const; bool IsHitFovy() const; private: const char* cameraName_{}; };
class JobIntroWaitCamera : public CameraState { public: explicit JobIntroWaitCamera(bool); ~JobIntroWaitCamera() override; void Tick() override; void Enter() override; void Leave() override; int GetModeType() const; bool CanChangeMode() const; bool CanLookAround() const; bool IsHitPos() const; bool IsHitArea() const; bool IsHitFovy() const; private: bool canLookAround_{}; };
