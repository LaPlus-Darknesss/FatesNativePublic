#include "fates/graphics/effect_emitter.hpp"
#include "fates/graphics/effect_object.hpp"
#include "fates/graphics/render_state.hpp"

EffectEmitter::EffectEmitter()=default;
EffectEmitter::~EffectEmitter(){ Free(); }
void EffectEmitter::Free(){
    if(fates::decomp_detail::IsEmitterAlive(emitter_)) fates::decomp_detail::KillEmitter(emitter_);
    emitter_={}; resource_=nullptr; file_.Free();
}
bool EffectEmitter::Load(const EffectFile& file,const char* name){
    Free(); file_.ReplaceHandle(file);
    auto* object=static_cast<const EffectObject*>(file_.GetFileData());
    resource_=object;
    return object!=nullptr && fates::decomp_detail::CreateEmitter(emitter_,*object,name);
}
bool EffectEmitter::Load(const EffectFile& file,int index){
    Free(); file_.ReplaceHandle(file);
    auto* object=static_cast<const EffectObject*>(file_.GetFileData());
    resource_=object;
    return object!=nullptr && fates::decomp_detail::CreateEmitter(emitter_,*object,index);
}
void EffectEmitter::Draw(RenderState& state,float frameRemainder){ if(IsAlive()) fates::decomp_detail::DrawEmitter(emitter_,state,frameRemainder); }
bool EffectEmitter::IsAlive() const { return fates::decomp_detail::IsEmitterAlive(emitter_); }
void EffectEmitter::Fade(){ if(IsAlive()) fates::decomp_detail::FadeEmitter(emitter_); }
void EffectEmitter::SetMatrix(const nn::math::MTX34& world){ if(IsAlive()) fates::decomp_detail::SetEmitterMatrix(emitter_,world); }
void EffectEmitter::SetColor(const Color8& color){ if(IsAlive()) fates::decomp_detail::SetEmitterColor(emitter_,color); }
void EffectEmitter::SetRatio(float ratio){ if(IsAlive()) fates::decomp_detail::SetEmitterRatio(emitter_,ratio); }
void EffectEmitter::ForceCalc(int frames){ if(IsAlive() && frames>0) fates::decomp_detail::ForceEmitterCalc(emitter_,frames); }
