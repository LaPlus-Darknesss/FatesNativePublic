#include "fates/graphics/effect_object.hpp"

EffectObject::EffectObject()=default;
void EffectObject::Setup(){
    const int slot=fates::decomp_detail::AcquireEffectResourceSlot();
    resource_.slot=slot;
    resource_.data=GetData();
    fates::decomp_detail::RegisterEffectResource(resource_,GetData(),slot);
}
void EffectObject::Cleanup(){
    if(resource_.slot<0) return;
    fates::decomp_detail::UnregisterEffectResource(resource_);
    fates::decomp_detail::ReleaseEffectResourceSlot(resource_.slot);
    resource_={};
}
IAllocator* EffectObject::GetAllocator() const { return fates::decomp_detail::GetEffectObjectAllocator(); }
bool EffectObject::IsDelay() const { return false; }
unsigned int EffectObject::GetAlign() const { return 0x80; }
