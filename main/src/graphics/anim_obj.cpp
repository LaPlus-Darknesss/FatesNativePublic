#include "fates/graphics/anim_obj.hpp"
#include <algorithm>
#include <cmath>

AnimObj::AnimObj(AnimConst::Type type):type_(type){}
AnimObj::~AnimObj(){ Free(); }
void AnimObj::Stop(){ Free(); }

bool AnimObj::Free(){
    const bool had=content_!=nullptr;
    if(had){ fates::decomp_detail::DestroyAnimationBinding(binding_); resources_.Free(); }
    content_=nullptr; flags_=0; frame_=0.0f; previousFrame_=0.0f; stepFrame_=1.0f;
    blendWeight_=blendStart_=blendTarget_=1.0f; blendFramesRemaining_=blendFramesTotal_=0.0f;
    return had;
}

bool AnimObj::Alloc(const nw::h3d::res::AnimContent* content,const ResFile& resources){
    Free();
    if(content==nullptr) return false;
    content_=content;
    resources_.ReplaceHandle(resources);
    if(!fates::decomp_detail::InitializeAnimationBinding(binding_,content_)){ content_=nullptr; resources_.Free(); return false; }
    frame_=0.0f; previousFrame_=-1.0f; stepFrame_=1.0f; flags_=Fresh;
    blendWeight_=blendStart_=blendTarget_=1.0f; blendFramesRemaining_=blendFramesTotal_=0.0f;
    return true;
}

float AnimObj::GetEndFrame() const { return content_ ? fates::decomp_detail::GetAnimationEndFrame(content_) : 0.0f; }
bool AnimObj::IsFinished() const { return (flags_&Finished)!=0; }
bool AnimObj::IsLoop() const { return content_ && fates::decomp_detail::IsAnimationLooping(content_); }
bool AnimObj::IsRewound() const { return (flags_&Rewound)!=0; }

void AnimObj::SetFrame(float frame){
    if(frame<frame_) flags_|=Rewound;
    frame_=frame;
    previousFrame_=frame_-stepFrame_;
}

void AnimObj::UpdateFrame(float deltaFrame){
    if(content_==nullptr) return;
    flags_&=static_cast<std::uint16_t>(~(Wrapped|Finished));
    const float delta=stepFrame_*deltaFrame;
    const float old=frame_;
    const float end=GetEndFrame();
    float next=0.0f;
    if(end!=0.0f){
        const float candidate=old+delta;
        if(IsLoop()){
            next=std::fmod(candidate,end);
            if(next<0.0f) next+=end;
        }else{
            next=std::clamp(candidate,0.0f,end);
            if((delta>=0.0f && next>=end) || (delta<0.0f && next<=0.0f)) flags_|=Finished;
        }
    }else flags_|=Finished;
    if((flags_&Fresh)!=0){ flags_&=static_cast<std::uint16_t>(~Fresh); flags_|=Wrapped; }
    if(next<old) flags_|=Wrapped;
    previousFrame_=old;
    frame_=next;
}

void AnimObj::ConfigureBlend(float start,float target,float frames){
    blendWeight_=blendStart_=start; blendTarget_=target; blendFramesTotal_=blendFramesRemaining_=std::max(frames,0.0f);
    if(blendFramesRemaining_==0.0f) blendWeight_=blendTarget_;
}
void AnimObj::AdvanceBlend(float deltaFrame){
    if(blendFramesRemaining_<=0.0f) return;
    blendFramesRemaining_=std::max(0.0f,blendFramesRemaining_-std::abs(deltaFrame));
    const float t=blendFramesTotal_<=0.0f?1.0f:1.0f-(blendFramesRemaining_/blendFramesTotal_);
    blendWeight_=blendStart_+(blendTarget_-blendStart_)*std::clamp(t,0.0f,1.0f);
}
