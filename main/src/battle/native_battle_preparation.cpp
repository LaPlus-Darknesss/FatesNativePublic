#include "fates/battle/native_battle_preparation.hpp"
#include "fates/runtime/native_item_inventory.hpp"
#include <bit>
namespace fates::battle::native {
namespace rn=fates::runtime::native;
BattleSideConditions BattlePreparationState::Side(std::uint8_t i) const {return {view.sides[i],resolved_item_indices[i],terrain_ids[i]};}
void BattlePreparationState::SetSide(std::uint8_t i,const BattleSideConditions& side) {view.sides[i]=side.side;resolved_item_indices[i]=side.resolved_item_index;terrain_ids[i]=side.terrain_id;}
std::int32_t BattleDistanceExact(std::int32_t x0,std::int32_t y0,std::int32_t x1,std::int32_t y1) noexcept {
    auto magnitude=[](std::uint32_t v){return (v&0x80000000u)?0u-v:v;};
    return std::bit_cast<std::int32_t>(magnitude(std::uint32_t(x0)-std::uint32_t(x1))+magnitude(std::uint32_t(y0)-std::uint32_t(y1)));
}
bool MovementForbiddenExact(std::uint64_t unit,std::uint64_t person,std::uint64_t job) noexcept {return ((unit|person|job)&(1ull<<6))!=0;}
BattlePreparationResult CompleteBattleConditionsExact(const BattlePreparationState& input,BattlePreparationServices& services) {
    BattlePreparationResult out{};out.state=input;auto& state=out.state;auto& sides=state.view.sides;const auto flags=state.view.battle_flags;
    auto fail=[&](BattlePreparationStatus status){out.status=status;out.state=input;return out;};
    sides[0].flags|=1;sides[2].flags|=3;sides[3].flags|=2;
    if(flags&2u){for(auto& side:sides)side.flags|=8;}
    else if(flags&4u){sides[0].flags|=8;sides[2].flags|=8;}
    for(std::uint8_t i=0;i<2;++i) {
        const auto completed=services.CompleteSide(state,i);
        if(completed.status!=BattleConditionStatus::Ok){out.side_status=completed.status;return fail(BattlePreparationStatus::UnresolvedSide);}
        state.SetSide(i,completed.value);
        const auto item=sides[i].item;
        if(item[0]) {
            const auto suppress=services.ItemNegatesTerrain({item[0],item[1]});
            if(!suppress)return fail(BattlePreparationStatus::UnresolvedTerrainRule);
            if(*suppress){sides[0].flags|=0x10;sides[1].flags|=0x10;}
        }
    }
    if(state.distance<=0)state.distance=(flags&1u)?0:BattleDistanceExact(sides[0].x,sides[0].y,sides[1].x,sides[1].y);
    if(!(flags&2u)) {
        state.movement_rule_byte=1;
        if(sides[1].unit==kNoBattleDualUnit)state.movement_rule_byte=0;
        else {
            const auto defender=services.MovementForbidden(sides[1].unit);
            if(!defender)return fail(BattlePreparationStatus::UnresolvedMovementRule);
            if(!*defender) {
                if(sides[0].unit==kNoBattleDualUnit)return fail(BattlePreparationStatus::InvalidUnit);
                const auto actor=services.MovementForbidden(sides[0].unit);
                if(!actor)return fail(BattlePreparationStatus::UnresolvedMovementRule);
                if(*actor && !(flags&4u))state.movement_rule_byte=0;
            }
        }
    }
    if(sides[0].flags&0x40u) {
        if(sides[0].unit==kNoBattleDualUnit)return fail(BattlePreparationStatus::InvalidUnit);
        const auto clone=services.Clone(sides[0].unit);
        if(!clone)return fail(BattlePreparationStatus::UnresolvedClone);
        if(*clone!=kNoBattleDualUnit && *clone==sides[1].unit)sides[0].flags|=4;
    }
    return out;
}
BattlePreparationResult CompleteBattleSourcesExact(const BattlePreparationState& input,BattleDualPreparationServices& dual,BattlePreparationServices& services) {
    BattlePreparationResult out{};out.state=input;const auto prepared=PrepareBattleDualSourcesExact(input.view,dual);
    if(prepared.status!=BattleDualPreparationStatus::Ok){out.status=BattlePreparationStatus::UnresolvedSources;out.source_status=prepared.status;return out;}
    out.state.view=prepared.view;
    for(std::uint8_t n=0;n<prepared.conditions_count;++n) {
        const auto i=prepared.conditions_order[n];const auto completed=services.CompleteSide(out.state,i);
        if(completed.status!=BattleConditionStatus::Ok){out.status=BattlePreparationStatus::UnresolvedSide;out.side_status=completed.status;out.state=input;return out;}
        out.state.SetSide(i,completed.value);
    }
    return out;
}
BattlePreparationResult PrepareBattleContextExact(const BattlePreparationState& input,BattleDualPreparationServices& dual,BattlePreparationServices& services) {
    auto out=CompleteBattleConditionsExact(input,services);if(out.status!=BattlePreparationStatus::Ok)return out;
    out=CompleteBattleSourcesExact(out.state,dual,services);if(out.status!=BattlePreparationStatus::Ok)out.state=input;return out;
}
std::optional<bool> ProjectBattleItemTerrainRule(const rn::DefinitionStore& defs,rn::UnitItemState item) {
    const auto* d=defs.FindItem(item.item_id);return d?std::optional((d->bitflags[7]&4u)!=0):std::nullopt;
}
namespace {
std::uint64_t Flags(const std::array<std::uint8_t,8>& bytes) {std::uint64_t v=0;for(unsigned i=0;i<8;++i)v|=std::uint64_t(bytes[i])<<(8*i);return v;}
class RuntimePreparationServices final:public BattlePreparationServices {
    const rn::NativeRuntime& runtime;const BattleCalculationItems* selected;
public:
    RuntimePreparationServices(const rn::NativeRuntime& r,const BattleCalculationItems* items=nullptr):runtime(r),selected(items){}
    BattleConditionResult CompleteSide(const BattlePreparationState& s,std::uint8_t i) override {
        auto side=s.Side(i);
        if(!selected || side.side.unit==kNoBattleDualUnit)return ProjectCurrentBattleSideConditions(runtime,side,s.view.battle_flags);
        const auto slot=side.side.unit;
        if(slot>=runtime.game.units.size() || !runtime.game.units[slot].occupied)return {BattleConditionStatus::InvalidUnit,side};
        const auto& u=runtime.game.units[slot];
        const auto item=side.side.item[0]?std::optional(rn::UnitItemState{side.side.item[0],side.side.item[1]}):((*selected)[i]?(*selected)[i]:rn::ReadCalculationEquippedItem(u));
        if(!item)return {BattleConditionStatus::UnresolvedEquipment,side};
        // Selection is a caller-owned calculation input, separate from the
        // current inventory writer. Known empty selected items are explicit too.
        side.side.item={item->item_id,item->state};
        if(side.side.requested_item_index<0)side.side.requested_item_index=0;
        return ProjectSelectedBattleSideConditions(runtime,u,*item,side,s.view.battle_flags);
    }
    std::optional<bool> ItemNegatesTerrain(rn::UnitItemState item) override {return ProjectBattleItemTerrainRule(runtime.definitions,item);}
    std::optional<bool> MovementForbidden(std::uint16_t slot) override {return ProjectBattleMovementRule(runtime,slot);}
    std::optional<std::uint16_t> Clone(std::uint16_t) override {return std::nullopt;}
};
}
std::optional<bool> ProjectBattleMovementRule(const rn::NativeRuntime& r,std::uint16_t slot) {
    if(slot>=r.game.units.size() || !r.game.units[slot].occupied)return std::nullopt;
    const auto& u=r.game.units[slot];const auto* p=r.definitions.FindPerson(u.person_id);const auto* j=r.definitions.FindJob(u.job_id);
    if(!p||!j)return std::nullopt;
    return MovementForbiddenExact(Flags(u.private_skill_bits),Flags(p->bitflags),Flags(j->bitflags));
}
BattlePreparationResult ProjectCurrentBattleConditions(const rn::NativeRuntime& r,const BattlePreparationState& input) {
    RuntimePreparationServices svc(r);return CompleteBattleConditionsExact(input,svc);
}
BattlePreparationResult ProjectBattleCalculationConditions(const rn::NativeRuntime& r,const BattlePreparationState& input,const BattleCalculationItems& items) {
    RuntimePreparationServices svc(r,&items);return CompleteBattleConditionsExact(input,svc);
}
BattlePreparationResult ProjectBattleCalculationSources(const rn::NativeRuntime& r,const BattlePreparationState& input,const BattleCalculationItems& items,BattleDualPreparationServices& dual) {
    RuntimePreparationServices svc(r,&items);return CompleteBattleSourcesExact(input,dual,svc);
}
}
