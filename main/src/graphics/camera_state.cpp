#include "fates/graphics/camera_state.hpp"
#include "fates/detail/camera_execution_runtime.hpp"
CameraState::CameraState()=default;
void CameraState::ChangeInput(){} void CameraState::ChangeCamera(int,const char*){} void CameraState::ChangeToFocus(int){} void CameraState::ChangeToNormal(){} void CameraState::ChangeToWinCut(int){} void CameraState::ChangeToDeathCut(int){} void CameraState::ChangeCut(){}
CameraParam* CameraState::GetParam(){ return &param_; } const CameraParam* CameraState::GetParam() const { return &param_; }
void CameraState::HitCheck(CameraParam* p){ if(p) fates::decomp_detail::CameraStateHitCheck(*this,*p); }
