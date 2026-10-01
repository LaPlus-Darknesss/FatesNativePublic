#pragma once
#include "fates/graphics/basic_types.hpp"
#include "fates/battle/battle_runtime.hpp"
class CameraParam; class ICamera;
class CounterScene { public: void PlayCamera(const char*); void BlendCamera(const char*); bool IsPlayingCamera() const; void SetupUnitViewerCamera(); void DisableCameraPlayerCtrl(); };
class CutSceneController { public: void PlayCamera(const char*); void BlendCamera(const char*,int); void CameraSetDepthLevel(float); void CameraSetDepthFactor(float); void SetCameraPlaybackRate(float); void CameraEnableLookAround(bool); };
class ProcLilith { public: void SetDefaultCamera(); };
class ProcJobIntro { public: void WaitCamera(); };
class ProcSpa { public: void ChangeCameraExit(); void Tick_AutoCameraCtrl(bool); void EndUnitViewerCameraSeq(); void Persistent_CameraEvent(); void InitUnitViewerCameraSeq(); void TickUnitViewerCameraSeq(); void SetUnitViewerCameraTarget(int); void TickUnitViewerCameraSeq_Extra(); void SetUnitsTransparencyInUnitViewerCamera(); };
class ProcJail { public: void ChangeMainCamera(); void ChangePreveilCamera(); };
