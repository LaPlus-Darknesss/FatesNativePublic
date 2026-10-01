#pragma once
#include "fates/graphics/basic_types.hpp"
class CameraParam; class CameraTrack; class TransitionCamera; class ZoomInCamera; class SubjectCamera; class WaitingCamera; class ComebackCamera; class EncounterCamera; class EventBlendCamera; class BossEncounterCamera; class JobIntroStartCamera; class SilentComebackCamera; class HeadCamera; class SideCamera; class TPSCamera; class UnitViewerCamera; class BossWaitingCamera; class JobIntroWaitCamera; class ShadowCamera; class StarginCamera;
namespace fates::decomp_detail {
void TransitionInitialize(TransitionCamera&,float); void TransitionDispose(TransitionCamera&); void TransitionCreateZoomCurve(TransitionCamera&); void TransitionCreateEncounterCurve(TransitionCamera&); void TransitionCreateComebackCurve(TransitionCamera&); void TransitionTick(TransitionCamera&); float TransitionTime(const TransitionCamera&); bool TransitionChangedBattle(const TransitionCamera&);
void ZoomInInitialize(ZoomInCamera&); void ZoomInTick(ZoomInCamera&);
void SubjectInitialize(SubjectCamera&,const nn::math::VEC3&,const nn::math::VEC3&); void SubjectUpdateParam(SubjectCamera&); void SubjectTick(SubjectCamera&); void SubjectEnter(SubjectCamera&); void SubjectLeave(SubjectCamera&);
void WaitingInitialize(WaitingCamera&); void WaitingChangeCamera(WaitingCamera&,int,const char*); void WaitingTick(WaitingCamera&); void WaitingEnter(WaitingCamera&); void WaitingLeave(WaitingCamera&);
void ComebackInitialize(ComebackCamera&); void ComebackTick(ComebackCamera&); void ComebackEnter(ComebackCamera&); void ComebackLeave(ComebackCamera&);
void EncounterInitialize(EncounterCamera&); void EncounterTick(EncounterCamera&);
void EventBlendInitialize(EventBlendCamera&,const CameraTrack*,int); void EventBlendTick(EventBlendCamera&);
void BossEncounterInitialize(BossEncounterCamera&); void BossEncounterTick(BossEncounterCamera&);
void JobIntroStartInitialize(JobIntroStartCamera&,bool); void JobIntroStartTick(JobIntroStartCamera&); void JobIntroStartEnter(JobIntroStartCamera&); void JobIntroStartLeave(JobIntroStartCamera&);
void SilentComebackEnter(SilentComebackCamera&);
void HeadInitialize(HeadCamera&); void HeadChangeFocus(HeadCamera&,int); void HeadTick(HeadCamera&); void HeadEnter(HeadCamera&); void HeadLeave(HeadCamera&);
void SideTick(SideCamera&); void TPSTick(TPSCamera&);
void UnitViewerInitialize(UnitViewerCamera&); void UnitViewerTick(UnitViewerCamera&); void UnitViewerEnter(UnitViewerCamera&); nn::math::VEC3 UnitViewerTrueTarget(const UnitViewerCamera&);
void BossWaitingInitialize(BossWaitingCamera&); void BossWaitingChangeCamera(BossWaitingCamera&,int,const char*); void BossWaitingDeathCut(BossWaitingCamera&,int); void BossWaitingTick(BossWaitingCamera&); void BossWaitingEnter(BossWaitingCamera&); void BossWaitingLeave(BossWaitingCamera&);
void JobIntroWaitInitialize(JobIntroWaitCamera&,bool); void JobIntroWaitTick(JobIntroWaitCamera&); void JobIntroWaitEnter(JobIntroWaitCamera&);
void ShadowCameraDispose(ShadowCamera&); void StarginChangeCut(StarginCamera&);
}
