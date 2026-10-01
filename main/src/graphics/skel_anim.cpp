#include "fates/graphics/skel_anim.hpp"
#include "fates/graphics/model.hpp"
bool SkelAnim::Play(const ResFile& r,int index){ return index!=0xffff && Alloc(r.GetSkelAnim(index),r); }
void SkelAnim::Stop(){ Free(); }
void SkelAnim::Calc(Model& model,fates::decomp_detail::AnimationBlendState* blend){
    if(Content()) fates::decomp_detail::ApplySkeletalAnimation(Content(),Binding(),model,GetFrame(),blendWeight_,blend);
}
SkelAnim::~SkelAnim(){ Free(); }
