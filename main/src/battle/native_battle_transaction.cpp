#include "fates/battle/native_battle_transaction.hpp"
#include "fates/runtime/native_unit_pair.hpp"
#include "fates/support/native_pair_bonus.hpp"
#include "fates/battle/native_battle_semantics.hpp"
#include "fates/battle/native_attack_stance.hpp"
#include "fates/battle/native_battle_dual_preparation.hpp"
#include "fates/battle/native_battle_preparation.hpp"
#include "fates/runtime/native_current_item_eligibility.hpp"
#include "fates/runtime/native_item_inventory.hpp"
#include <memory>
#include "fates/runtime/native_rng.hpp"
#include "fates/battle/native_battle_postcombat.hpp"
#include <algorithm>
#include <array>
#include <cstdlib>

namespace fates::battle::native {
namespace {
constexpr std::array<std::uint16_t,101> kHybridHitThreshold={0, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 1100, 1200, 1300, 1400, 1500, 1600, 1700, 1800, 1900, 2000, 2100, 2200, 2300, 2400, 2500, 2600, 2700, 2800, 2900, 3000, 3100, 3200, 3300, 3400, 3500, 3600, 3700, 3800, 3900, 4000, 4100, 4200, 4300, 4400, 4500, 4600, 4700, 4800, 4900, 5000, 5142, 5286, 5432, 5579, 5726, 5874, 6023, 6172, 6321, 6470, 6618, 6765, 6912, 7057, 7201, 7343, 7482, 7620, 7755, 7887, 8016, 8142, 8265, 8384, 8500, 8611, 8718, 8821, 8920, 9014, 9104, 9189, 9269, 9345, 9416, 9483, 9545, 9603, 9656, 9705, 9750, 9790, 9827, 9861, 9891, 9918, 9942, 9963, 9982, 10000};

bool Zero8(const std::array<std::uint8_t,8>& a) noexcept {
    return std::all_of(a.begin(),a.end(),[](auto v){return v==0;});
}

// Pass77 supports combat-semantic ordinary items while allowing restrictions
// that matter only before an item is already equipped (for example Enemy Only).
// Any bit with an unowned in-combat/post-combat effect remains fail-closed.
bool SupportedItem(const fates::runtime::native::DefinitionStore& defs,
                   const fates::runtime::native::ItemDefinition& item) noexcept {
    const auto group=defs.WeaponExpGroupForItem(item);
    if(group>=8 || group==6) return false; // no tools, staves or rods in attack exchange
    if(item.min_range<1 || item.max_range<item.min_range || item.movement!=0) return false;
    if(!Zero8(item.bonuses) || !Zero8(item.extra_data)) return false;
    // byte0: permit only Magic Weapon and Valuables; all other known bits can
    // alter use/counter/effectiveness/staff behavior.
    if((item.bitflags[0] & static_cast<std::uint8_t>(~0x82u))!=0) return false;
    // byte1: equip/inventory restrictions and infinite uses are exchange-neutral;
    // recovery/absorb/staff families are not yet owned.
    if((item.bitflags[1] & static_cast<std::uint8_t>(~0x0Fu))!=0) return false;
    // byte2 is equipment eligibility (including Enemy Only). Once equipped it
    // does not change this bounded exchange; group6 already excludes staves.
    // Pass78 owns exactly bitflags_5 bit0: ISID_戦闘後能力減少.
    // Pass107 owns ISID_必殺奥義禁止 (Paragon bitflags_4 bit3). Active
    // offensive proc skills are still rejected by the ordinary skill gate.
    if((item.bitflags[3] & static_cast<std::uint8_t>(~0x08u))!=0) return false;
    if((item.bitflags[4] & static_cast<std::uint8_t>(~0x01u))!=0) return false;
    for(std::size_t i=5;i<7;++i) if(item.bitflags[i]!=0) return false;
    if((item.bitflags[7]&std::uint8_t(~4u))!=0)return false; // ISID_terrain-negation, bit58
    return true;
}

const fates::runtime::native::TerrainDefinition* TerrainOf(
    const fates::runtime::native::NativeRuntime& r,
    const fates::runtime::native::UnitState& u) {
    return u.has_position && u.x>=0 && u.y>=0
        ? r.definitions.TerrainAt(static_cast<std::uint32_t>(u.x),static_cast<std::uint32_t>(u.y))
        : nullptr;
}

using BattleItems=BattleCalculationItems;
const fates::runtime::native::UnitItemState* OverrideItem(const BattleItems* items,std::size_t side) {return items && (*items)[side]?&*(*items)[side]:nullptr;}

struct SideResolved {
    fates::runtime::native::ItemCombatValues item_values{};
    std::uint32_t condition_flags{};
    bool terrain_applies{};
    const fates::runtime::native::JobDefinition* job{};
    const fates::runtime::native::ItemDefinition* item{};
    const fates::runtime::native::ItemSubKindDefinition* subkind{};
    const fates::runtime::native::TerrainDefinition* terrain{};
    int rank{-1};
    std::uint8_t weapon_group{0xFF};
    std::uint8_t effective_weapon_exp{};
    std::uint16_t category_mask{};
    bool magic{};
};

bool Resolve(const fates::runtime::native::NativeRuntime& r,
             const fates::runtime::native::UnitState& u,
             SideResolved& out, const bool allow_unarmed_defender = false,
             const fates::runtime::native::UnitItemState* selected=nullptr,std::uint32_t side_flags=0,
             const BattleSideConditions* prepared=nullptr) {
    out.job=r.definitions.FindJob(u.job_id);
    out.terrain=TerrainOf(r,u);
    if(!out.job || !out.terrain) return false;
    const auto category=ProjectUnitBattleCategory(r,u);if(!category)return false;
    out.category_mask=*category;
    // Retail Side::CalculateDetail retains the present Unit with an empty
    // unit::Item. It is not a missing side or a synthetic zero-stat weapon.
    // Only explicit absence qualifies: unknown IDs, wrong-class equipment and
    // staves in the attack-equipment field retain their ordinary refusal.
    const auto instance=selected?std::optional(*selected):fates::runtime::native::ReadCalculationEquippedItem(u);
    if(!instance)return false;
    const auto conditions=prepared?BattleConditionResult{BattleConditionStatus::Ok,*prepared}:ProjectSelectedBattleSideConditions(r,u,*instance,side_flags);
    if(conditions.status!=BattleConditionStatus::Ok)return false;
    out.condition_flags=conditions.value.side.flags;
    out.terrain_applies=conditions.value.terrain_id.has_value() && !(out.condition_flags&0x10u);
    if(instance->item_id==0) return allow_unarmed_defender;
    out.item=r.definitions.FindItem(instance->item_id);
    if(!out.item || !SupportedItem(r.definitions,*out.item)) return false;
    out.subkind=r.definitions.FindItemSubKind(out.item->weapon_category);
    if(!out.subkind || out.subkind->weapon_exp_group>=8) return false;
    out.weapon_group=out.subkind->weapon_exp_group;
    const auto* person=r.definitions.FindPerson(u.person_id);if(!person)return false;
    const auto limits=fates::runtime::native::ResolveCurrentWeaponExpLimits(*person,*out.job,u,r.definitions.weapon_rank_thresholds());
    out.effective_weapon_exp=std::min(u.weapon_exp[out.weapon_group],limits[out.weapon_group]);
    out.rank=r.definitions.WeaponRankIndex(out.effective_weapon_exp);
    out.magic=(out.condition_flags&0x20u)!=0;
    const auto values=fates::runtime::native::ProjectItemCombatValues(r.definitions,*instance);
    if(!values)return false;
    out.item_values=*values;
    return out.rank>=0 && out.effective_weapon_exp>=out.item->required_weapon_exp;
}

std::int32_t Clamp100(std::int32_t v) noexcept { return std::clamp(v,0,100); }

bool PairFactsValid(const fates::runtime::native::NativeRuntime& r,std::uint16_t slot) {
    using fates::runtime::native::PairRole;
    const auto& u=r.game.units[slot];
    if(!u.pair.bound)return u.pair.role==PairRole::None;
    if(u.pair.partner_slot>=r.game.units.size() || u.pair.partner_slot==slot ||
       (u.pair.role!=PairRole::Lead && u.pair.role!=PairRole::Partner))return false;
    const auto& p=r.game.units[u.pair.partner_slot];
    return p.occupied && !p.defeated && p.force_type==u.force_type && p.pair.bound &&
        p.pair.partner_slot==slot && p.pair.role==(u.pair.role==PairRole::Lead?PairRole::Partner:PairRole::Lead);
}
struct ForcedSourceContext {
    BattlePreparationState conditions{};
    std::array<fates::support::native::LocalSupportProjection,2> support{};
};
bool ApplyPairCapabilityProjection(const fates::runtime::native::NativeRuntime& r,
                                   const std::uint16_t self_slot,
                                   fates::runtime::native::UnitState& self,
                                   BattlePreviewSide& metadata,bool calculation_pair_role=false) {
    using fates::runtime::native::PairRole;
    if (!self.pair.bound) return true;
    if(calculation_pair_role) {
        if(!PairFactsValid(r,self_slot))return false;
        if(!BattlePairCapabilityAppliesExact(self.flags,true))return true;
    } else if(self.pair.role==PairRole::Partner)return false;
    if ((!calculation_pair_role && self.pair.role != PairRole::Lead) || self.pair.partner_slot >= r.game.units.size() ||
        (!r.game.support_context.ordinary_source_bound && !self.pair.person_guard_bonus_bound)) return false;
    const auto partner_slot=self.pair.partner_slot;
    const auto& partner=r.game.units[partner_slot];
    if (!partner.occupied || partner.defeated || partner.force_type!=self.force_type ||
        !partner.pair.bound || partner.pair.role!=(self.pair.role==PairRole::Lead?PairRole::Partner:PairRole::Lead) ||
        partner.pair.partner_slot!=self_slot) return false;
    const auto* partner_job=r.definitions.FindJob(partner.job_id);
    if(!partner_job) return false;
    std::optional<fates::support::native::PairBonusProjection> current;
    if(r.game.support_context.ordinary_source_bound) {
        current=fates::support::native::ProjectCurrentPairBonuses(r,self_slot);
        if(current->status!=fates::support::native::PairBonusStatus::Ok)return false;
    }
    metadata.guard_stance_pair=true;
    metadata.pair_partner_slot=partner_slot;
    metadata.guard_progress_sum=static_cast<std::uint32_t>(self.pair.guard_progress)+
                                static_cast<std::uint32_t>(partner.pair.guard_progress);
    auto bonus=[&](std::size_t i)->int {
        const int job=static_cast<std::int8_t>(partner_job->pair_up_bonuses[i]);
        const int person=static_cast<int>(self.pair.person_guard_bonus[i]);
        const int total=current?current->total[i]:job+person;
        metadata.pair_capability_bonus[i]=static_cast<std::int16_t>(total);
        return total;
    };
    // Retail Unit::GetDoubleCapability returns zero for capability index 0.
    metadata.pair_capability_bonus[0]=0;
    auto apply=[&](std::int16_t base,std::size_t i) {
        const auto amount=bonus(i);
        return static_cast<std::int16_t>(calculation_pair_role
            ?ResolveBattlePairCapabilityExact(base,amount,self.flags,true):int(base)+amount);
    };
    self.strength=apply(self.strength,1);self.magic=apply(self.magic,2);
    self.skill=apply(self.skill,3);self.speed=apply(self.speed,4);self.luck=apply(self.luck,5);
    self.defense=apply(self.defense,6);self.resistance=apply(self.resistance,7);
    return true;
}
std::uint16_t EfficacyMask(const fates::runtime::native::ItemDefinition& item) noexcept {
    return static_cast<std::uint16_t>(item.effective_damage[0]) |
           static_cast<std::uint16_t>(static_cast<std::uint16_t>(item.effective_damage[1])<<8);
}

BattlePreviewSide Preview(const fates::runtime::native::NativeRuntime& r,
                          const fates::runtime::native::UnitState& self,
                          const SideResolved& side,
                          const fates::runtime::native::UnitState& /*opponent*/,
                          const SideResolved& opp,
                          int distance, const AroundSkillProjection& around,
                          const fates::support::native::LocalSupportProjection& support, bool support_bound) {
    BattlePreviewSide p{};
    p.around=around;
    p.local_support_context_applied=support_bound;p.local_support=support;
    const auto support_bonus=support.bonuses;
    p.has_attack_weapon=side.item!=nullptr;
    p.equipped_item_id=side.item?side.item->id:0;
    p.condition_flags=side.condition_flags;p.terrain_bonus_applied=side.terrain_applies;p.unit_category_mask=side.category_mask;
    // CalculateDetail only enters the interaction lookup when BOTH item IDs
    // are nonzero. The attacker's ordinary rank bonus still applies at delta 0.
    const auto delta=side.item && opp.item
        ? r.definitions.WeaponInteractionDelta(side.weapon_group,opp.weapon_group) : 0;
    const bool include_rank=delta>=0;
    const int rank_mt=include_rank?r.definitions.WeaponRankMightBonus(side.weapon_group,side.effective_weapon_exp):0;
    const int rank_hit=include_rank?r.definitions.WeaponRankHitBonus(side.weapon_group,side.effective_weapon_exp):0;
    const int interaction_rank=delta<0?opp.rank:side.rank;
    const int interaction_mt=delta==0?0:r.definitions.WeaponInteractionMight(static_cast<std::uint8_t>(interaction_rank))*delta;
    const int interaction_hit=delta==0?0:r.definitions.WeaponInteractionHit(static_cast<std::uint8_t>(interaction_rank))*delta;

    const auto efficacy=ResolveEfficacyFlags({
        true,side.item!=nullptr,0,false,opp.category_mask,0,side.item?EfficacyMask(*side.item):std::uint16_t(0),false
    });
    p.effective=(efficacy.sideFlags & kSideFlagEffective)!=0;
    p.uses_magic=side.magic;

    const int offensive_stat=side.magic?static_cast<int>(self.magic):static_cast<int>(self.strength);
    const int item_might=side.item?side.item_values.power:0;
    const int base_attack=std::max(0,offensive_stat+item_might+rank_mt+interaction_mt);
    p.attack=ResolveDetailAttack({
        true,true,side.item!=nullptr,false,efficacy.sideFlags,base_attack,0,0,
        item_might,false,false,false,false,false,around.modifiers.attack_addend,0,0
    });
    const auto base_hit=static_cast<std::int16_t>(std::max(0,
        static_cast<int>(side.job->hit)+(side.item?side.item_values.hit:0)+
        ((static_cast<int>(self.skill)*3+static_cast<int>(self.luck))>>1)+rank_hit+interaction_hit));
    p.hit=ResolveDetailHit({true,true,side.item!=nullptr,false,base_hit,0,0,false,0,support_bonus.hit,0,efficacy.sideFlags});
    const auto base_avoid=WrapSigned16(std::max(0,static_cast<int>(side.job->avoid)+
        ((static_cast<int>(self.speed)*3+static_cast<int>(self.luck))>>1)+
        (side.item?static_cast<int>(side.item->avoid):0)));
    p.avoid=ResolveDetailAvoid({true,true,base_avoid,side.terrain_applies,side.terrain->avoid_bonus,
        around.modifiers.avoid_addend,support_bonus.avoid,0});
    p.critical=side.item?std::max(0,static_cast<int>(side.job->crit)+side.item_values.critical+
        (std::max(static_cast<int>(self.skill)-4,0)>>1)):0;
    DetailTailInput dodge{};dodge.selfPresent=dodge.opponentPresent=true;
    dodge.baseSecure=WrapSigned16(static_cast<int>(side.job->dodge)+(static_cast<int>(self.luck)>>1)+(side.item?static_cast<int>(side.item->dodge):0));
    dodge.secureModifier=around.modifiers.dodge_addend;
    dodge.secureBaseAddend=support_bonus.dodge;
    if(support_bound) {
        DetailTailInput critical{};critical.selfPresent=critical.opponentPresent=critical.primaryLane=true;
        critical.equippedItemPresent=side.item!=nullptr;critical.baseCritical=WrapSigned16(p.critical);
        critical.criticalBaseAddend=support_bonus.critical;p.critical=ResolveDetailTail(critical).critical;
    }
    p.dodge=ResolveDetailTail(dodge).secure;
    const auto defenses=ResolveDetailDefense({true,true,self.defense,self.resistance,around.modifiers.defense_resistance_addend,0,around.modifiers.resistance_addend,0,0,side.terrain_applies,side.terrain->defense_bonus});
    p.defense=defenses.defense+defenses.defenseBonus;
    p.resistance=defenses.resistance+defenses.resistanceBonus;
    p.continuous=std::max(0,static_cast<int>(self.speed)+(side.item?static_cast<int>(side.item->effective_speed_player):0));
    p.under_continuous=std::max(0,static_cast<int>(self.speed)+(side.item?static_cast<int>(side.item->effective_speed_enemy):0));
    p.simple_hit=Clamp100(p.hit);
    p.simple_critical=Clamp100(p.critical);
    p.simple_damage=0;
    p.attack_count=(side.item && distance>=side.item->min_range && distance<=side.item->max_range)?1:0;
    return p;
}

void CompletePreview(BattlePreviewSide& attacker,
                     const BattlePreviewSide& defender,
                     int distance,
                     const fates::runtime::native::ItemDefinition* item) {
    // GetSimplePower/GetSimpleHit/GetSimpleTimes and CalculateSimple's critical
    // lane return zero for item ID 0. Keep defenses and both speed lanes intact.
    if(!item) {
        attacker.simple_hit=attacker.simple_critical=attacker.simple_damage=0;
        attacker.attack_count=0;
        return;
    }
    attacker.simple_hit=Clamp100(attacker.hit-defender.avoid);
    attacker.simple_critical=ResolveOrdinarySimpleCriticalChance(
        attacker.critical,defender.dodge,(item->bitflags[3]&0x08u)!=0);
    const int mitigation=attacker.uses_magic?defender.resistance:defender.defense;
    attacker.simple_damage=std::max(0,attacker.attack-mitigation);
    if(distance<item->min_range || distance>item->max_range) attacker.attack_count=0;
    else attacker.attack_count=static_cast<std::uint8_t>(
        attacker.continuous>=defender.under_continuous+5?2:1);
}

std::uint8_t CombinedGuardProgress(const fates::runtime::native::UnitState& lead,
                                   const fates::runtime::native::UnitState* partner) noexcept {
    const unsigned total=static_cast<unsigned>(lead.pair.guard_progress)+
                         (partner?static_cast<unsigned>(partner->pair.guard_progress):0u);
    return static_cast<std::uint8_t>(std::min(total,255u));
}

bool ResolvePairMembers(fates::runtime::native::NativeRuntime& r,
                        const std::uint16_t lead_slot,
                        fates::runtime::native::UnitState*& lead,
                        fates::runtime::native::UnitState*& partner) noexcept {
    using fates::runtime::native::PairRole;
    if(lead_slot>=r.game.units.size()) return false;
    lead=&r.game.units[lead_slot];
    partner=nullptr;
    if(!lead->pair.bound) return true;
    if(lead->pair.role!=PairRole::Lead || lead->pair.partner_slot>=r.game.units.size()) return false;
    partner=&r.game.units[lead->pair.partner_slot];
    return partner->occupied && !partner->defeated && partner->pair.bound &&
           partner->pair.role==PairRole::Partner && partner->pair.partner_slot==lead_slot;
}

void AdvanceIncomingPairProgress(fates::runtime::native::UnitState& lead,
                                 fates::runtime::native::UnitState& partner) noexcept {
    // B007 paired Faceless have no Guard Gauge acceleration skill. Retail advances
    // both unit-owned progress bytes after a non-Guard incoming strike.
    if(lead.pair.guard_progress<255u) ++lead.pair.guard_progress;
    if(partner.pair.guard_progress<255u) ++partner.pair.guard_progress;
}

void AdvanceActingPairProgress(fates::runtime::native::UnitState& lead,
                               fates::runtime::native::UnitState& partner) noexcept {
    // Retail keeps two separate progress bytes. Below combined 9 both advance; at
    // combined 9 exactly the lower/equal partner byte advances so the sum reaches 10.
    const unsigned total=static_cast<unsigned>(lead.pair.guard_progress)+partner.pair.guard_progress;
    if(total<9u) {
        if(lead.pair.guard_progress<255u) ++lead.pair.guard_progress;
        if(partner.pair.guard_progress<255u) ++partner.pair.guard_progress;
    } else if(total<10u) {
        if(partner.pair.guard_progress<=lead.pair.guard_progress) {
            if(partner.pair.guard_progress<255u) ++partner.pair.guard_progress;
        } else if(lead.pair.guard_progress<255u) ++lead.pair.guard_progress;
    }
}

BattleStrike Strike(fates::runtime::native::NativeRuntime& r,
                    std::uint16_t actor,
                    std::uint16_t target,
                    const BattlePreviewSide& preview) {
    auto& t=r.game.units[target];
    fates::runtime::native::UnitState *actor_lead=nullptr,*actor_partner=nullptr;
    fates::runtime::native::UnitState *target_lead=nullptr,*target_partner=nullptr;
    (void)ResolvePairMembers(r,actor,actor_lead,actor_partner);
    (void)ResolvePairMembers(r,target,target_lead,target_partner);
    BattleStrike strike{};
    strike.actor_slot=actor;
    strike.target_slot=target;
    strike.hp_before=t.current_hp;
    strike.target_guard_progress_before=target_lead?CombinedGuardProgress(*target_lead,target_partner):0;
    strike.actor_guard_progress_before=actor_lead?CombinedGuardProgress(*actor_lead,actor_partner):0;
    strike.guard_stance_intercepted=target_partner && strike.target_guard_progress_before>=10u;
    strike.hit_threshold=RetailHybridHitThreshold(preview.simple_hit);
    strike.hit_roll=fates::runtime::native::DrawGameRandom(r.game,10000);
    strike.hit=strike.hit_roll<strike.hit_threshold;
    if(strike.hit) {
        strike.critical_roll_consumed=true;
        strike.critical_roll=fates::runtime::native::DrawGameRandom(r.game,100);
        strike.critical=strike.critical_roll<static_cast<std::uint32_t>(preview.simple_critical);
        if(!strike.guard_stance_intercepted) {
            strike.damage=preview.simple_damage*(strike.critical?3:1);
            t.current_hp=static_cast<std::int16_t>(std::max(0,static_cast<int>(t.current_hp)-strike.damage));
            if(t.current_hp<1) { t.defeated=true; strike.defeated=true; }
        }
    }
    if(target_partner) {
        if(strike.guard_stance_intercepted) {
            target_lead->pair.guard_progress=0;
            target_partner->pair.guard_progress=0;
        } else {
            AdvanceIncomingPairProgress(*target_lead,*target_partner);
        }
    }
    if(actor_partner) AdvanceActingPairProgress(*actor_lead,*actor_partner);
    strike.target_guard_progress_after=target_lead?CombinedGuardProgress(*target_lead,target_partner):0;
    strike.actor_guard_progress_after=actor_lead?CombinedGuardProgress(*actor_lead,actor_partner):0;
    strike.hp_after=t.current_hp;
    return strike;
}

bool ReleasePartnerAfterLeadDefeat(fates::runtime::native::NativeRuntime& r,
                                       const std::uint16_t lead_slot,
                                       std::vector<PairSeparationRecord>& records) noexcept {
    using fates::runtime::native::PairRole;
    if(lead_slot>=r.game.units.size()) return false;
    auto& lead=r.game.units[lead_slot];
    if(!lead.defeated || !lead.pair.bound) return true;
    if(lead.pair.role!=PairRole::Lead || lead.pair.partner_slot>=r.game.units.size()) return false;
    const auto partner_slot=lead.pair.partner_slot;
    auto& partner=r.game.units[partner_slot];
    if(!partner.occupied || partner.defeated || !partner.pair.bound ||
       partner.pair.role!=PairRole::Partner || partner.pair.partner_slot!=lead_slot) return false;

    // Retail Unit::DoubleOff clears the reciprocal relationship. SequenceBattle::DeadActionEnd
    // then re-materializes the surviving partner using GetRescuePosition from the defeated
    // lead's tile. In the B007 call shape the origin tile is legal (param6=1); the defeated
    // lead no longer occupies the tactical image, so the partner remains at those coordinates.
    const auto x=lead.x, y=lead.y;
    if(fates::runtime::native::UnlinkUnitPair(r.game,lead_slot)!=fates::runtime::native::UnitPairStatus::Ok)return false;
    lead.has_position=false;
    partner.has_position=true;
    partner.x=x; partner.y=y;
    records.push_back({lead_slot,partner_slot,x,y});
    return true;
}

// Passive local-aura/ordinary-item subset only. Offense proc, status,
// absorption, source postcombat, paired Guard Stance and Brave ownership are
// deliberately not inferred from a mechanically valid equipment ID.
bool AssistParticipantsSupported(const fates::runtime::native::NativeRuntime& r,
    const fates::runtime::native::UnitState& u,std::uint16_t slot,bool calculation_pairs=false) {
    using fates::runtime::native::PairRole;
    if(!u.occupied || !u.has_position || !u.combat_state_valid || u.complex_battle_rules ||
       u.defeated || u.current_hp<=0)return false;
    if(calculation_pairs) {
        if((u.flags&0x40000u) || !PairFactsValid(r,slot))return false;
        if(u.x<0 || u.y<0 || u.x>127 || u.y>127)return false;
        if(std::any_of(u.weakness.begin(),u.weakness.end(),[](auto v){return v!=0;}))return false;
        for(auto value:{u.strength,u.magic,u.skill,u.speed,u.luck,u.defense,u.resistance})
            if(value<0 || value>99)return false;
    }
    else if(u.pair.bound || u.pair.role!=PairRole::None)return false;
    const auto* person=r.definitions.FindPerson(u.person_id);
    const auto* job=r.definitions.FindJob(u.job_id);
    if(!person || !job)return false;
    auto passive=[](std::uint16_t id){return id==0 || IsSupportedLocalAroundSkill(id);};
    for(auto id:u.equipped_skill_ids)if(!passive(id))return false;
    for(auto id:person->personal_skills)if(!passive(id))return false;
    if(u.equipped_item_id) {
        const auto* item=r.definitions.FindItem(u.equipped_item_id);
        if(!item || !SupportedItem(r.definitions,*item) || item->bitflags[4]!=0)return false;
    }
    return true;
}

BattleTransactionStatus BuildAssistPreviews(const fates::runtime::native::NativeRuntime& r,
    std::uint16_t attacker_slot,std::uint16_t defender_slot,
    const fates::runtime::native::UnitState& attacker,
    const fates::runtime::native::UnitState& defender,
    bool physical_only,BattlePreviewResult& out,const BattlePreparationState& conditions,const ForcedSourceContext* forced=nullptr,const BattleItems* items=nullptr) {
    if(!out.attacker.local_support.selected_has_weapon && !out.defender.local_support.selected_has_weapon)
        return BattleTransactionStatus::Ok;
    if(!AssistParticipantsSupported(r,attacker,attacker_slot,forced!=nullptr)||!AssistParticipantsSupported(r,defender,defender_slot,forced!=nullptr))
        return BattleTransactionStatus::UnsupportedAttackStance;
    // Reify ONLY the hypothetical position in a private image. This prevents
    // the old attacker cell from becoming a spurious third-party aura source
    // when the opponent-versus-assist defensive lane is calculated.
    auto image=std::make_unique<fates::runtime::native::NativeRuntime>(r);
    image->game.units[attacker_slot]=attacker;
    image->game.units[defender_slot]=defender;
    const std::array<std::uint16_t,2> primaries{{attacker_slot,defender_slot}};
    const std::array<BattlePreviewSide*,2> sides{{&out.attacker,&out.defender}};
    for(std::size_t i=0;i<2;++i) {
        const auto& local=sides[i]->local_support;
        if(!local.selected_has_weapon)continue;
        auto& assist=out.assists[i];assist.present=true;
        assist.unit_slot=local.selection.selected;assist.primary_slot=primaries[i];
        if(assist.unit_slot>=r.game.units.size() || assist.unit_slot==attacker_slot || assist.unit_slot==defender_slot)
            return BattleTransactionStatus::UnsupportedAttackStance;
        auto source=image->game.units[assist.unit_slot];
        if(const auto* item=OverrideItem(items,i+2))source.equipped_item_id=item->item_id;
        if(forced){source.x=std::int16_t(forced->conditions.view.sides[i+2].x);source.y=std::int16_t(forced->conditions.view.sides[i+2].y);}
        const auto target_slot=primaries[1-i];const auto& target=image->game.units[target_slot];
        if(!AssistParticipantsSupported(*image,source,assist.unit_slot,forced!=nullptr))return BattleTransactionStatus::UnsupportedAttackStance;
        BattlePreviewSide source_pair_meta{};
        if(forced && !ApplyPairCapabilityProjection(*image,assist.unit_slot,source,source_pair_meta,true))
            return BattleTransactionStatus::UnsupportedPairProjection;
        SideResolved sr,tr;
        const auto target_conditions=conditions.Side(std::uint8_t(1-i));
        const auto source_conditions=forced?std::optional(forced->conditions.Side(std::uint8_t(i+2))):std::nullopt;
        if(!Resolve(*image,source,sr,false,OverrideItem(items,i+2),i==0?3u:2u,source_conditions?&*source_conditions:nullptr)||!Resolve(*image,target,tr,true,OverrideItem(items,1-i),i==0?0u:1u,&target_conditions))return BattleTransactionStatus::UnsupportedItem;
        if(physical_only && sr.magic)return BattleTransactionStatus::UnsupportedItem;
        // Damage-reducing terrain and other extra detail owners remain outside
        // this ordinary assist subset instead of applying an incomplete half.
        if((sr.terrain->flags_0x18|tr.terrain->flags_0x18)&0x600000u)
            return BattleTransactionStatus::UnsupportedAttackStance;
        const auto around=forced?ProjectBattleAroundSkills:ProjectLocalAroundSkills;
        auto source_around=around(*image,assist.unit_slot,target_slot,
            source.x,source.y,target.x,target.y);
        auto target_around=around(*image,target_slot,assist.unit_slot,
            target.x,target.y,source.x,source.y);
        if(source_around.status!=AroundProjectionStatus::Ok || target_around.status!=AroundProjectionStatus::Ok)
            return BattleTransactionStatus::UnsupportedAroundProjection;
        // CalculateDetail does not run DualSupportCalculator on a side marked
        // as assist. The opposing PRIMARY does keep its own support bonuses.
        fates::support::native::LocalSupportProjection no_support{};
        auto& p=assist.side;
        p=Preview(*image,source,sr,target,tr,out.distance,source_around,no_support,false);
        p.guard_stance_pair=source_pair_meta.guard_stance_pair;p.pair_partner_slot=source_pair_meta.pair_partner_slot;
        p.pair_capability_bonus=source_pair_meta.pair_capability_bonus;p.guard_progress_sum=source_pair_meta.guard_progress_sum;
        const auto target_lane=Preview(*image,target,tr,source,sr,out.distance,
            target_around,sides[1-i]->local_support,true);
        assist.opposing_avoid=target_lane.avoid;assist.opposing_dodge=target_lane.dodge;
        assist.opposing_defense=target_lane.defense;assist.opposing_resistance=target_lane.resistance;
        p.simple_hit=Clamp100(p.hit-target_lane.avoid);
        p.simple_critical=ResolveOrdinarySimpleCriticalChance(p.critical,target_lane.dodge,(sr.item->bitflags[3]&8u)!=0);
        const auto mitigation=p.uses_magic?target_lane.resistance:target_lane.defense;
        p.simple_damage=ResolveAssistPowerExact(p.attack-mitigation);
        p.simple_damage_rate=ResolveSimpleDamageRate(true,2u,false,false);
        // Assist flag bypasses source-to-target range and caps its own count
        // at one. CalculateDual then gates it on the PRIMARY's attack count.
        const auto source_count=ResolveAssistTimesExact(true,false,i==0?3u:2u,i==0?0u:1u,0u);
        const auto& primary=image->game.units[primaries[i]];
        const bool own_pair=forced && primary.pair.bound && primary.pair.partner_slot==assist.unit_slot;
        // The admitted passive skill set excludes the paired-Dual enabling skill.
        const auto dual=ResolveDualAttackState({true,own_pair,primary.pair.guard_progress,source.pair.guard_progress,false,i==0?1u:0u,
            sides[i]->attack_count,source_count});
        p.attack_count=std::uint8_t(dual.partnerSimpleAttackCount);
        assist.active=p.attack_count>0;
    }
    return BattleTransactionStatus::Ok;
}

BattlePreviewResult BuildPreviewImpl(const fates::runtime::native::NativeRuntime& r,
                                     std::uint16_t attacker_slot,
                                     std::uint16_t defender_slot,
                                     bool physical_only,
                                     bool override_position,
                                     std::int16_t attacker_x,
                                     std::int16_t attacker_y,const ForcedSourceContext* forced=nullptr,const BattleItems* items=nullptr) {
    BattlePreviewResult out{};
    if(attacker_slot>=r.game.units.size() || defender_slot>=r.game.units.size() || attacker_slot==defender_slot) return out;
    auto attacker=r.game.units[attacker_slot];
    auto defender=r.game.units[defender_slot];
    if(const auto* item=OverrideItem(items,0))attacker.equipped_item_id=item->item_id;
    if(const auto* item=OverrideItem(items,1))defender.equipped_item_id=item->item_id;
    if(override_position) { attacker.has_position=true; attacker.x=attacker_x; attacker.y=attacker_y; }
    if(!attacker.occupied || !defender.occupied || !fates::runtime::native::IsTacticalForce(attacker.force_type) || !fates::runtime::native::IsTacticalForce(defender.force_type)) return out;
    if(attacker.pair.role==fates::runtime::native::PairRole::Partner ||
       defender.pair.role==fates::runtime::native::PairRole::Partner) {
        out.status=BattleTransactionStatus::UnsupportedPairProjection; return out;
    }
    if(LocalAuraForcesAllied(attacker.force_type,defender.force_type)) { out.status=BattleTransactionStatus::AlliedTarget; return out; }
    if(!attacker.combat_state_valid || !defender.combat_state_valid || attacker.defeated || defender.defeated) {
        out.status=BattleTransactionStatus::MissingCombatState; return out;
    }
    if(attacker.complex_battle_rules || defender.complex_battle_rules ||
       !PostCombatSkillSetSupported(attacker) || !PostCombatSkillSetSupported(defender)) {
        out.status=BattleTransactionStatus::UnsupportedComplexRules; return out;
    }
    BattlePreviewSide attacker_pair_meta{}, defender_pair_meta{};
    if(!ApplyPairCapabilityProjection(r,attacker_slot,attacker,attacker_pair_meta,forced!=nullptr) ||
       !ApplyPairCapabilityProjection(r,defender_slot,defender,defender_pair_meta,forced!=nullptr)) {
        out.status=BattleTransactionStatus::UnsupportedPairProjection; return out;
    }
    BattlePreparationState condition_input{};
    condition_input.view.sides[0].unit=attacker_slot;condition_input.view.sides[0].x=attacker.x;condition_input.view.sides[0].y=attacker.y;
    condition_input.view.sides[1].unit=defender_slot;condition_input.view.sides[1].x=defender.x;condition_input.view.sides[1].y=defender.y;
    const auto prepared=forced?BattlePreparationResult{BattlePreparationStatus::Ok,forced->conditions}:
        ProjectBattleCalculationConditions(r,condition_input,items?*items:BattleItems{});
    if(prepared.status!=BattlePreparationStatus::Ok){out.status=BattleTransactionStatus::UnsupportedItem;return out;}
    const auto actor_conditions=prepared.state.Side(0),defender_conditions=prepared.state.Side(1);
    SideResolved ar,dr;
    if(!Resolve(r,attacker,ar,false,OverrideItem(items,0),1,&actor_conditions) || !Resolve(r,defender,dr,true,OverrideItem(items,1),0,&defender_conditions)) { out.status=BattleTransactionStatus::UnsupportedItem; return out; }
    if(physical_only && (ar.magic || dr.magic)) { out.status=BattleTransactionStatus::UnsupportedItem; return out; }
    const int distance=prepared.state.distance;
    out.movement_rule_byte=prepared.state.movement_rule_byte;
    out.distance=static_cast<std::uint8_t>(std::clamp(distance,0,255));
    const auto around=forced?ProjectBattleAroundSkills:ProjectLocalAroundSkills;
    auto attacker_around=around(r,attacker_slot,defender_slot,
        attacker.x,attacker.y,defender.x,defender.y);
    auto defender_around=around(r,defender_slot,attacker_slot,
        defender.x,defender.y,attacker.x,attacker.y);
    if(attacker_around.status!=AroundProjectionStatus::Ok || defender_around.status!=AroundProjectionStatus::Ok) {
        out.attacker.around=attacker_around; out.defender.around=defender_around;
        out.status=BattleTransactionStatus::UnsupportedAroundProjection; return out;
    }
    fates::support::native::LocalSupportProjection attacker_support{},defender_support{};
    const bool support_bound=r.game.support_context.bound;
    if(support_bound) {
        attacker_support=forced?forced->support[0]:fates::support::native::ProjectLocalSupportBonuses(r,attacker_slot,attacker.x,attacker.y,attacker_slot);
        defender_support=forced?forced->support[1]:fates::support::native::ProjectLocalSupportBonuses(r,defender_slot,defender.x,defender.y,attacker_slot);
        out.attacker.local_support_context_applied=out.defender.local_support_context_applied=true;
        out.attacker.local_support=attacker_support;out.defender.local_support=defender_support;
        if(attacker_support.status!=fates::support::native::LocalSupportStatus::Ok ||
           defender_support.status!=fates::support::native::LocalSupportStatus::Ok) {
            out.status=BattleTransactionStatus::UnsupportedLocalSupportProjection;return out;
        }
        // Armed sources are projected below AFTER primary detail/counts. The
        // AI score adapter must still reject unowned four-side indication.
    }
    out.attacker=Preview(r,attacker,ar,defender,dr,distance,attacker_around,attacker_support,support_bound);
    out.defender=Preview(r,defender,dr,attacker,ar,distance,defender_around,defender_support,support_bound);
    out.attacker.guard_stance_pair=attacker_pair_meta.guard_stance_pair;
    out.attacker.pair_partner_slot=attacker_pair_meta.pair_partner_slot;
    out.attacker.guard_progress_sum=attacker_pair_meta.guard_progress_sum;
    out.attacker.pair_capability_bonus=attacker_pair_meta.pair_capability_bonus;
    out.defender.guard_stance_pair=defender_pair_meta.guard_stance_pair;
    out.defender.pair_partner_slot=defender_pair_meta.pair_partner_slot;
    out.defender.guard_progress_sum=defender_pair_meta.guard_progress_sum;
    out.defender.pair_capability_bonus=defender_pair_meta.pair_capability_bonus;
    CompletePreview(out.attacker,out.defender,distance,ar.item);
    CompletePreview(out.defender,out.attacker,distance,dr.item);
    // Retail CalculateSimple +0x38: supported ordinary items cannot carry
    // ISID_必殺倍率＋ because SupportedItem rejects byte3 effects, so the
    // ordinary base is 300. Dragonskin halves the opposing side's rate.
    out.attacker.simple_damage_rate=ResolveSimpleDamageRate(
        ar.item!=nullptr,0u,false,UnitHasSkill(defender,kSkillDragonskin));
    out.defender.simple_damage_rate=ResolveSimpleDamageRate(
        dr.item!=nullptr,0u,false,UnitHasSkill(attacker,kSkillDragonskin));
    if(UnitHasSkill(attacker,kSkillWaryFighter) || UnitHasSkill(defender,kSkillWaryFighter)) {
        if(out.attacker.attack_count>1) out.attacker.attack_count=1;
        if(out.defender.attack_count>1) out.defender.attack_count=1;
    }
    if(!forced && out.attacker.attack_count==0) { out.status=BattleTransactionStatus::OutOfRange; return out; }
    out.status=BuildAssistPreviews(r,attacker_slot,defender_slot,attacker,defender,physical_only,out,prepared.state,forced,items);
    if(out.status!=BattleTransactionStatus::Ok)return out;
    out.status=BattleTransactionStatus::Ok;
    return out;
}

std::optional<fates::runtime::native::UnitItemState> SelectCalculationItem(
    const fates::runtime::native::NativeRuntime& r,const fates::runtime::native::UnitState& u,std::uint8_t index) {
    namespace rn=fates::runtime::native;
    if(index>=5)return std::nullopt;
    const auto inventory=rn::ReadCalculationInventory(u);if(!inventory)return std::nullopt;
    const auto instance=(*inventory)[index];
    const auto* p=r.definitions.FindPerson(u.person_id);const auto* j=r.definitions.FindJob(u.job_id);
    const auto* item=r.definitions.FindItem(instance.item_id);
    if(!instance.item_id || !p || !j || !item)return std::nullopt;
    if(rn::ProjectCurrentItemEligibility(r,u,*item,false,true)!=rn::CurrentItemEligibility::Yes)return std::nullopt;
    return instance;
}

BattleTransactionResult ExecuteImpl(fates::runtime::native::NativeRuntime& r,
                                    std::uint16_t attacker_slot,
                                    std::uint16_t defender_slot,
                                    bool physical_only) {
    BattleTransactionResult out{};
    const auto preview=BuildPreviewImpl(r,attacker_slot,defender_slot,physical_only,false,0,0);
    out.status=preview.status;
    out.distance=preview.distance;
    out.attacker=preview.attacker;
    out.defender=preview.defender;
    out.assists=preview.assists;
    if(preview.status!=BattleTransactionStatus::Ok) return out;
    auto& attacker=r.game.units[attacker_slot];
    auto& defender=r.game.units[defender_slot];
    if(!r.game.rng.game_state.initialized) { out.status=BattleTransactionStatus::MissingGameRngState; return out; }
    const auto before=r.game.rng.game;
    const auto schedule=BuildResolvedAttackScheduleExact(
        {{out.attacker.attack_count,out.defender.attack_count,
          out.assists[0].side.attack_count,out.assists[1].side.attack_count}},{{0,0,0,0}},false);
    for(std::uint8_t n=0;n<schedule.count;++n) {
        // Retail CalculateAttackSingle terminates the exchange at primary
        // defeat. No stale assist/counter/follow-up rolls survive that boundary.
        if(attacker.defeated || defender.defeated)break;
        const auto side=schedule.sides[n];const bool support=side>=2;
        const auto primary=std::uint8_t(side&1u);
        const auto actor=support?out.assists[primary].unit_slot:(primary?defender_slot:attacker_slot);
        const auto target=primary?attacker_slot:defender_slot;
        const auto& p=support?out.assists[primary].side:(primary?out.defender:out.attacker);
        auto strike=Strike(r,actor,target,p);strike.support_strike=support;strike.battle_side=side;
        out.strikes.push_back(strike);
    }
    ApplyBattlePostCombatEffects(r,attacker_slot,defender_slot,out.distance,out.post_combat);
    if(!ReleasePartnerAfterLeadDefeat(r,defender_slot,out.pair_separations) ||
       !ReleasePartnerAfterLeadDefeat(r,attacker_slot,out.pair_separations)) {
        out.status=BattleTransactionStatus::UnsupportedPairDefeatResolution;
        out.game_rng_draws=r.game.rng.game-before;
        return out;
    }
    attacker.action_committed=true;
    out.game_rng_draws=r.game.rng.game-before;
    out.status=BattleTransactionStatus::Ok;
    return out;
}
}

std::uint32_t RetailHybridHitThreshold(const std::int32_t hit) noexcept {
    return kHybridHitThreshold[static_cast<std::size_t>(std::clamp(hit,0,100))];
}

BattlePreviewResult PreviewOrdinaryBattle(const fates::runtime::native::NativeRuntime& r,
                                          const std::uint16_t attacker_slot,
                                          const std::uint16_t defender_slot) {
    return BuildPreviewImpl(r,attacker_slot,defender_slot,false,false,0,0);
}

BattlePreviewResult PreviewOrdinaryBattleAt(const fates::runtime::native::NativeRuntime& r,
                                            const std::uint16_t attacker_slot,
                                            const std::uint16_t defender_slot,
                                            const std::int16_t attacker_x,
                                            const std::int16_t attacker_y) {
    return BuildPreviewImpl(r,attacker_slot,defender_slot,false,true,attacker_x,attacker_y);
}

BattlePreviewResult PreviewOrdinaryBattleAtInventory(
    const fates::runtime::native::NativeRuntime& r,std::uint16_t actor,std::uint16_t target,
    std::int16_t x,std::int16_t y,std::uint8_t index) {
    BattlePreviewResult out{};
    if(actor>=r.game.units.size())return out;
    BattleItems items{};items[0]=SelectCalculationItem(r,r.game.units[actor],index);
    if(!items[0]){out.status=BattleTransactionStatus::UnsupportedItem;return out;}
    return BuildPreviewImpl(r,actor,target,false,true,x,y,nullptr,&items);
}

BattlePreviewResult PreviewBattleWithForcedSource(
    const fates::runtime::native::NativeRuntime& runtime,const ForcedSourceBattleRequest& req) {
    namespace rn=fates::runtime::native;namespace sn=fates::support::native;
    BattlePreviewResult out{};
    auto fail=[&](BattleTransactionStatus status){out.status=status;return out;};
    if(req.attacker>=runtime.game.units.size() || req.defender>=runtime.game.units.size() ||
       req.source>=runtime.game.units.size() || req.attacker==req.defender ||
       req.source==req.attacker || req.source==req.defender)return out;
    if(req.attacker_item_index>=5 || req.source_item_index>=5)return fail(BattleTransactionStatus::UnsupportedItem);
    if(!runtime.game.support_context.bound)return fail(BattleTransactionStatus::UnsupportedLocalSupportProjection);
    auto image=std::make_unique<rn::NativeRuntime>(runtime);
    for(const auto slot:{req.attacker,req.defender,req.source}) {
        const auto& u=image->game.units[slot];
        if(!u.occupied || !u.has_position || u.x<0 || u.y<0 || u.x>127 || u.y>127)return out;
        if(!PairFactsValid(*image,slot))return fail(BattleTransactionStatus::UnsupportedPairProjection);
        if(u.flags&0x40000u)return fail(BattleTransactionStatus::UnsupportedComplexRules);
        // These are resolved ordinary base capabilities; malformed synthetic
        // values are not silently turned into a current original Unit state.
        for(auto value:{u.strength,u.magic,u.skill,u.speed,u.luck,u.defense,u.resistance})
            if(value<0 || value>99)return fail(BattleTransactionStatus::UnsupportedComplexRules);
    }
    auto& actor=image->game.units[req.attacker];auto& source=image->game.units[req.source];
    if(source.force_type!=actor.force_type)return fail(BattleTransactionStatus::UnsupportedAttackStance);
    BattleItems items{};
    items[0]=SelectCalculationItem(*image,actor,req.attacker_item_index);
    items[2]=SelectCalculationItem(*image,source,req.source_item_index);
    if(!items[0] || !items[2])return fail(BattleTransactionStatus::UnsupportedItem);
    actor.equipped_item_id=items[0]->item_id;source.equipped_item_id=items[2]->item_id;
    ForcedSourceContext context{};
    context.support[0]=sn::ProjectSpecifiedSupportBonuses(*image,req.attacker,req.source,req.attack_x,req.attack_y,req.attacker);
    const auto& target=image->game.units[req.defender];
    context.support[1]=sn::ProjectBattleSupportBonuses(*image,req.defender,target.x,target.y,req.attacker);
    if(context.support[0].status!=sn::LocalSupportStatus::Ok || context.support[1].status!=sn::LocalSupportStatus::Ok) {
        out.attacker.local_support=context.support[0];out.defender.local_support=context.support[1];
        return fail(BattleTransactionStatus::UnsupportedLocalSupportProjection);
    }
    // Validate every actual participant before narrowing original signed-byte
    // coordinates or projecting pair stats. A paired primary's coordinate
    // override must not disguise unsupported state on an automatic source.
    for(const auto slot:{req.attacker,req.defender,req.source,context.support[1].selection.selected}) {
        if(slot==sn::kNoSupportUnit)continue;
        if(slot>=image->game.units.size() || !AssistParticipantsSupported(*image,image->game.units[slot],slot,true))
            return fail(BattleTransactionStatus::UnsupportedAttackStance);
    }
    auto& view=context.conditions.view;view.units.resize(image->game.units.size());
    for(std::uint16_t i=0;i<image->game.units.size();++i) {
        const auto& u=image->game.units[i];auto& facts=view.units[i];facts.public_flags=u.flags;
        if(u.occupied && u.pair.bound) {
            if(!PairFactsValid(*image,i))return fail(BattleTransactionStatus::UnsupportedPairProjection);
            // Native pair roles are the semantic owner of original public2/4.
            const auto role=u.pair.role==rn::PairRole::Lead?2u:4u;
            if((u.flags&6u) && (u.flags&6u)!=role)return fail(BattleTransactionStatus::UnsupportedPairProjection);
            facts.public_flags=(u.flags&~6u)|role;facts.partner=u.pair.partner_slot;
        } else if(u.occupied && ((u.flags&6u)||u.pair.role!=rn::PairRole::None))
            return fail(BattleTransactionStatus::UnsupportedPairProjection);
        facts.x=std::int8_t(u.x);facts.y=std::int8_t(u.y);
    }
    view.sides[0].unit=req.attacker;view.sides[0].source=req.source;view.sides[0].flags=1;
    view.sides[0].x=req.attack_x;view.sides[0].y=req.attack_y;view.sides[0].requested_item_index=req.attacker_item_index;
    view.sides[1].unit=req.defender;view.sides[1].x=target.x;view.sides[1].y=target.y;
    view.sides[2].flags=3;view.sides[2].requested_item_index=req.source_item_index;view.sides[3].flags=2;
    struct Services final:BattleDualPreparationServices {
        const ForcedSourceContext& context;
        explicit Services(const ForcedSourceContext& c):context(c){}
        std::optional<std::uint16_t> SelectLocalSource(const BattleDualView&,std::uint8_t primary) override {
            return context.support[primary].selection.selected;
        }
        std::optional<std::int32_t> RelianceLevel(const BattleDualView&,std::uint8_t primary,std::uint16_t source) override {
            const auto& selection=context.support[primary].selection;
            if(selection.selected!=source)return std::nullopt;
            return selection.reliance_level;
        }
    } services(context);
    auto primaries=ProjectBattleCalculationConditions(*image,context.conditions,items);
    if(primaries.status!=BattlePreparationStatus::Ok)return fail(BattleTransactionStatus::UnsupportedItem);
    auto prepared=ProjectBattleCalculationSources(*image,primaries.state,items,services);
    if(prepared.status!=BattlePreparationStatus::Ok)return fail(BattleTransactionStatus::UnsupportedLocalSupportProjection);
    context.conditions=std::move(prepared.state);
    const auto flags=ProjectBattleCalculationPairFlagsExact(view);
    if(flags.status!=BattleDualPreparationStatus::Ok)return fail(BattleTransactionStatus::UnsupportedPairProjection);
    for(std::uint16_t i=0;i<image->game.units.size();++i)image->game.units[i].flags=flags.during_calculation[i];
    // Outer primary conditions and complete assist conditions precede private
    // pair flags, as in BattleInfo::Calculate. Live clone identity and the
    // remaining special item/skill/world branches retain explicit admission gates.
    return BuildPreviewImpl(*image,req.attacker,req.defender,false,true,req.attack_x,req.attack_y,&context,&items);
}

BattleTransactionResult ExecuteOrdinaryBattle(fates::runtime::native::NativeRuntime& r,
                                               const std::uint16_t attacker_slot,
                                               const std::uint16_t defender_slot) {
    auto staged=std::make_unique<fates::runtime::native::NativeRuntime>(r);
    auto result=ExecuteImpl(*staged,attacker_slot,defender_slot,false);
    if(result.status==BattleTransactionStatus::Ok)r.game=std::move(staged->game);
    else {result.strikes.clear();result.post_combat.clear();result.pair_separations.clear();result.game_rng_draws=0;}
    return result;
}

BattleTransactionResult ExecuteOrdinaryPhysicalBattle(fates::runtime::native::NativeRuntime& r,
                                                       const std::uint16_t attacker_slot,
                                                       const std::uint16_t defender_slot) {
    auto staged=std::make_unique<fates::runtime::native::NativeRuntime>(r);
    auto result=ExecuteImpl(*staged,attacker_slot,defender_slot,true);
    if(result.status==BattleTransactionStatus::Ok)r.game=std::move(staged->game);
    else {result.strikes.clear();result.post_combat.clear();result.pair_separations.clear();result.game_rng_draws=0;}
    return result;
}
}
