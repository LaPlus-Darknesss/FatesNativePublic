#pragma once
#include "fates/runtime/native_unit_capabilities.hpp"
#include <optional>
namespace fates::runtime::native {
struct UnitItemState;
struct ItemRangeServices {
    virtual ~ItemRangeServices()=default;
    virtual std::optional<std::uint8_t> WeaponGroup()=0;
    virtual std::optional<bool> PointBlankSkill()=0;
    virtual std::optional<bool> InfiniteStaffSkill()=0;
    virtual std::optional<int> CurrentMagic()=0;
};
// Range bytes are unsigned here, even though Paragon's stored fields are i8.
// 254 outer range means current magic/2 (minimum 1); 255 stays 255.
std::optional<int> ItemRangeInnerExact(std::uint8_t raw,ItemRangeServices&);
std::optional<int> ItemRangeOuterExact(std::uint8_t raw,ItemRangeServices&);
UnitCapabilityResult ProjectCurrentItemRangeInner(const NativeRuntime&,const UnitState&,UnitItemState);
UnitCapabilityResult ProjectCurrentItemRangeOuter(const NativeRuntime&,const UnitState&,UnitItemState);
}
