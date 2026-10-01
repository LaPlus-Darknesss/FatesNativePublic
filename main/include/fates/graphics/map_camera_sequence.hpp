#pragma once
class ProcInst;
namespace map::SequenceHelper {
void WaitCamera(ProcInst*); void MoveCameraInt(ProcInst*,int,int); void SetCameraFirst(ProcInst*); void MoveCameraProlixity(ProcInst*,int,int); void WaitCameraProlixity(ProcInst*); void WaitCameraAngleProlixity(ProcInst*); void WaitCameraDistanceProlixity(ProcInst*);
namespace anonymous_namespace {
class ProcWaitCamera { public: bool Tick(); ~ProcWaitCamera()=default; };
class ProcWaitCameraProlixity { public: bool Tick(); ~ProcWaitCameraProlixity()=default; };
class ProcWaitCameraAngleProlixity { public: bool Tick(); ~ProcWaitCameraAngleProlixity()=default; };
class ProcWaitCameraDistanceProlixity { public: bool Tick(); ~ProcWaitCameraDistanceProlixity()=default; };
}
}
namespace map::sequence_camera { void SetSkippedCamera(); void SetCameraForSkip(); void CameraShot(); void CameraHitWait(); void CameraShotWait(); void MoveCamera(); }
