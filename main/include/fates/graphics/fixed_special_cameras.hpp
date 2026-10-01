#pragma once
#include "fates/graphics/fixed_camera.hpp"
class HeadCamera : public FixedCamera { public: HeadCamera(); ~HeadCamera() override; void ChangeToFocus(int) override; void Tick() override; void Enter() override; void Leave() override; int GetModeType() const; bool CanChangeMode() const; bool IsHitPos() const; bool IsHitArea() const; private: int unit_{-1}; nn::math::VEC3 headRotation_{}; };
class SideCamera : public FixedCamera { public: SideCamera(); ~SideCamera() override; void Tick() override; int GetModeType() const; bool CanChangeMode() const; bool IsHitFovy() const; };
class TPSCamera : public FixedCamera { public: TPSCamera(); ~TPSCamera() override; void Tick() override; void Enter() override; void Leave() override; int GetModeType() const; bool CanChangeMode() const; };
