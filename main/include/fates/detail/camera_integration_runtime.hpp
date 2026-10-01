#pragma once
#include "fates/graphics/basic_types.hpp"
class CameraParam; class CameraTrack; class ICamera; class ProcInst; class Stream;
class CameraProduct; class CameraRotation; class CameraButtonProc; class CameraButtonGroup;
class BattleUnit; class BattleWorld; class CounterScene; class CutSceneController; class ProcSpa; class ProcJail; class ProcJobIntro; class ProcLilith;
namespace map { class Camera; }
namespace castle { class FocusCamera; class FocusCameraProc; }
namespace cmvm { class CmContext; }
namespace fates::decomp_detail {
void CameraProductOffset(CameraProduct&,const nn::math::VEC3&,float,const nn::math::VEC3&); CameraTrack* CameraProductFind(const CameraProduct&,const char*); void CameraProductDispose(CameraProduct&);
bool CameraRotationTick(CameraRotation&); void CameraRotationChange(CameraRotation&,const char*); void CameraButtonProcPersistent(CameraButtonProc&); void CameraButtonProcTick(CameraButtonProc&); void CameraButtonGroupDraw(CameraButtonGroup&); void CameraButtonGroupTick(CameraButtonGroup&); void CameraButtonsCreate(); void CameraButtonsDelete(); void CameraButtonsSetDisabled(bool); bool CameraButtonsIsDisabled(); void CameraButtonsSetVisible(bool);
void MapCameraInitialize(); map::Camera* MapCameraGet(); void MapCameraFinalize(); void MapCameraDeserialize(map::Camera&,Stream*); void MapCameraSerialize(const map::Camera&,Stream*);
void FocusCameraForceMove(castle::FocusCamera&); void FocusCameraSetTarget(castle::FocusCamera&,int,const nn::math::VEC3&); void FocusCameraSetTargetType(castle::FocusCamera&,int); castle::FocusCamera* FocusCameraGet(); castle::FocusCamera* FocusCameraTryGet(); void FocusCameraCreate(ProcInst*); void FocusCameraDestroy(); void FocusCameraProcPersistent(castle::FocusCameraProc&);
void BattleUnitUpdateCameraVisibility(BattleUnit&); void BattleInstantCamera(); int BattleCameraModeType(); void BattleCalcComeback(CameraParam*); bool BattleIsPlayingCamera(); ICamera* BattleWorldMainCamera(const BattleWorld&);
void CounterPlayCamera(CounterScene&,const char*); void CounterBlendCamera(CounterScene&,const char*); bool CounterIsPlayingCamera(const CounterScene&); void CounterSetupUnitViewer(CounterScene&); void CounterDisablePlayerCamera(CounterScene&);
void CutScenePlayCamera(CutSceneController&,const char*); void CutSceneBlendCamera(CutSceneController&,const char*,int); void CutSceneDepthLevel(CutSceneController&,float); void CutSceneDepthFactor(CutSceneController&,float); void CutScenePlaybackRate(CutSceneController&,float); void CutSceneLookAround(CutSceneController&,bool);
void ProcLilithDefaultCamera(ProcLilith&); void ProcJobIntroWaitCamera(ProcJobIntro&); void ProcSpaCameraExit(ProcSpa&); void ProcSpaAutoCamera(ProcSpa&,bool); void ProcSpaEndViewer(ProcSpa&); void ProcSpaCameraEvent(ProcSpa&); void ProcSpaInitViewer(ProcSpa&); void ProcSpaTickViewer(ProcSpa&); void ProcSpaSetViewerTarget(ProcSpa&,int); void ProcSpaTickViewerExtra(ProcSpa&); void ProcSpaSetViewerTransparency(ProcSpa&); void ProcJailMainCamera(ProcJail&); void ProcJailPreveilCamera(ProcJail&);
void BattleSeqWaitCamera(); void BattleSeqChangeCamera(); void BattleSeqPlayArenaCamera(); void BattleSeqPlayChangeCamera(); void BattleSeqPlayComebackCamera(); void BattleSeqPlaySeamlessCamera();
void SequenceWaitCamera(ProcInst*); void SequenceMoveCamera(ProcInst*,int,int); void SequenceSetCameraFirst(ProcInst*); void SequenceMoveCameraProlixity(ProcInst*,int,int); bool SequenceWaitProlixity(ProcInst*,int); void SequenceSetSkippedCamera(); void SequenceSetCameraForSkip(); void SequenceCameraShot(); void SequenceCameraHitWait(); void SequenceCameraShotWait(); void SequenceMoveCamera();
void CameraVmCommand(cmvm::CmContext*,const char*); void CameraVmCommandI(cmvm::CmContext*,const char*,int); void CameraVmCommandII(cmvm::CmContext*,const char*,int,int); void CameraVmCommandShake(cmvm::CmContext*,const char*,unsigned int,int,int,unsigned int); void CameraVmCommandObject(cmvm::CmContext*,const char*,const char*);
void CameraShakePersistent(); void CameraShakeTick(); void CameraShakeCleanup();
}
