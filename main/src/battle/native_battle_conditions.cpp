#include "fates/battle/native_battle_conditions.hpp"
#include "fates/runtime/native_item_inventory.hpp"
#include "fates/runtime/native_current_item_eligibility.hpp"
#include "fates/runtime/native_initial_weapon_exp.hpp"
#include <algorithm>
namespace fates::battle::native {
namespace rn=fates::runtime::native;
std::uint16_t ResolveUnitCategoryExact(std::uint16_t category,std::uint64_t unit,
    std::uint64_t person,std::uint64_t job) noexcept {
    const auto flags=unit|person|job;
    if(flags&(1ull<<23))category|=2;
    if(flags&(1ull<<24))category|=4;
    if(flags&(1ull<<34))category&=std::uint16_t(~2u);
    return category;
}
std::int32_t HeldInventoryIndexExact(const std::array<rn::UnitItemState,5>& items,
    const std::array<std::uint8_t,5>& groups,const std::array<bool,5>& eligible) noexcept {
    for(std::size_t i=0;i<5;++i)if((items[i].state&0x4000u) || (groups[i]==6 && eligible[i]))return std::int32_t(i);
    return -1;
}
BattleConditionResult CompleteBattleSideConditionsExact(const BattleSideConditions& input,
    std::uint32_t battle_flags,BattleSideConditionServices& svc) {
    auto value=input;auto& side=value.side;
    auto fail=[&](BattleConditionStatus status){return BattleConditionResult{status,input};};
    if(side.unit==kNoBattleDualUnit)return {BattleConditionStatus::Ok,value};
    std::array<std::int8_t,2> xy{};
    if(!(side.flags&8u) && (side.x<0 || side.y<0)) {
        const auto v=svc.Coordinates(side.unit);if(!v)return fail(BattleConditionStatus::UnresolvedCoordinates);xy=*v;
    }
    CompleteBattleSidePositionExact(side.x,side.y,side.flags,xy[0],xy[1]);
    if(side.item[0]==0) {
        if(side.requested_item_index<0) {
            const auto i=svc.EquippedIndex(side.unit);if(!i)return fail(BattleConditionStatus::UnresolvedEquipment);side.requested_item_index=*i;
        }
        if(side.requested_item_index>=0) {
            const auto item=svc.InventoryItem(side.unit,side.requested_item_index);if(!item)return fail(BattleConditionStatus::UnresolvedEquipment);side.item={item->item_id,item->state};
        }
    }
    if(side.item[0]!=0) {
        const auto f=svc.ItemFacts({side.item[0],side.item[1]});if(!f)return fail(BattleConditionStatus::UnresolvedItem);
        if(f->magic)side.flags|=0x20;
        if(f->staff){side.flags|=0x40;if(f->obstruct_staff)side.flags|=0x80;}
        if(f->long_range)side.flags|=0x100;
        if(f->absorb_half)side.flags|=0x1000;
        if((side.flags&1u)&&f->attack_twice)side.flags|=0x800;
    }
    if(side.requested_item_index>=0)value.resolved_item_index=side.requested_item_index;
    else {
        const auto i=svc.HeldIndex(side.unit);if(!i)return fail(BattleConditionStatus::UnresolvedHeldIndex);value.resolved_item_index=*i;
    }
    if(!value.terrain_id) {
        const auto t=svc.TerrainAt(side.x,side.y);if(!t)return fail(BattleConditionStatus::UnresolvedTerrain);value.terrain_id=t->id;
    }
    bool suppressed=!value.terrain_id || (side.flags&8u)!=0;
    if(!suppressed) {
        const auto category=svc.Category(side.unit);if(!category)return fail(BattleConditionStatus::UnresolvedCategory);
        if(*category&1u){const auto skill=svc.HasWingShield(side.unit);if(!skill)return fail(BattleConditionStatus::UnresolvedSkill);suppressed=!*skill;}
    }
    if(suppressed)side.flags|=0x10;
    if((battle_flags&0x40u)&&(side.flags&1u))side.flags|=0x4000;
    return {BattleConditionStatus::Ok,value};
}
std::optional<BattleConditionItemFacts> ProjectBattleConditionItemFacts(const rn::DefinitionStore& defs,rn::UnitItemState instance) {
    const auto* item=defs.FindItem(instance.item_id);if(!item)return std::nullopt;
    const auto* sub=defs.FindItemSubKind(item->weapon_category);if(!sub)return std::nullopt;
    // Original named ItemSkill indices: Far2, AbsorbHalf13, ObstructStaff16,
    // AttackTwice35. Independently checked against original GameData labels.
    return BattleConditionItemFacts{defs.IsMagicItem(*item),sub->weapon_exp_group==6,
        (item->bitflags[2]&1u)!=0,(item->bitflags[0]&4u)!=0,
        (item->bitflags[1]&0x20u)!=0,(item->bitflags[4]&8u)!=0};
}
namespace {
std::uint64_t Flags(const std::array<std::uint8_t,8>& bytes) {
    std::uint64_t flags=0;for(unsigned i=0;i<8;++i)flags|=std::uint64_t(bytes[i])<<(8*i);return flags;
}
class RuntimeServices : public BattleSideConditionServices {
    const rn::NativeRuntime& runtime;
    const rn::UnitState* selected_unit{};
    std::optional<rn::UnitItemState> selected_item{};
    std::int32_t selected_index{};
    const rn::UnitState* Unit(std::uint16_t slot) const {
        if(selected_unit)return selected_unit;
        return slot<runtime.game.units.size()&&runtime.game.units[slot].occupied?&runtime.game.units[slot]:nullptr;
    }
public:
    explicit RuntimeServices(const rn::NativeRuntime& r):runtime(r){}
    RuntimeServices(const rn::NativeRuntime& r,const rn::UnitState& u,rn::UnitItemState item,std::int32_t index=0):runtime(r),selected_unit(&u),selected_item(item),selected_index(index){}
    std::optional<std::array<std::int8_t,2>> Coordinates(std::uint16_t slot) override {
        const auto* u=Unit(slot);if(!u||u->x<-128||u->x>127||u->y<-128||u->y>127)return std::nullopt;
        return std::array<std::int8_t,2>{std::int8_t(u->x),std::int8_t(u->y)};
    }
    std::optional<std::int32_t> EquippedIndex(std::uint16_t slot) override {
        const auto* u=Unit(slot);if(!u||rn::ValidateCurrentInventory(*u)!=rn::InventoryStatus::Ok)return std::nullopt;
        return rn::EquippedInventoryIndexExact(u->inventory.items);
    }
    std::optional<rn::UnitItemState> InventoryItem(std::uint16_t slot,std::int32_t index) override {
        if(selected_item&&index==selected_index)return selected_item;
        const auto* u=Unit(slot);if(!u||index<0||index>=5||rn::ValidateCurrentInventory(*u)!=rn::InventoryStatus::Ok)return std::nullopt;
        return u->inventory.items[std::size_t(index)];
    }
    std::optional<BattleConditionItemFacts> ItemFacts(rn::UnitItemState item) override {return ProjectBattleConditionItemFacts(runtime.definitions,item);}
    std::optional<std::int32_t> HeldIndex(std::uint16_t slot) override {
        const auto* u=Unit(slot);if(!u||rn::ValidateCurrentInventory(*u)!=rn::InventoryStatus::Ok)return std::nullopt;
        // Resolve only visited slots. A staff before a later equipped weapon wins.
        for(std::size_t i=0;i<5;++i) {
            const auto instance=u->inventory.items[i];if(instance.state&0x4000u)return std::int32_t(i);
            const auto* item=runtime.definitions.FindItem(instance.item_id);if(!item)return std::nullopt;
            const auto* sub=runtime.definitions.FindItemSubKind(item->weapon_category);if(!sub)return std::nullopt;
            if(sub->weapon_exp_group!=6)continue;
            const auto eligible=rn::ProjectCurrentItemEligibility(runtime,*u,*item,true,true);
            if(eligible==rn::CurrentItemEligibility::Yes)return std::int32_t(i);
            if(eligible!=rn::CurrentItemEligibility::No)return std::nullopt;
        }return -1;
    }
    std::optional<BattleConditionTerrain> TerrainAt(std::int32_t x,std::int32_t y) override {
        const auto* map=runtime.definitions.terrain_map();if(!map||map->width>32||map->height>32)return std::nullopt;
        if(x<0||y<0||std::uint32_t(x)>=map->width||std::uint32_t(y)>=map->height)return BattleConditionTerrain{};
        const auto* t=runtime.definitions.TerrainAt(std::uint32_t(x),std::uint32_t(y));
        return BattleConditionTerrain{t?std::optional(t->id):std::nullopt};
    }
    std::optional<std::uint16_t> Category(std::uint16_t slot) override {
        const auto* u=Unit(slot);return u?ProjectUnitBattleCategory(runtime,*u):std::nullopt;
    }
    std::optional<bool> HasWingShield(std::uint16_t slot) override {
        const auto* u=Unit(slot);if(!u||!runtime.definitions.FindSkill(122))return std::nullopt;
        const auto* person=runtime.definitions.FindPerson(u->person_id);if(!person)return std::nullopt;
        return rn::ProjectCurrentEquippedSkill(runtime,*u,122);
    }
};
}
std::optional<std::uint16_t> ProjectUnitBattleCategory(const rn::NativeRuntime& r,const rn::UnitState& u) {
    const auto* p=r.definitions.FindPerson(u.person_id);const auto* j=r.definitions.FindJob(u.job_id);if(!p||!j)return std::nullopt;
    return ResolveUnitCategoryExact(r.definitions.BaseJobCategoryMask(*j),Flags(u.private_skill_bits),Flags(p->bitflags),Flags(j->bitflags));
}
BattleConditionResult ProjectCurrentBattleSideConditions(const rn::NativeRuntime& r,const BattleSideConditions& input,std::uint32_t flags) {
    if(input.side.unit!=kNoBattleDualUnit && (input.side.unit>=r.game.units.size()||!r.game.units[input.side.unit].occupied))
        return {BattleConditionStatus::InvalidUnit,input};
    RuntimeServices services(r);return CompleteBattleSideConditionsExact(input,flags,services);
}
BattleConditionResult ProjectSelectedBattleSideConditions(const rn::NativeRuntime& r,const rn::UnitState& u,
    rn::UnitItemState item,std::uint32_t side_flags,std::uint32_t battle_flags) {
    BattleSideConditions input{};input.side.unit=0;input.side.item={item.item_id,item.state};input.side.requested_item_index=0;
    input.side.x=u.x;input.side.y=u.y;input.side.flags=side_flags;
    if(!u.occupied)return {BattleConditionStatus::InvalidUnit,input};
    RuntimeServices services(r,u,item);return CompleteBattleSideConditionsExact(input,battle_flags,services);
}
BattleConditionResult ProjectSelectedBattleSideConditions(const rn::NativeRuntime& r,const rn::UnitState& u,
    rn::UnitItemState item,const BattleSideConditions& input,std::uint32_t battle_flags) {
    if(!u.occupied)return {BattleConditionStatus::InvalidUnit,input};
    if(input.side.requested_item_index<0 || input.side.requested_item_index>=5)return {BattleConditionStatus::UnresolvedEquipment,input};
    RuntimeServices services(r,u,item,input.side.requested_item_index);return CompleteBattleSideConditionsExact(input,battle_flags,services);
}
}
