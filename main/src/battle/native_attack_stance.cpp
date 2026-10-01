#include "fates/battle/native_attack_stance.hpp"
#include <algorithm>
#include <bit>
namespace fates::battle::native {
std::int32_t ResolveAssistPowerExact(std::int32_t power) noexcept {
    // GetSimplePower: arithmetic shift by one, THEN lower clamp. For negative
    // inputs the eventual clamp makes truncation-vs-floor immaterial.
    return power>0?power/2:0;
}
std::uint8_t ResolveAssistTimesExact(bool item,bool silenced_magic,
    std::uint32_t flags,std::uint32_t opposing,std::uint32_t battle) noexcept {
    if(!item || silenced_magic || !(flags&2u))return 0;
    // Assist bit 2 bypasses distance/weapon-range lookup. Initiator bit 1
    // bypasses the counter-side exclusions, but not its own 0x140 gate.
    if(!(flags&1u) && ((flags&0x100u)||(opposing&0x140u)||(battle&0x20u)))return 0;
    if(flags&0x140u)return 0;
    return 1;
}
ResolvedAttackSchedule BuildResolvedAttackScheduleExact(
    const std::array<std::int32_t,4>& counts,
    const std::array<std::uint32_t,4>& flags,bool defender_first) noexcept {
    ResolvedAttackSchedule out{};
    auto append=[&](std::uint8_t side,std::int32_t minimum) {
        if(counts[side]<minimum)return;
        out.sides[out.count++]=side;
        if(flags[side]&0x800u)out.sides[out.count++]=side;
    };
    const std::uint8_t first=defender_first?1:0,second=defender_first?0:1;
    append(first,1);append(std::uint8_t(first+2),1);
    append(second,1);append(std::uint8_t(second+2),1);
    append(first,2);append(second,2);
    return out;
}
}

#include "fates/battle/native_battle_dual_preparation.hpp"
namespace fates::battle::native {
namespace {
bool ValidDualUnit(const BattleDualView& v,std::uint16_t u) {
    return u!=kNoBattleDualUnit && u<v.units.size();
}
bool ValidDualReference(const BattleDualView& v,std::uint16_t u) {
    return u==kNoBattleDualUnit || ValidDualUnit(v,u);
}
}
BattleDualPreparationResult PrepareBattleDualSourcesExact(
    const BattleDualView& input,BattleDualPreparationServices& services) {
    BattleDualPreparationResult out{};out.view=input;
    auto finish=[&](BattleDualPreparationStatus status) {
        out.status=status;
        if(status!=BattleDualPreparationStatus::Ok){out.view=input;out.conditions_count=0;}
        return out;
    };
    auto& view=out.view;
    if((view.battle_flags&0x10u)||(view.sides[0].flags&0x40u))
        return finish(BattleDualPreparationStatus::Ok);
    for(std::uint8_t i=0;i<2;++i) {
        auto& primary=view.sides[i];auto& assist=view.sides[i+2];
        if(!ValidDualUnit(view,primary.unit))return finish(BattleDualPreparationStatus::InvalidUnitGraph);
        const auto& unit=view.units[primary.unit];
        if(!ValidDualReference(view,unit.partner)||!ValidDualReference(view,unit.clone))
            return finish(BattleDualPreparationStatus::InvalidUnitGraph);
        if(primary.source==kNoBattleDualUnit && !(view.battle_flags&0x18u) &&
           (!(primary.flags&1u)||!(view.battle_flags&4u))) {
            const auto selected=services.SelectLocalSource(view,i);
            if(!selected)return finish(BattleDualPreparationStatus::UnresolvedSelection);
            primary.source=*selected;
        }
        if(!ValidDualReference(view,primary.source))return finish(BattleDualPreparationStatus::InvalidUnitGraph);
        if(primary.source!=kNoBattleDualUnit) {
            const auto level=services.RelianceLevel(view,i,primary.source);
            if(!level)return finish(BattleDualPreparationStatus::UnresolvedReliance);
            primary.reliance_level=*level;
        }
        assist.unit=primary.source;assist.reliance_level=primary.reliance_level;
        // This checks presence of the PRIMARY's pair, not identity of source.
        if(unit.partner!=kNoBattleDualUnit){assist.x=primary.x;assist.y=primary.y;}
        if(unit.clone!=kNoBattleDualUnit && unit.clone==primary.source) {
            primary.flags|=4u;assist.flags|=4u;
        }
        // An existing clone flag also inherits equipment, even without a match.
        if(primary.flags&4u){assist.item=primary.item;assist.requested_item_index=primary.requested_item_index;}
    }
    out.conditions_count=2;return finish(BattleDualPreparationStatus::Ok);
}
void CompleteBattleSidePositionExact(std::int32_t& x,std::int32_t& y,
    std::uint32_t flags,std::int8_t unit_x,std::int8_t unit_y) noexcept {
    if(flags&8u){x=y=-1;return;}
    if(x<0)x=unit_x;
    if(y<0)y=unit_y;
}
BattleDualPreparationStatus CompleteBattleSidePositionExact(BattleDualView& v,std::uint8_t i) {
    if(i>=v.sides.size())return BattleDualPreparationStatus::InvalidUnitGraph;
    auto& side=v.sides[i];
    if(side.unit==kNoBattleDualUnit)return BattleDualPreparationStatus::Ok;
    if(!ValidDualUnit(v,side.unit))return BattleDualPreparationStatus::InvalidUnitGraph;
    const auto& unit=v.units[side.unit];
    CompleteBattleSidePositionExact(side.x,side.y,side.flags,unit.x,unit.y);
    return BattleDualPreparationStatus::Ok;
}
BattleCalculationPairFlags ProjectBattleCalculationPairFlagsExact(const BattleDualView& v) {
    BattleCalculationPairFlags out{};
    // Validate every dereferenced pair before producing either phase. Original
    // objects require these pointers; malformed semantic graphs are unresolved.
    for(const auto& side:v.sides) {
        if(!ValidDualReference(v,side.unit))return out;
        if(side.unit!=kNoBattleDualUnit && (v.units[side.unit].public_flags&4u) &&
           !ValidDualUnit(v,v.units[side.unit].partner))return out;
    }
    for(const auto& u:v.units)out.during_calculation.push_back(u.public_flags);
    for(std::size_t i=0;i<4;++i) {
        const auto slot=v.sides[i].unit;if(slot==kNoBattleDualUnit)continue;
        const auto& u=v.units[slot];if(!(u.public_flags&4u))continue;
        if(i>=2) {
            const auto primary=v.sides[i-2].unit;
            if(primary==kNoBattleDualUnit || v.units[primary].partner==slot)continue;
        }
        out.during_calculation[slot]|=0x200000u;
        out.during_calculation[u.partner]|=0x200000u;
    }
    out.after_cleanup=out.during_calculation;
    for(const auto& side:v.sides)if(side.unit!=kNoBattleDualUnit) {
        const auto& u=v.units[side.unit];if(!(u.public_flags&4u))continue;
        out.after_cleanup[side.unit]&=~0x200000u;
        out.after_cleanup[u.partner]&=~0x200000u;
    }
    out.status=BattleDualPreparationStatus::Ok;return out;
}
}

namespace fates::battle::native {
bool BattlePairCapabilityAppliesExact(std::uint32_t flags,bool partner) noexcept {
    return partner && (flags&((flags&0x200000u)?4u:2u))!=0;
}
}

namespace fates::battle::native {
std::int32_t ResolveBattlePairCapabilityExact(std::int32_t base,std::int32_t bonus,
    std::uint32_t flags,bool partner) noexcept {
    if(BattlePairCapabilityAppliesExact(flags,partner))
        base=std::bit_cast<std::int32_t>(std::uint32_t(base)+std::uint32_t(bonus));
    return std::clamp(base,0,99);
}
}
