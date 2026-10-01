#pragma once
#include "fates/runtime/native_unit_capabilities.hpp"
#include <optional>
namespace fates::runtime::native {
struct UnitItemState;
struct HeldCapabilityServices {
    virtual ~HeldCapabilityServices()=default;
    virtual std::optional<bool> IsHeldEnhancement(unsigned slot)=0;
    virtual std::optional<std::int8_t> HeldBonus(unsigned slot,unsigned capability)=0;
};
// GetEnhanceHave visits every slot, including empty Item records, with the
// special-weapon switch false. It sums signed Item+40 lanes, without a clamp.
std::optional<int> HeldCapabilityExact(unsigned capability,HeldCapabilityServices&);
UnitCapabilityResult ProjectCurrentHeldCapability(const NativeRuntime&,const UnitState&,unsigned capability);

enum class MagicSkill : std::uint8_t { PlusTwo,Absorb,Medicine };
struct MagicPowerServices {
    virtual ~MagicPowerServices()=default;
    // explicit_item=false selects current equipped inventory, with known absence
    // contributing zero. Explicit items resolve even when effects are disabled.
    virtual std::optional<int> SelectedItemBonus(bool explicit_item)=0;
    virtual std::optional<int> BaseCapability()=0;
    virtual std::optional<std::uint8_t> EnhanceByte(unsigned index)=0;
    virtual std::optional<int> SelectedPartnerBonus()=0;
    virtual std::optional<bool> HasSkill(MagicSkill)=0;
    virtual std::optional<std::int8_t> AbsorbValue()=0;
    virtual std::optional<int> HeldContribution()=0;
    virtual std::optional<std::int8_t> Weakness()=0;
};
// GetPowImpl's ordered current contributions. BaseCapability uses the shared
// signed base/limit calculation. Effects and weakness are independent switches.
std::optional<int> MagicPowerExact(std::uint32_t flags,bool explicit_item,bool effects,
    bool weakness,int enhance_type,MagicPowerServices&);
UnitCapabilityResult ProjectCurrentMagicPower(const NativeRuntime&,const UnitState&,
    const UnitItemState* explicit_item=nullptr,bool effects=true,bool weakness=true,int enhance_type=0);
}
