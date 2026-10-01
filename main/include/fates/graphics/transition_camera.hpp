#pragma once
#include "fates/graphics/camera_state.hpp"
#include "fates/graphics/camera_motion.hpp"
#include "fates/graphics/event_camera.hpp"
class TransitionCamera : public CameraState {
public:
    explicit TransitionCamera(float duration=1.0f); ~TransitionCamera() override;
    void CreateZoomInCurve(); void CreateEncountCurve(); void CreateCommeBackCurve(); void Tick() override;
    int GetModeType() const; bool CanChangeMode() const; bool CanLookAround() const; bool IsChangeBattle() const; float GetTime() const; bool IsHitPos() const; bool IsHitArea() const; bool IsHitFovy() const;
protected: CameraParam from_{}; CameraParam to_{}; float frame_{}; float duration_{1.0f}; bool changedBattle_{};
};
class ZoomInCamera : public TransitionCamera { public: ZoomInCamera(); ~ZoomInCamera() override; void Tick() override; void Enter() override; void Leave() override; };
class SubjectCamera : public TransitionCamera { public: SubjectCamera(const nn::math::VEC3&,const nn::math::VEC3&); ~SubjectCamera() override; void UpdateParam(); void Tick() override; void Enter() override; void Leave() override; bool IsHitPos() const; bool IsHitArea() const; private: nn::math::VEC3 subject_{}; nn::math::VEC3 rotation_{}; CameraInput input_{}; };
class WaitingCamera : public TransitionCamera { public: WaitingCamera(); ~WaitingCamera() override; void ChangeCamera(int,const char*) override; void Tick() override; void Enter() override; void Leave() override; bool CanLookAround() const; private: int requestedType_{}; const char* requestedName_{}; };
class ComebackCamera : public TransitionCamera { public: ComebackCamera(); ~ComebackCamera() override; void Tick() override; void Enter() override; void Leave() override; bool IsHitPos() const; bool IsHitArea() const; };
class EncounterCamera : public TransitionCamera { public: EncounterCamera(); ~EncounterCamera() override; void Tick() override; void Enter() override; void Leave() override; };
class EventBlendCamera : public EventCamera { public: EventBlendCamera(const CameraTrack*,int); ~EventBlendCamera() override; void Tick() override; private: CameraParam blendFrom_{}; int blendFrames_{}; };
class BossEncounterCamera : public TransitionCamera { public: BossEncounterCamera(); ~BossEncounterCamera() override; void Tick() override; void Enter() override; void Leave() override; };
class JobIntroStartCamera : public TransitionCamera { public: explicit JobIntroStartCamera(bool); ~JobIntroStartCamera() override; void Tick() override; void Enter() override; void Leave() override; private: bool variant_{}; };
class SilentComebackCamera : public ComebackCamera { public: ~SilentComebackCamera() override; void Enter() override; };
