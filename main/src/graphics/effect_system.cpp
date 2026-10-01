#include "fates/graphics/effect_system.hpp"
#include "fates/detail/effect_runtime.hpp"

void EffectSystem::Initialize(){ (void)fates::decomp_detail::InitializeEffectVendorSystem(); }
void EffectSystem::Update(){ fates::decomp_detail::ResetEffectVendorFrameState(); }
fates::decomp_detail::EffectVendorSystem* EffectSystem::GetSystem(){ return fates::decomp_detail::GetEffectVendorSystem(); }
