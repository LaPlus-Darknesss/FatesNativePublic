#include "fates/runtime/native_item_range.hpp"
#include "fates/runtime/native_magic_power.hpp"
#include "fates/runtime/native_runtime.hpp"
#include "fates/runtime/native_current_item_eligibility.hpp"
#include <algorithm>
namespace fates::runtime::native {
std::optional<int> ItemRangeInnerExact(std::uint8_t raw,ItemRangeServices& s) {
    if(raw>1){const auto group=s.WeaponGroup();if(!group)return std::nullopt;
        if(*group==4){const auto skill=s.PointBlankSkill();if(!skill)return std::nullopt;if(*skill)return 1;}}
    return raw;
}
std::optional<int> ItemRangeOuterExact(std::uint8_t raw,ItemRangeServices& s) {
    int value=raw;
    if(raw==254){const auto magic=s.CurrentMagic();if(!magic)return std::nullopt;value=std::max(1,*magic/2);}
    const auto group=s.WeaponGroup();if(!group)return std::nullopt;
    if(*group==6){const auto skill=s.InfiniteStaffSkill();if(!skill)return std::nullopt;if(*skill)value=std::max(10,value);}
    return value;
}
namespace {
using S=UnitCapabilityStatus;
UnitCapabilityResult CurrentRange(const NativeRuntime& r,const UnitState& u,UnitItemState instance,bool outer) {
    if(!u.occupied)return {S::InvalidUnit};
    const auto* item=r.definitions.FindItem(instance.item_id);if(!item)return {S::MissingItemDefinition};
    struct Services:ItemRangeServices {
        const NativeRuntime& r;const UnitState& u;const ItemDefinition& item;S error{S::Ok};
        Services(const NativeRuntime& r,const UnitState& u,const ItemDefinition& item):r(r),u(u),item(item){}
        std::optional<std::uint8_t> WeaponGroup() override {
            const auto* sub=r.definitions.FindItemSubKind(item.weapon_category);if(!sub){error=S::MissingDefinition;return std::nullopt;}return sub->weapon_exp_group;
        }
        std::optional<bool> Skill(const char* name) {
            const auto* skill=r.definitions.FindSkill(name);if(!skill||skill->id>32767){error=S::MissingDefinition;return std::nullopt;}
            const auto value=ProjectCurrentEquippedSkill(r,u,std::int16_t(skill->id));if(!value)error=S::UnresolvedSkill;return value;
        }
        std::optional<bool> PointBlankSkill() override {return Skill("SEID_\x8b\xdf\x90\xda\x8e\xcb\x8c\x82");}
        std::optional<bool> InfiniteStaffSkill() override {return Skill("SEID_\x96\xb3\x8c\xc0\x82\xcc\x8f\xf1");}
        std::optional<int> CurrentMagic() override {
            // Original range query passes nullptr, true, true, type 0. Its
            // selected item is the equipped inventory item, not this range item.
            const auto value=ProjectCurrentMagicPower(r,u);if(value.status!=S::Ok){error=value.status;return std::nullopt;}return value.value;
        }
    } svc(r,u,*item);
    const auto value=outer?ItemRangeOuterExact(std::uint8_t(item->max_range),svc):ItemRangeInnerExact(std::uint8_t(item->min_range),svc);
    return value?UnitCapabilityResult{S::Ok,*value}:UnitCapabilityResult{svc.error};
}
}
UnitCapabilityResult ProjectCurrentItemRangeInner(const NativeRuntime& r,const UnitState& u,UnitItemState item){return CurrentRange(r,u,item,false);}
UnitCapabilityResult ProjectCurrentItemRangeOuter(const NativeRuntime& r,const UnitState& u,UnitItemState item){return CurrentRange(r,u,item,true);}
}
