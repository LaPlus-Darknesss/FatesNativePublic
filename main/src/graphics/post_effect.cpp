#include "fates/graphics/post_effect.hpp"
#include "fates/detail/camera_execution_runtime.hpp"
void* PostEffect::buffer_=nullptr;
void PostEffect::Initialize(){ if(!buffer_) buffer_=fates::decomp_detail::AllocatePostEffectBuffer(0x20000,0x80); }
void PostEffect::Finalize(){ if(buffer_){ fates::decomp_detail::FreePostEffectBuffer(buffer_); buffer_=nullptr; } }
void* PostEffect::Buffer(){return buffer_;}
