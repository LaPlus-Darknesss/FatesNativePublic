#include "fates/runtime/native_magic_power.hpp"
#include "fates/runtime/native_runtime.hpp"
#include "fates/runtime/native_item_inventory.hpp"
#include "fates/runtime/native_movement_rules.hpp"
#include "fates/runtime/native_current_item_eligibility.hpp"
#include "fates/support/native_pair_bonus.hpp"
#include "fates/unit/native_unit_semantics.hpp"
namespace fates::runtime::native {
namespace {
using S=UnitCapabilityStatus;
S Inventory(const UnitState& u) {
    switch(ValidateCurrentInventory(u)) {
    case InventoryStatus::Ok:return S::Ok;
    case InventoryStatus::Unbound:return S::UnboundInventory;
    case InventoryStatus::InvalidUnit:return S::InvalidUnit;
    default:return S::StaleInventory;
    }
}
const char* SkillName(MagicSkill skill) {
    switch(skill) {
    case MagicSkill::PlusTwo:return "SEID_\x96\x82\x97\xcd+2";
    case MagicSkill::Absorb:return "SEID_\x96\x82\x97\xcd\x82\xcc\x8b\x7a\x8e\xfb";
    default:return "SEID_\x82\xe6\x82\xad\x8c\xf8\x82\xad\x96\xf2";
    }
}
}
std::optional<int> HeldCapabilityExact(unsigned cap,HeldCapabilityServices& s) {
    if(cap>=8)return std::nullopt;
    int value=0;
    for(unsigned i=0;i<5;++i) {
        const auto eligible=s.IsHeldEnhancement(i);if(!eligible)return std::nullopt;
        if(*eligible){const auto bonus=s.HeldBonus(i,cap);if(!bonus)return std::nullopt;value+=*bonus;}
    }
    return value;
}
UnitCapabilityResult ProjectCurrentHeldCapability(const NativeRuntime& r,const UnitState& u,unsigned cap) {
    if(cap>=8)return {S::InvalidCapability};
    const auto status=Inventory(u);if(status!=S::Ok)return {status};
    struct Services:HeldCapabilityServices {
        const NativeRuntime& r;const UnitState& u;S error{S::Ok};
        Services(const NativeRuntime& r,const UnitState& u):r(r),u(u){}
        const ItemDefinition* Item(unsigned slot) {
            const auto* item=r.definitions.FindItem(u.inventory.items[slot].item_id);
            if(!item)error=S::MissingItemDefinition;return item;
        }
        std::optional<bool> IsHeldEnhancement(unsigned slot) override {
            const auto* item=Item(slot);if(!item)return std::nullopt;
            const auto eligible=ProjectCurrentHeldEnhancement(r,u,*item,false);
            if(!eligible)error=S::UnresolvedHeldEnhancement;return eligible;
        }
        std::optional<std::int8_t> HeldBonus(unsigned slot,unsigned cap) override {
            const auto* item=Item(slot);return item?std::optional<std::int8_t>(std::int8_t(item->extra_data[cap])):std::nullopt;
        }
    } svc(r,u);
    const auto value=HeldCapabilityExact(cap,svc);return value?UnitCapabilityResult{S::Ok,*value}:UnitCapabilityResult{svc.error};
}
std::optional<int> MagicPowerExact(std::uint32_t flags,bool explicit_item,bool effects,bool weakness,int type,MagicPowerServices& s) {
    int item_bonus=0;
    if(explicit_item||effects){const auto item=s.SelectedItemBonus(explicit_item);if(!item)return std::nullopt;item_bonus=*item;}
    const auto base=s.BaseCapability();if(!base)return std::nullopt;int value=*base;
    if(effects) {
        if(weakness){const auto b=s.EnhanceByte(0);if(!b)return std::nullopt;if(*b&0x80)value-=value/2;}
        if(flags&((flags&0x200000)?4u:2u)){const auto pair=s.SelectedPartnerBonus();if(!pair)return std::nullopt;value+=*pair;}
        const auto plus=s.HasSkill(MagicSkill::PlusTwo);if(!plus)return std::nullopt;if(*plus)value+=2;
        const auto absorb=s.HasSkill(MagicSkill::Absorb);if(!absorb)return std::nullopt;
        if(*absorb){const auto bonus=s.AbsorbValue();if(!bonus)return std::nullopt;value+=*bonus;}
        auto effect=[&](int match,unsigned byte,std::uint8_t mask)->std::optional<bool>{
            if(type==match)return true;const auto b=s.EnhanceByte(byte);if(!b)return std::nullopt;return (*b&mask)!=0;
        };
        const auto first=effect(24,3,1);if(!first)return std::nullopt;if(*first)value+=4;
        const auto shared=effect(31,3,0x80);if(!shared)return std::nullopt;if(*shared)value+=2;
        const auto medicine=effect(10,1,4);if(!medicine)return std::nullopt;
        if(*medicine){value+=2;const auto skill=s.HasSkill(MagicSkill::Medicine);if(!skill)return std::nullopt;if(*skill)++value;}
        const auto second=effect(17,2,2);if(!second)return std::nullopt;if(*second)value+=4;
        const auto final=s.EnhanceByte(4);if(!final)return std::nullopt;
        if(*final&0x40)value+=2;if(*final&0x80)--value;
        value+=item_bonus;
        const auto held=s.HeldContribution();if(!held)return std::nullopt;value+=*held;
        if(weakness){const auto penalty=s.Weakness();if(!penalty)return std::nullopt;value-=*penalty;}
    }
    return fates::unit::native::ClampFinalCapability(value);
}
UnitCapabilityResult ProjectCurrentMagicPower(const NativeRuntime& r,const UnitState& u,
    const UnitItemState* explicit_item,bool effects,bool weakness,int type) {
    if(!u.occupied)return {S::InvalidUnit};
    struct Services:MagicPowerServices {
        const NativeRuntime& r;const UnitState& u;const UnitItemState* item;S error{S::Ok};
        Services(const NativeRuntime& r,const UnitState& u,const UnitItemState* item):r(r),u(u),item(item){}
        std::optional<int> Result(UnitCapabilityResult v){if(v.status!=S::Ok){error=v.status;return std::nullopt;}return v.value;}
        std::optional<int> SelectedItemBonus(bool explicit_item) override {
            const UnitItemState* selected=item;
            if(!explicit_item) {
                error=Inventory(u);if(error!=S::Ok)return std::nullopt;
                const auto i=EquippedInventoryIndexExact(u.inventory.items);if(i<0)return 0;selected=&u.inventory.items[std::size_t(i)];
            }
            const auto* d=selected?r.definitions.FindItem(selected->item_id):nullptr;
            if(!d){error=S::MissingItemDefinition;return std::nullopt;}return std::int8_t(d->bonuses[2]);
        }
        std::optional<int> BaseCapability() override {return Result(ProjectCurrentBaseCapability(r,u,2));}
        std::optional<std::uint8_t> EnhanceByte(unsigned i) override {
            if(!u.enhance.bound){error=S::UnboundEnhance;return std::nullopt;}return u.enhance.flags[i];
        }
        std::optional<int> SelectedPartnerBonus() override {
            const auto source=u.pair.partner_slot;if(source==0xffff)return 0;
            if(source>=r.game.units.size()){error=S::MalformedPair;return std::nullopt;}
            const auto& partner=r.game.units[source];const auto receiver=partner.pair.partner_slot;
            if(!u.pair.bound||!partner.occupied||!partner.pair.bound||&partner==&u||receiver>=r.game.units.size()||&r.game.units[receiver]!=&u){error=S::MalformedPair;return std::nullopt;}
            const auto bonus=fates::support::native::ProjectCurrentGuardBonusesFromSource(r,receiver,source);
            if(bonus.status!=fates::support::native::PairBonusStatus::Ok){error=S::UnresolvedPairBonus;return std::nullopt;}return bonus.total[2];
        }
        std::optional<bool> HasSkill(MagicSkill which) override {
            const auto* skill=r.definitions.FindSkill(SkillName(which));
            if(!skill||skill->id>32767){error=S::MissingDefinition;return std::nullopt;}
            const auto value=ProjectCurrentEquippedSkill(r,u,std::int16_t(skill->id));if(!value)error=S::UnresolvedSkill;return value;
        }
        std::optional<std::int8_t> AbsorbValue() override {
            if(!u.enhance.bound){error=S::UnboundEnhance;return std::nullopt;}return std::int8_t(u.enhance.values[2]);
        }
        std::optional<int> HeldContribution() override {return Result(ProjectCurrentHeldCapability(r,u,2));}
        std::optional<std::int8_t> Weakness() override {
            if(!u.enhance.bound){error=S::UnboundEnhance;return std::nullopt;}return std::int8_t(u.weakness[2]);
        }
    } svc(r,u,explicit_item);
    const auto value=MagicPowerExact(u.flags,explicit_item!=nullptr,effects,weakness,type,svc);
    return value?UnitCapabilityResult{S::Ok,*value}:UnitCapabilityResult{svc.error};
}
}
