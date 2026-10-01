#include "fates/map/native_deployment_ranges.hpp"
#include "fates/map/native_deployment_semantics.hpp"
#include "fates/runtime/native_runtime.hpp"
#include "fates/runtime/native_current_item_eligibility.hpp"
#include "fates/runtime/native_item_inventory.hpp"
#include "fates/runtime/native_item_range.hpp"
#include "fates/battle/native_battle_conditions.hpp"
namespace fates::map::native {
std::optional<bool> CannonItemAvailableExact(bool silence,DeploymentRangeServices& s) {
    for(unsigned i=0;i<5;++i){
        const auto equip=s.CanEquip(0,i,false);if(!equip)return std::nullopt;if(!*equip)continue;
        if(silence){const auto silent=s.Silenced(0);if(!silent)return std::nullopt;
            if(*silent){const auto magic=s.IsMagic(0,i);if(!magic)return std::nullopt;if(*magic)continue;}}
        return true;
    }return false;
}
std::optional<DeploymentRangeMasks> BuildDeploymentRangesExact(std::uint32_t member,std::uint32_t args,
    const fates::runtime::native::ItemRangeRuleDefinitions& rules,DeploymentRangeServices& s) {
    DeploymentRangeMasks out{};DistanceMask64 attack{},rod{};
    for(unsigned who=0;who<2;++who){
        if(who){out.without_partner_attack=attack.bits;out.without_partner_rod=rod.bits;
            if(!(args&0x8000))break;const auto partner=s.PartnerExists();if(!partner)return std::nullopt;if(!*partner)break;}
        for(unsigned i=0;i<5;++i){
            const auto equip=s.CanEquip(who,i,true);if(!equip)return std::nullopt;if(!*equip)continue;
            if(member&0x100000){const auto index=s.EquippedIndex(who);if(!index)return std::nullopt;if(*index!=int(i))continue;}
            const auto bits=s.ItemFlags(who,i);if(!bits)return std::nullopt;
            if((member&0x200000)&&!(*bits&rules.basic_staff))continue;
            if(!(args&0x400000)){const auto silent=s.Silenced(who);if(!silent)return std::nullopt;
                if(*silent){const auto magic=s.IsMagic(who,i);if(!magic)return std::nullopt;if(*magic)continue;}}
            const auto inner=s.Inner(who,i);if(!inner)return std::nullopt;
            const auto outer=s.Outer(who,i);if(!outer)return std::nullopt;
            // Current range getters return unsigned bytes or bounded magic/2.
            if(*inner<0||*inner>255||*outer<0||*outer>255)return std::nullopt;
            const auto lo=(member&0x800)?1:*inner,hi=(member&0x800)?1:*outer;
            const auto group=s.Group(who,i);if(!group)return std::nullopt;
            if(*group==6){if((member&0x20000)||((member&0x40000)&&(*bits&rules.interference_staff))||
                ((member&0x80000)&&(*bits&rules.recovery_staff)))MergeRetailItemRange(rod,unsigned(lo),unsigned(hi));}
            else if(member&0x10000)MergeRetailItemRange(attack,unsigned(lo),unsigned(hi));
        }
    }
    if(args&0x2000000){
        const auto category=s.CannonCategory();if(!category)return std::nullopt;
        if(*category){const auto item=CannonItemAvailableExact(!(args&0x400000),s);if(!item)return std::nullopt;
            if(*item){
                const auto area=s.CannonModificationSkill();if(!area)return std::nullopt;
                const auto inner=s.CannonModificationSkill();if(!inner)return std::nullopt;
                const auto outer=s.CannonModificationSkill();if(!outer)return std::nullopt;
                const auto range=ResolveCannonRange(*inner?2:3,*outer?3:4,*area?2:1);
                MergeRetailItemRange(attack,unsigned(range.inner),unsigned(range.outer));
            }}
    }
    out.attack=attack.bits;out.rod=rod.bits;out.attack_max=attack.maxOuter;out.rod_max=rod.maxOuter;return out;
}
namespace {
namespace rn=fates::runtime::native;using S=DeploymentRangeStatus;
    struct Services:DeploymentRangeServices {
        const rn::NativeRuntime& r;const rn::UnitState& primary;CurrentDeploymentRangeResult& result;
        Services(const rn::NativeRuntime& r,const rn::UnitState& u,CurrentDeploymentRangeResult& out):r(r),primary(u),result(out){}
        const rn::UnitState* Unit(unsigned who) {
            if(!who)return &primary;
            const auto s=primary.pair.partner_slot;
            if(s>=r.game.units.size()){result.status=S::MalformedPair;return nullptr;}
            const auto& p=r.game.units[s];
            if(!primary.pair.bound||!p.occupied||!p.pair.bound||&p==&primary||
                p.pair.partner_slot>=r.game.units.size()||&r.game.units[p.pair.partner_slot]!=&primary){result.status=S::MalformedPair;return nullptr;}
            return &p;
        }
        const rn::UnitState* Inventory(unsigned who) {
            const auto* u=Unit(who);if(!u)return nullptr;
            const auto status=rn::ValidateCurrentInventory(*u);if(status==rn::InventoryStatus::Ok)return u;
            result.status=status==rn::InventoryStatus::Unbound?S::UnboundInventory:S::StaleInventory;return nullptr;
        }
        const rn::ItemDefinition* Item(unsigned who,unsigned index) {
            const auto* u=Inventory(who);if(!u)return nullptr;
            const auto* d=r.definitions.FindItem(u->inventory.items[index].item_id);
            if(!d)result.status=S::MissingDefinition;return d;
        }
        std::optional<bool> CanEquip(unsigned who,unsigned i,bool staff) override {
            const auto* item=Item(who,i);if(!item)return std::nullopt;
            const auto value=rn::ProjectCurrentItemEligibility(r,*Unit(who),*item,staff,true);
            if(value==rn::CurrentItemEligibility::No)return false;if(value==rn::CurrentItemEligibility::Yes)return true;
            result.status=S::UnresolvedEligibility;return std::nullopt;
        }
        std::optional<int> EquippedIndex(unsigned who) override {
            const auto* u=Inventory(who);return u?std::optional<int>(rn::EquippedInventoryIndexExact(u->inventory.items)):std::nullopt;
        }
        std::optional<std::uint64_t> ItemFlags(unsigned who,unsigned i) override {
            const auto* item=Item(who,i);if(!item)return std::nullopt;
            std::uint64_t bits=0;for(unsigned b=0;b<8;++b)bits|=std::uint64_t(item->bitflags[b])<<(8*b);return bits;
        }
        std::optional<bool> Silenced(unsigned who) override {
            const auto* u=Unit(who);if(!u)return std::nullopt;
            if(!u->enhance.bound){result.status=S::UnboundEnhance;return std::nullopt;}return (u->enhance.flags[0]&8)!=0;
        }
        std::optional<bool> IsMagic(unsigned who,unsigned i) override {
            const auto* item=Item(who,i);if(!item)return std::nullopt;
            if(!r.definitions.FindItemSubKind(item->weapon_category)){result.status=S::MissingDefinition;return std::nullopt;}
            return r.definitions.IsMagicItem(*item);
        }
        std::optional<int> Range(unsigned who,unsigned i,bool outer) {
            const auto* u=Inventory(who);if(!u)return std::nullopt;
            const auto value=outer?rn::ProjectCurrentItemRangeOuter(r,*u,u->inventory.items[i]):rn::ProjectCurrentItemRangeInner(r,*u,u->inventory.items[i]);
            result.item_range_status=value.status;
            if(value.status!=rn::UnitCapabilityStatus::Ok){result.status=S::UnresolvedItemRange;return std::nullopt;}return value.value;
        }
        std::optional<int> Inner(unsigned who,unsigned i) override {return Range(who,i,false);}
        std::optional<int> Outer(unsigned who,unsigned i) override {return Range(who,i,true);}
        std::optional<std::uint8_t> Group(unsigned who,unsigned i) override {
            const auto* item=Item(who,i);if(!item)return std::nullopt;
            const auto* sub=r.definitions.FindItemSubKind(item->weapon_category);if(!sub){result.status=S::MissingDefinition;return std::nullopt;}return sub->weapon_exp_group;
        }
        std::optional<bool> PartnerExists() override {
            if(primary.pair.partner_slot==0xffff)return false;
            if(!Unit(1))return std::nullopt;return true;
        }
        std::optional<bool> CannonCategory() override {
            const auto& movement=r.definitions.movement_rules();if(!movement){result.status=S::MissingDefinition;return std::nullopt;}
            const auto category=fates::battle::native::ProjectUnitBattleCategory(r,primary);
            if(!category){result.status=S::UnresolvedCategory;return std::nullopt;}return (*category&movement->shooter_category)!=0;
        }
        std::optional<bool> CannonModificationSkill() override {
            const auto* skill=r.definitions.FindSkill("SEID_\x96\x43\x90\x67\x89\xfc\x91\xa2");
            if(!skill||skill->id>32767){result.status=S::MissingDefinition;return std::nullopt;}
            const auto value=rn::ProjectCurrentEquippedSkill(r,primary,std::int16_t(skill->id));
            if(!value)result.status=S::UnresolvedSkill;return value;
        }
    };
}
CurrentCannonItemResult BuildCurrentCannonItemAvailability(const fates::runtime::native::NativeRuntime& r,std::uint16_t slot,bool silence) {
    if(slot>=r.game.units.size()||!r.game.units[slot].occupied)return {};
    CurrentDeploymentRangeResult out{};out.status=DeploymentRangeStatus::Ok;Services svc(r,r.game.units[slot],out);
    const auto value=CannonItemAvailableExact(silence,svc);
    return {out.status,value.value_or(false)};
}
CurrentDeploymentRangeResult BuildCurrentDeploymentRanges(const fates::runtime::native::NativeRuntime& r,
    std::uint16_t slot,std::uint32_t member,std::uint32_t args) {
    namespace rn=fates::runtime::native;using S=DeploymentRangeStatus;CurrentDeploymentRangeResult out{};
    if(slot>=r.game.units.size()||!r.game.units[slot].occupied)return out;
    const auto& rules=r.definitions.item_range_rules();if(!rules){out.status=S::MissingDefinition;return out;}
    Services svc(r,r.game.units[slot],out);
    out.status=S::Ok;const auto masks=BuildDeploymentRangesExact(member,args,*rules,svc);
    if(masks)out.masks=*masks;else if(out.status==S::Ok)out.status=S::InvalidRange;return out;
}
}
