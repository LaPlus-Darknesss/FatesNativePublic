#include "fates/graphics/model_state.hpp"

ModelState::ModelState(){ fates::decomp_detail::InitializeModelStateDefaults(modelState_); }
void ModelState::UpdateState(){
    auto& s=modelState_;
    s.flags &= static_cast<std::uint16_t>(~0x19u);
    for(unsigned shift=0;shift<16;shift+=4) if((s.stageMask & (0xFu<<shift))!=0) s.flags|=0x11;
    if(s.constantColorIndex!=-1) s.flags|=0x9;
    if(static_cast<unsigned>(s.additiveColor.r)+s.additiveColor.g+s.additiveColor.b!=0) s.flags|=0x9;
    if(s.callback!=nullptr) s.flags|=0x1;
    if((s.flags&0x800)!=0) s.flags|=0x1;
}
void ModelState::BeginCommand(nn::gr::CTR::CommandBufferJumpHelper& helper,const ModelCallback::ModelArgs& args) const { fates::decomp_detail::EmitModelStateBegin(modelState_,helper,args); }
void ModelState::EndCommand(nn::gr::CTR::CommandBufferJumpHelper& helper,const ModelCallback::ModelArgs& args) const { fates::decomp_detail::EmitModelStateEnd(modelState_,helper,args); }
