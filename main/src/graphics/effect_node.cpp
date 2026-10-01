#include "fates/graphics/effect_node.hpp"
#include "fates/graphics/render_state.hpp"
#include <algorithm>

EffectNode::EffectNode()=default;
EffectNode::~EffectNode(){ sourcePackage_.Free(); model_.FreeModel(); emitter_.Free(); modelResources_.Free(); emitterResources_.Free(); }
void EffectNode::FadeEmitter(){ emitter_.Fade(); }
void EffectNode::SetAnimFrame(float frame){ animFrame_=frame; dirty_|=FrameDirty; }
void EffectNode::SetStepFrame(float frame){ stepFrame_=frame; dirty_|=StepDirty; }
void EffectNode::SetModelAlpha(unsigned char alpha){ modelColor_.a=alpha; dirty_|=ModelColorDirty; }
void EffectNode::SetModelColor(const Color8& color){ modelColor_=color; dirty_|=ModelColorDirty; }
void EffectNode::SetEmitterAlpha(unsigned char alpha){ emitterColor_.a=alpha; dirty_|=EmitterColorDirty; }
void EffectNode::SetEmitterColor(const Color8& color){ emitterColor_=color; dirty_|=EmitterColorDirty; }
void EffectNode::SetEmitterRatio(float ratio){ emitterRatio_=ratio; dirty_|=RatioDirty; }
void EffectNode::UpdateTransform(){ dirty_|=TransformDirty; }
void EffectNode::SetIdling(int frames){ idlingFrames_=static_cast<std::uint16_t>(std::max(frames,0)); dirty_|=IdlingDirty; }

bool EffectNode::Setup(const char* modelName,const char* emitterName){
    if(!sourcePackage_.TryFinishAsync()) return false;
    if(modelName!=nullptr){
        modelResources_.ReplaceHandle(sourcePackage_);
        (void)model_.LoadModel(modelResources_,modelName);
        (void)anim_.PlayAll(modelResources_,modelName);
    }
    if(emitterName!=nullptr){ emitterResources_.ReplaceHandle(sourcePackage_); (void)emitter_.Load(emitterResources_,emitterName); }
    return true;
}
bool EffectNode::Setup(){
    return Setup(fates::decomp_detail::GetDefaultEffectModelEntry(),fates::decomp_detail::GetDefaultEffectEmitterEntry());
}
void EffectNode::ReadAsync(const char* path){ sourcePackage_.ReadAsync(path); }

void EffectNode::OnUpdate(RenderState& state){
    (void)state;
    float delta=stepFrame_;
    if((dirty_&FrameDirty)!=0){ anim_.SetAnimFrame(animFrame_); }
    if((dirty_&StepDirty)!=0){ anim_.SetStepFrame(stepFrame_); }
    if((dirty_&IdlingDirty)!=0){ delta+=static_cast<float>(idlingFrames_); }
    if((dirty_&ModelColorDirty)!=0){ model_.SetConstantColor(0,modelColor_); }
    if((dirty_&EmitterColorDirty)!=0){ emitter_.SetColor(emitterColor_); }
    if((dirty_&RatioDirty)!=0){ emitter_.SetRatio(emitterRatio_); }
    if((dirty_&TransformDirty)!=0){ emitter_.SetMatrix(model_.GetModelMatrix()); }
    dirty_=0;
    anim_.CalcAnim(&model_,delta);
    model_.Update(nullptr,model_.GetModelMatrix());
    if(emitter_.IsAlive()){
        const int whole=static_cast<int>(frameRemainder_+delta);
        if(whole>0) emitter_.ForceCalc(whole);
        frameRemainder_=(frameRemainder_+delta)-static_cast<float>(whole);
    }
    totalFrame_+=delta;
}
void EffectNode::OnDrawOpa(RenderState& state){ model_.DrawOpa(state,model_.GetModelMatrix()); }
void EffectNode::OnDrawXlu(RenderState& state){ model_.DrawXlu(state,model_.GetModelMatrix()); emitter_.Draw(state,frameRemainder_); }
bool EffectNode::IsFinished() const { return anim_.IsFinished() && !emitter_.IsAlive(); }
bool EffectNode::IsEmitterAlive() const { return emitter_.IsAlive(); }
bool EffectNode::IsAsyncLoading() const { return sourcePackage_.IsAsyncLoading(); }
float EffectNode::GetAnimFrame() const { return anim_.GetAnimFrame(); }
float EffectNode::GetRemainFrame() const { return anim_.GetRemainFrame(); }
