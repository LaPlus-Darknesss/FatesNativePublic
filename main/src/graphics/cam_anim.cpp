#include "fates/graphics/cam_anim.hpp"
CamAnim::CamAnim():AnimObj(AnimConst::Type::Camera){ fates::decomp_detail::ResetCameraAnimationState(cameraState_); }
bool CamAnim::Play(const ResFile& r,int index){
    Stop(); if(index==0xffff) return false;
    cameraState_.camera=r.GetCamera(index); if(cameraState_.camera==nullptr) return false;
    const char* name=fates::decomp_detail::GetCameraResourceName(cameraState_.camera);
    const int ai=r.GetCamAnimIndex(name); if(ai==0xffff) return false;
    return Alloc(r.GetCamAnim(ai),r);
}
void CamAnim::Stop(){ fates::decomp_detail::ResetCameraAnimationState(cameraState_); Free(); }
bool CamAnim::Calc(){ return Content() && cameraState_.camera && fates::decomp_detail::EvaluateCameraAnimation(cameraState_,Content(),Binding(),GetFrame()); }
CamAnim::~CamAnim(){ Stop(); }
