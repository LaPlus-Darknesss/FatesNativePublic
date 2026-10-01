#include "fates/graphics/light_anim.hpp"
LightAnim::LightAnim():AnimObj(AnimConst::Type::Light){}
bool LightAnim::Play(const ResFile& r,int index){
    Stop(); if(index==0xffff) return false;
    const auto* light=r.GetLight(index); if(light==nullptr || !fates::decomp_detail::CloneLightForAnimation(lightState_,light)) return false;
    const char* name=fates::decomp_detail::GetLightResourceName(light);
    const int ai=r.GetLightAnimIndex(name); if(ai==0xffff){ fates::decomp_detail::DestroyLightAnimationState(lightState_); return false; }
    return Alloc(r.GetLightAnim(ai),r);
}
void LightAnim::Stop(){ fates::decomp_detail::DestroyLightAnimationState(lightState_); Free(); }
bool LightAnim::Calc(){ return Content() && lightState_.mutableLight && fates::decomp_detail::EvaluateLightAnimation(lightState_,Content(),Binding(),GetFrame()); }
void LightAnim::Transform(const nn::math::MTX34& m){ if(lightState_.mutableLight) fates::decomp_detail::TransformAnimatedLight(lightState_,m); }
LightAnim::~LightAnim(){ Stop(); }
