#include "fates/ai/native_dispos_combat_initialization.hpp"
#include <array>
#include <algorithm>
#include "fates/runtime/native_initial_item_eligibility.hpp"
#include "fates/runtime/native_initial_weapon_exp.hpp"
#include "fates/runtime/native_item_inventory.hpp"

namespace fates::ai::native {
namespace {
int RoundRetailGrowth(const int product) noexcept {
    if(product<=0) return 0;
    int q=product/100;
    if((product%100)>49) ++q;
    return q;
}
int Clamp99(const int v) noexcept { return v<0?0:(v>99?99:v); }

using fates::runtime::native::PersonDefinition;
using fates::runtime::native::JobDefinition;
using fates::runtime::native::ItemDefinition;
using fates::runtime::native::UnitState;
using fates::runtime::native::DefinitionStore;

NativeDisposCombatInitStatus InitializeDisposEquipment(
    const DefinitionStore& defs, const PersonDefinition& person,
    const JobDefinition& job, const std::uint8_t difficulty,
    UnitState& unit, NativeDisposCombatInitResult& out) {
    const auto limits=fates::runtime::native::ResolveBaseWeaponExpLimits(person,job,defs.weapon_rank_thresholds());
    unit.weapon_exp=fates::runtime::native::CalculateFirstWeaponExp(
        person.weapon_exp,limits,person.enemy_flag,difficulty,defs.weapon_rank_thresholds());
    unit.equipped_item_id=0;unit.inventory={};
    std::array<fates::runtime::native::UnitItemState,5> current_items{};
    unit.initial_inventory_item_ids.fill(0);
    unit.initial_inventory_drop_flags.fill(false);
    std::array<fates::runtime::native::InitialItemEligibility,5> eligibility{};
    std::size_t inserted=0;
    for (std::size_t source=0; source<unit.dispos_item_ids.size(); ++source) {
        const auto id=unit.dispos_item_ids[source];
        const int adjustment=unit.dispos_item_difficulty_adjustments[source][difficulty];
        if (!id) continue;
        if (adjustment<0) { ++out.excluded_difficulty_items; continue; }
        out.failed_item_id=id;
        const auto flags=unit.dispos_item_flags[source];
        if (adjustment!=0 || (flags&~1u)!=0)
            return NativeDisposCombatInitStatus::UnsupportedItemVariant;
        const auto* item=defs.FindItem(id);
        if (!item) return NativeDisposCombatInitStatus::InvalidItem;
        // Retail discards enemy-only items for Person enemy_flag==1.
        const bool enemy_only=(item->bitflags[2]&0x80u)!=0;
        if (person.enemy_flag==1 && enemy_only) continue;
        const auto* sub=defs.FindItemSubKind(item->weapon_category);
        if (!sub) return NativeDisposCombatInitStatus::InvalidItem;
        const auto group=sub->weapon_exp_group;
        const auto eligible=fates::runtime::native::ResolveInitialItemEligibility(defs,person,job,*item,limits);
        if (eligible==fates::runtime::native::InitialItemEligibility::Unproved)
            return NativeDisposCombatInitStatus::UnsupportedEquipRestriction;
        const bool weapon=group<8 && group!=6;
        const bool dropped=(flags&1u)!=0 && (!enemy_only || (item->bitflags[0]&0x80u)!=0);
        // Player drop weapons and multi-use non-weapons can duplicate entries.
        // The bounded source-item projection does not claim that branch yet.
        if (dropped && ((weapon && person.enemy_flag==1) ||
                        (!weapon && (item->uses<0 || item->uses>1))))
            return NativeDisposCombatInitStatus::UnsupportedItemVariant;
        unit.initial_inventory_item_ids[inserted]=id;
        unit.initial_inventory_drop_flags[inserted]=dropped;
        auto instance=fates::runtime::native::NewItemInstanceExact(id,weapon,std::uint8_t(item->uses));
        if(dropped)instance.state|=0x8000u;
        // Original SetDisposItem gives non-player weapons variant1, then clears
        // that variant for the admitted unrefined drop branch.
        if(weapon && person.enemy_flag!=1)instance.state=std::uint16_t((instance.state&0xff80u)|1u);
        if(dropped)instance.state&=0xff80u;
        current_items[inserted]=instance;
        eligibility[inserted]=eligible;
        if (eligible==fates::runtime::native::InitialItemEligibility::Yes && person.enemy_flag!=1 &&
            (inserted==0 || !dropped)) {
            const auto before=unit.weapon_exp[group];
            unit.weapon_exp[group]=std::max(before,item->required_weapon_exp);
            if (before!=unit.weapon_exp[group]) ++out.stored_exp_raises;
        }
        ++inserted;
    }
    // Retail scans compact inventory after ALL accepted items can raise EXP.
    // Staff may raise EXP but cannot become the ordinary equipped battle weapon.
    std::array<bool,5> can_equip{};
    for (std::size_t i=0; i<inserted; ++i) {
        if (eligibility[i]!=fates::runtime::native::InitialItemEligibility::Yes) continue;
        const auto* item=defs.FindItem(unit.initial_inventory_item_ids[i]);
        const auto group=defs.FindItemSubKind(item->weapon_category)->weapon_exp_group;
        if (group==6) continue;
        if (std::min(unit.weapon_exp[group],limits[group])>=item->required_weapon_exp) {
            can_equip[i]=true;
        }
    }
    fates::runtime::native::EquipInventoryExact(current_items,-1,can_equip);
    if(fates::runtime::native::RestoreCurrentInventory(defs,unit,current_items)!=fates::runtime::native::InventoryStatus::Ok)
        return NativeDisposCombatInitStatus::InvalidItem;
    out.failed_item_id=0;
    return unit.equipped_item_id ? NativeDisposCombatInitStatus::Ok
        : NativeDisposCombatInitStatus::NoEquipableAuthoredWeapon;
}
}

NativeDisposCombatInitResult InitializeDisposCombatState(
    fates::runtime::native::NativeRuntime& runtime,
    const NativeDisposDifficulty difficulty,
    const std::span<const std::uint16_t> unit_slots) {
    NativeDisposCombatInitResult out{};
    const auto difficulty_byte=static_cast<std::uint8_t>(difficulty);
    if (!GameUserDifficultyMatches(runtime,difficulty_byte)) { out.status=NativeDisposCombatInitStatus::InvalidDifficulty; return out; }
    for(const auto slot:unit_slots) {
        out.failed_unit_slot=slot;
        if(slot>=runtime.game.units.size()) { out.status=NativeDisposCombatInitStatus::MissingDefinition; return out; }
        if(!runtime.game.units[slot].occupied) continue;
        out.failed_unit_slot=slot;
        // Per-unit staging: a failed equipment precondition must not publish a
        // partially initialized unit or a false combat_state_valid marker.
        auto u=runtime.game.units[slot];
        const auto* person=runtime.definitions.FindPerson(u.person_id);
        const auto* job=runtime.definitions.FindJob(u.job_id);
        if(!person||!job){out.status=NativeDisposCombatInitStatus::MissingDefinition;return out;}
        int level=u.dispos_authored_level?u.dispos_authored_level:person->level;
        if(level<1||level>99){out.status=NativeDisposCombatInitStatus::UnsupportedLevel;return out;}
        int growth_steps=person->enemy_flag==0?level-1:0;
        if(difficulty==NativeDisposDifficulty::Normal) growth_steps+=person->normal_auto_growth_offset;
        else if(difficulty==NativeDisposDifficulty::Lunatic) growth_steps+=person->lunatic_auto_growth_offset;
        if(growth_steps<0) growth_steps=0;
        std::array<int,8> stats{};
        for(std::size_t i=0;i<8;++i){
            const int job_growth=person->enemy_flag==1?job->player_growths[i]:job->enemy_growths[i];
            int growth=static_cast<int>(static_cast<std::int8_t>(person->growths[i]))+static_cast<int>(static_cast<std::int8_t>(job_growth));
            if(growth<0)growth=0;
            int delta=RoundRetailGrowth(growth*growth_steps);
            if(difficulty==NativeDisposDifficulty::Normal) delta+=static_cast<std::int8_t>(person->penalties[i]);
            else if(difficulty==NativeDisposDifficulty::Lunatic) delta+=static_cast<std::int8_t>(person->bonuses[i]);
            const int limit=Clamp99(static_cast<int>(static_cast<std::int8_t>(person->modifiers[i]))+static_cast<int>(static_cast<std::int8_t>(job->max_stats[i])));
            int value=static_cast<int>(static_cast<std::int8_t>(person->bases[i]))+static_cast<int>(static_cast<std::int8_t>(job->bases[i]))+delta;
            if(value<0)value=0;
            if(value>limit)value=limit;
            stats[i]=value; u.auto_growth_delta[i]=static_cast<std::int8_t>(delta);
        }
        u.max_hp=static_cast<std::int16_t>(stats[0]);u.current_hp=u.max_hp;
        u.strength=static_cast<std::int16_t>(stats[1]);u.magic=static_cast<std::int16_t>(stats[2]);
        u.skill=static_cast<std::int16_t>(stats[3]);u.speed=static_cast<std::int16_t>(stats[4]);
        u.luck=static_cast<std::int16_t>(stats[5]);u.defense=static_cast<std::int16_t>(stats[6]);u.resistance=static_cast<std::int16_t>(stats[7]);
        auto equipment_result=out;
        out.status=InitializeDisposEquipment(runtime.definitions,*person,*job,difficulty_byte,u,equipment_result);
        out.failed_item_id=equipment_result.failed_item_id;
        if(out.status!=NativeDisposCombatInitStatus::Ok) return out;
        out.excluded_difficulty_items=equipment_result.excluded_difficulty_items;
        out.stored_exp_raises=equipment_result.stored_exp_raises;
        u.complex_battle_rules=false;u.defeated=false;u.action_committed=false;
        u.create_from_dispos_combat_init_bound=true;u.create_level=static_cast<std::uint8_t>(level);u.combat_state_valid=true;
        runtime.game.units[slot]=u;
        out.failed_unit_slot=0xFFFFu;
        ++out.initialized_units;++out.equipped_units;
    }
    out.failed_unit_slot=0xFFFFu;
    return out;
}
} // namespace fates::ai::native
