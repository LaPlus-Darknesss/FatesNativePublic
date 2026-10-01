#include "fates/graphics/game_effect_object.hpp"
#include "fates/detail/effect_runtime.hpp"
#include "fates/graphics/scene_system.hpp"

GameEffectObject::GameEffectObject(){ handle=fates::decomp_detail::RegisterGameEffectObject(this); }
GameEffectObject::~GameEffectObject(){ fates::decomp_detail::RemoveEffectDefinitionLighting(*this,scene); node_.Detach(); fates::decomp_detail::UnregisterGameEffectObject(this); }
void GameEffectObject::UpdateMatrix(){ node_.SetWorldTranslate(translate); node_.UpdateTransform(); }
bool GameEffectObject::Init(float deltaFrame){
    (void)deltaFrame; fates::decomp_detail::ApplyEffectDefinitionInit(*this); UpdateMatrix(); flags_|=Initialized; return true;
}
bool GameEffectObject::SkipTick(){
    if((flags_&DeletePending)!=0) return true;
    if(fates::decomp_detail::ShouldSkipEffectForBlackout()) return true;
    if(fates::decomp_detail::ShouldSkipEffectForInput()) return true;
    if(!eternal && node_.IsFinished()) return true;
    return false;
}
bool GameEffectObject::Tick(float deltaFrame){
    if(callback!=nullptr) callback->OnUpdate(GameEffect(handle));
    fates::decomp_detail::UpdateEffectAttachment(*this);
    fates::decomp_detail::ApplyEffectDefinitionTick(*this,deltaFrame);
    if(delay>0.0f){ delay-=deltaFrame; return true; }
    elapsed+=deltaFrame; frame+=stepFrame*deltaFrame; node_.SetStepFrame(stepFrame); node_.SetAnimFrame(frame); node_.SetEmitterRatio(emitterRatio);
    fates::decomp_detail::ApplyEffectDefinitionLighting(*this,scene);
    return !SkipTick();
}
bool GameEffectObject::Update(float deltaFrame){
    for(;;){
        if((flags_&DeletePending)!=0) return false;
        switch(phase_){
            case Phase::Initialize: if(!Init(deltaFrame)) return false; phase_=Phase::WaitForResources; break;
            case Phase::WaitForResources:
                if(node_.IsAsyncLoading()) return true;
                (void)node_.Setup(); node_.Attach(scene); flags_|=Started; phase_=Phase::Running; break;
            case Phase::Running:
                if(Tick(deltaFrame)) return true;
                phase_=Phase::Finished; break;
            case Phase::Finished: MarkDelete(); return false;
        }
    }
}
