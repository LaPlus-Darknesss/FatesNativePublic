#include "fates/graphics/anim_ctrl.hpp"
#include "fates/graphics/cam_anim.hpp"
#include "fates/graphics/light_anim.hpp"
#include "fates/graphics/mat_anim.hpp"
#include "fates/graphics/model.hpp"
#include "fates/graphics/skel_anim.hpp"
#include "fates/graphics/vis_anim.hpp"
#include <algorithm>

namespace { constexpr int kInvalid=0xffff; }
AnimCtrl::AnimCtrl()=default;
AnimCtrl::~AnimCtrl(){ FreeAnim(); }
void AnimCtrl::OnPlayAnim(AnimObj*){}
void AnimCtrl::OnStopAnim(AnimObj*){}

AnimObj* AnimCtrl::AllocAnim(AnimConst::Type type){
    std::unique_ptr<AnimObj> p;
    switch(type){case AnimConst::Type::Skeletal:p=std::make_unique<SkelAnim>();break;case AnimConst::Type::Material:p=std::make_unique<MatAnim>();break;case AnimConst::Type::Visibility:p=std::make_unique<VisAnim>();break;case AnimConst::Type::Camera:p=std::make_unique<CamAnim>();break;case AnimConst::Type::Light:p=std::make_unique<LightAnim>();break;}
    if (!p) {
        return nullptr;
    }
    p->group_ = activeGroup_;
    AnimObj* raw = p.get();
    owned_.push_back(std::move(p));
    groups_[activeGroup_].push_back(raw);
    return raw;
}
AnimObj* AnimCtrl::GetAnimObj(AnimConst::Type type) const { const auto& g=groups_[activeGroup_]; for(auto it=g.rbegin();it!=g.rend();++it) if((*it)->GetType()==type) return *it; return nullptr; }
void AnimCtrl::DeleteAnim(AnimObj* a){
    if (!a) {
        return;
    }
    if (a->Content()) {
        OnStopAnim(a);
        a->Stop();
    }
    auto& g=groups_[a->group_]; g.erase(std::remove(g.begin(),g.end(),a),g.end());
    owned_.erase(std::remove_if(owned_.begin(),owned_.end(),[&](const auto& p){return p.get()==a;}),owned_.end());
}
void AnimCtrl::RandomAnim(){ for(AnimObj* a:groups_[activeGroup_]) if(a->Content()) a->SetFrame(fates::decomp_detail::RandomAnimationFrame(a->GetEndFrame())); }

AnimObj* AnimCtrl::PlayMatAnim(const ResFile& r,int i){ if(i==kInvalid)return nullptr; AnimObj* a=GetAnimObj(AnimConst::Type::Material); if(!a)a=AllocAnim(AnimConst::Type::Material); if(a&&a->Play(r,i))OnPlayAnim(a); return a; }
AnimObj* AnimCtrl::PlayVisAnim(const ResFile& r,int i){ if(i==kInvalid)return nullptr; AnimObj* a=GetAnimObj(AnimConst::Type::Visibility); if(!a)a=AllocAnim(AnimConst::Type::Visibility); if(a&&a->Play(r,i))OnPlayAnim(a); return a; }
AnimObj* AnimCtrl::PlaySkelAnim(const ResFile& r,int i,float blendTime){
    if (i == kInvalid) {
        return nullptr;
    }
    auto* a = AllocAnim(AnimConst::Type::Skeletal);
    if (!a || !a->Play(r, i)) {
        if (a) {
            DeleteAnim(a);
        }
        return nullptr;
    }
    OnPlayAnim(a);
    const float frames=std::max(0.0f,blendTime*16.666666f); bool old=false;
    for(AnimObj* o:groups_[activeGroup_]) if(o!=a&&o->GetType()==AnimConst::Type::Skeletal){ old=true; o->ConfigureBlend(o->blendWeight_,0.0f,frames); }
    a->ConfigureBlend(frames>0.0f&&old?0.0f:1.0f,1.0f,frames); blendMode_=old?1:0; if(frames==0.0f&&old)SweepAnim(); return a;
}

bool AnimCtrl::PlayAll(const ResFile& r,const char* id){
    const int si=id?r.GetSkelAnimIndex(id):(r.GetSkelAnimCount()>0?0:kInvalid); const bool s=PlaySkelAnim(r,si)!=nullptr;
    const int mi=id?r.GetMatAnimIndex(id):(r.GetMatAnimCount()>0?0:kInvalid); const bool m=PlayMatAnim(r,mi)!=nullptr;
    const int vi=id?r.GetVisAnimIndex(id):(r.GetVisAnimCount()>0?0:kInvalid); const bool v=PlayVisAnim(r,vi)!=nullptr;
    const int li=id?r.GetLightIndex(id):(r.GetLightCount()>0?0:kInvalid); AnimObj* l=GetAnimObj(AnimConst::Type::Light); if(li!=kInvalid&&!l)l=AllocAnim(AnimConst::Type::Light); const bool light=l&&li!=kInvalid&&l->Play(r,li); if(light)OnPlayAnim(l);
    return s||m||v||light;
}
bool AnimCtrl::TryPlayAll(const ResFile& r,const char* id){ if(!groups_[activeGroup_].empty())return false; return PlayAll(r,id); }
void AnimCtrl::SetAnimFrame(float f){ for(auto* a:groups_[activeGroup_])a->SetFrame(f); }
void AnimCtrl::SetStepFrame(float f){ for(auto* a:groups_[activeGroup_])a->SetStepFrame(f); }
void AnimCtrl::SetFrameToEnd(){ for(auto* a:groups_[activeGroup_])a->SetFrame(a->GetEndFrame()); }
void AnimCtrl::SetFrameToFinish(){ for(auto* a:groups_[activeGroup_])a->SetFrame(a->GetStepFrame()>=0.0f?a->GetEndFrame():0.0f); }
void AnimCtrl::SetLoop(bool loop){ for(auto* a:groups_[activeGroup_])if(a->Content())fates::decomp_detail::SetAnimationLooping(a->Content(),loop); }

void AnimCtrl::CalcAnim(Model* model,float deltaFrame){
    if (model == nullptr) {
        return;
    }
    if (model_ != model) {
        fates::decomp_detail::DestroyAnimationBlendState(blendState_);
        model_ = model;
        fates::decomp_detail::EnsureAnimationBlendState(blendState_, *model);
    }
    int skelCount=0; for(const auto& p:owned_)if(p->Content()&&p->GetType()==AnimConst::Type::Skeletal)++skelCount; const bool blend=blendMode_==1&&skelCount>1;
    for(auto& p:owned_){ AnimObj& a=*p; if(!a.Content())continue; a.AdvanceBlend(deltaFrame);
        switch(a.GetType()){
        case AnimConst::Type::Skeletal: static_cast<SkelAnim&>(a).Calc(*model,blend?&blendState_:nullptr); break;
        case AnimConst::Type::Material: fates::decomp_detail::ApplyMaterialAnimation(a.Content(),a.Binding(),*model,a.GetFrame()); break;
        case AnimConst::Type::Visibility: fates::decomp_detail::ApplyVisibilityAnimation(a.Content(),a.Binding(),*model,a.GetFrame()); break;
        case AnimConst::Type::Camera: static_cast<CamAnim&>(a).Calc(); break;
        case AnimConst::Type::Light: static_cast<LightAnim&>(a).Calc(); break;
        }
        a.UpdateFrame(deltaFrame);
    }
    if (blend) {
        fates::decomp_detail::NormalizeSkeletalBlend(blendState_, *model);
    }
    SweepAnim();
}
void AnimCtrl::ResetAnim(){ if(model_)fates::decomp_detail::ResetModelAnimationState(*model_); }
void AnimCtrl::StopAll(){ auto copy=groups_[activeGroup_]; for(auto* a:copy)if(a&&a->Content()){OnStopAnim(a);a->Stop();} SweepAnim(); }
void AnimCtrl::SweepAnim(){
    std::vector<AnimObj*> dead; for(const auto& p:owned_)if(!p->Content()||(p->GetType()==AnimConst::Type::Skeletal&&!p->BlendActive()&&p->blendWeight_<=0.0f))dead.push_back(p.get()); for(auto* a:dead)DeleteAnim(a);
}
void AnimCtrl::FreeAnim(){ owned_.clear(); for(auto& g:groups_)g.clear(); fates::decomp_detail::DestroyAnimationBlendState(blendState_); model_=nullptr; }
bool AnimCtrl::IsFinished() const { for(auto* a:groups_[activeGroup_])if(!a->IsFinished())return false;return true; }
float AnimCtrl::GetAnimFrame() const { const auto& g=groups_[activeGroup_];return g.empty()?0.0f:g.back()->GetFrame(); }
float AnimCtrl::GetRemainFrame() const { const auto& g=groups_[activeGroup_];return g.empty()?0.0f:g.back()->GetEndFrame()-g.back()->GetFrame(); }
bool AnimCtrl::IsLoop() const { for(auto* a:groups_[activeGroup_])if(a->IsLoop())return true;return false; }
