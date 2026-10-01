#pragma once
namespace fates::decomp_detail { struct EffectVendorSystem; }
class EffectSystem {
public:
    static void Initialize();
    static void Update();
    static fates::decomp_detail::EffectVendorSystem* GetSystem();
};
