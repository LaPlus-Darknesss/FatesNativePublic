#include "fates/runtime/native_item_inventory.hpp"
#include "fates/ai/native_ai_attack_viability.hpp"
#include "fates/ai/native_ai_shared_battle_preview.hpp"
#include "fates/battle/native_battle_postcombat.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <memory>

namespace fates::ai::native {
namespace rn=fates::runtime::native;
namespace bn=fates::battle::native;
bool IsPower0AttackExact(const AiPower0AttackFacts& f) noexcept {
    // Preserve retail precedence. Not equivalent to damage==0, nor to whether
    // any postcombat effect would eventually change the target's current HP.
    if(f.primary_power>0 && f.ordinary_outcome_probability>0.0f)return false;
    if(f.defender_attack_count>0)return true;
    if(f.poison_strike || f.grisly_wound || f.savage_blow)return false;
    int defense=0,resistance=0;
    if(f.primary_item_weakness) {
        defense=std::max(defense,int(f.primary_defense_weakness));
        resistance=std::max(resistance,int(f.primary_resistance_weakness));
    }
    if(f.partner_attack_count>0 && f.partner_item_weakness) {
        defense=std::max(defense,int(f.partner_defense_weakness));
        resistance=std::max(resistance,int(f.partner_resistance_weakness));
    }
    if(f.seal_defense)defense=std::max(defense,6);
    if(f.seal_resistance)resistance=std::max(resistance,6);
    if(f.target_status_immunity)defense=resistance=0;
    else if(f.target_status_resistance){defense>>=1;resistance>>=1;}
    if(!f.target_exists)return true;
    if(f.inevitable_end)return defense-1<=0 && resistance-1<=0;
    return int(f.target_defense_weakness)>=defense-1 &&
           int(f.target_resistance_weakness)>=resistance-1;
}
void ConsiderPlanningWeaponExact(AiPlanningWeaponChoice& out,const AiPlanningWeaponCandidate& c,
                                rn::NativeGameState& state) {
    if(c.power0 || out.missing_ai_random)return;
    if(out.selected) {
        if(out.candidate.score>c.score)return;
        if(out.candidate.score==c.score) {
            if(!state.rng.ai_state.initialized){out.missing_ai_random=true;return;}
            ++out.equal_score_draws;
            if(DrawAiLocalRandomBoundedExact(state,2u)!=0)return;
        }
    }
    out.selected=true;out.candidate=c;
}
AiPower0AttackFacts BindOrdinaryPower0(const rn::UnitState& actor,const rn::UnitState& target,
                                     const AiOrdinaryTacticalCandidateInput& candidate) {
    AiPower0AttackFacts f{};
    f.primary_power=candidate.side0.simple_power;
    f.ordinary_outcome_probability=BindOrdinaryNoSpecialBattleInfoExact(candidate.side0).indication.probabilities.residual;
    f.defender_attack_count=candidate.side1.simple_attack_count;
    f.poison_strike=bn::UnitHasSkill(actor,bn::kSkillPoisonStrike);
    f.grisly_wound=bn::UnitHasSkill(actor,bn::kSkillGrislyWound);
    f.savage_blow=bn::UnitHasSkill(actor,bn::kSkillSavageBlow);
    // Retail Power0 does not replace primary power with primary+assist damage.
    // A zero-primary attack is still evaluated in that original decision order.
    // Ordinary supported assists have no item weakness; retain the count but do
    // not invent an unowned weakness/proc branch from their extra strike.
    f.partner_attack_count=candidate.assists[0].active?
        candidate.assists[0].side.simple_attack_count:0;
    f.seal_defense=bn::UnitHasSkill(actor,bn::kSkillSealDefense);
    f.seal_resistance=bn::UnitHasSkill(actor,bn::kSkillSealResistance);
    f.target_status_immunity=bn::UnitHasSkill(target,bn::kSkillStatusImmunity);
    f.target_status_resistance=bn::UnitHasSkill(target,bn::kSkillStatusResistance);
    f.target_exists=true;
    f.inevitable_end=bn::UnitHasSkill(actor,bn::kSkillInevitableEnd);
    f.target_defense_weakness=std::int8_t(target.weakness[6]);
    f.target_resistance_weakness=std::int8_t(target.weakness[7]);
    return f;
}
namespace {
bool ViabilityProfile(const rn::NativeRuntime& r,std::uint16_t slot) {
    if(slot>=r.game.units.size())return false;
    const auto& u=r.game.units[slot];
    // No partner switch, clever-mode expectation comparison, mode8 probability
    // cutoff, or stale mutable inventory are silently inferred in this slice.
    return u.force_type==1 && u.ai.attack_id==kAttackAttack &&
           !u.pair.bound && u.pair.role==rn::PairRole::None &&
           u.ai.runtime_tuning_bound && u.ai.battle_rate<=2;
}
}
AiNearestTargetSelection SelectNearestTargetWithAttackViability(
    const rn::NativeRuntime& r,std::uint16_t actor_slot,const AiNearestEnemyPlanning& planning,
    rn::NativeGameState& state,AiNearestViabilityTrace& trace) {
    AiNearestTargetSelection out{};
    const auto first_draw=state.rng.ai;
    auto failure=[&](AiNearestEnemyMoveStatus s) {out.status=s;trace.attempted_ai_draws+=state.rng.ai-first_draw;return out;};
    if(planning.status!=AiNearestEnemyMoveStatus::Ok || !planning.move_power || !ViabilityProfile(r,actor_slot))
        return failure(AiNearestEnemyMoveStatus::UnsupportedAttackViability);
    const auto& actor=r.game.units[actor_slot];
    auto preview_runtime=std::make_unique<rn::NativeRuntime>(r);
    for(const auto& t:planning.targets) {
        if(t.target_slot>=r.game.units.size())return failure(AiNearestEnemyMoveStatus::UnsupportedTargetPermission);
        const auto union_position=SelectNearestPlanningPosition(t.cells,state);
        out.position_ties=std::uint16_t(out.position_ties+union_position.ties);
        if(union_position.status==AiNearestEnemyMoveStatus::MissingAiRandom)return failure(union_position.status);
        if(union_position.status!=AiNearestEnemyMoveStatus::Ok)continue;
        ++trace.target_checks;trace.last_target=t.target_slot;
        const auto& target=r.game.units[t.target_slot];
        if(target.pair.bound || target.pair.role!=rn::PairRole::None || target.weakness[6]>99 || target.weakness[7]>99)
            return failure(AiNearestEnemyMoveStatus::UnsupportedAttackViability);
        AiPlanningWeaponChoice best{};
        const auto inventory=rn::ReadCalculationInventory(actor);if(!inventory)return failure(AiNearestEnemyMoveStatus::MissingItem);
        for(std::uint8_t index=0;index<inventory->size();++index) {
            const auto id=(*inventory)[index].item_id;if(!id)continue;
            if(std::find(planning.usable_inventory_ids.begin(),planning.usable_inventory_ids.end(),id)==planning.usable_inventory_ids.end())continue;
            const auto* item=r.definitions.FindItem(id);
            if(!item)return failure(AiNearestEnemyMoveStatus::MissingItem);
            std::vector<AiPlanningAttackCell> cells;
            for(const auto& cell:t.cells) {
                const int d=std::abs(int(cell.x)-target.x)+std::abs(int(cell.y)-target.y);
                if(d>=item->min_range && d<=item->max_range)cells.push_back(cell);
            }
            const auto pos=SelectNearestPlanningPosition(cells,state);
            trace.weapon_position_ties=std::uint16_t(trace.weapon_position_ties+pos.ties);
            if(pos.status==AiNearestEnemyMoveStatus::MissingAiRandom)return failure(pos.status);
            if(pos.status!=AiNearestEnemyMoveStatus::Ok)continue;
            // This copy selects a slot for the simulator only. Moving toward a
            // target does not commit GetAttackScore's speculative equipment.
            // The request carries a slot; the live/current equipment is untouched.
            AiOrdinaryTacticalCandidateInput candidate{};bn::BattlePreviewResult preview{};
            preview.status=static_cast<bn::BattleTransactionStatus>(0xFFu);
            const auto status=ProjectSharedOrdinaryBattleCandidate(*preview_runtime,
                {actor_slot,t.target_slot,index,pos.x,pos.y,index},candidate,&preview);
            ++trace.weapon_previews;trace.last_item=id;trace.last_projection_status=std::uint8_t(status);trace.last_preview_status=std::uint8_t(preview.status);
            if(preview.status==bn::BattleTransactionStatus::UnsupportedAroundProjection) {
                const bool first=preview.attacker.around.status!=bn::AroundProjectionStatus::Ok;
                const auto& around=first?preview.attacker.around:preview.defender.around;
                trace.last_around_side=first?0u:1u;trace.last_around_status=std::uint8_t(around.status);
                trace.last_around_source=around.rejected_source;trace.last_around_skill=around.rejected_skill;
            }
            if(status!=AiCandidatePreviewStatus::Ok)return failure(AiNearestEnemyMoveStatus::UnsupportedAttackViability);
            const bool power0=IsPower0AttackExact(BindOrdinaryPower0(actor,target,candidate));
            if(power0)++trace.power0_rejections;
            const auto score=std::bit_cast<std::uint32_t>(ScoreOrdinaryTacticalCandidateExact(candidate));
            const auto previous_weapon_ties=best.equal_score_draws;
            ConsiderPlanningWeaponExact(best,{index,id,pos.x,pos.y,score,power0},state);
            trace.weapon_score_ties=std::uint16_t(trace.weapon_score_ties+best.equal_score_draws-previous_weapon_ties);
            if(best.missing_ai_random)return failure(AiNearestEnemyMoveStatus::MissingAiRandom);
        }
        if(!best.selected)continue;
        const auto target_score=OrdinaryNearestTargetScore(t.history_count,std::uint16_t(union_position.cost),planning.move_power);
        if(target_score>out.score)continue;
        if(target_score==out.score) {
            if(!state.rng.ai_state.initialized)return failure(AiNearestEnemyMoveStatus::MissingAiRandom);
            ++out.target_ties;if(DrawAiLocalRandomBoundedExact(state,2u)!=0)continue;
        }
        out.status=AiNearestEnemyMoveStatus::Ok;out.score=target_score;out.target_slot=t.target_slot;
        out.attack_x=union_position.x;out.attack_y=union_position.y;trace.selected_item=best.candidate.item_id;
    }
    trace.attempted_ai_draws+=state.rng.ai-first_draw;return out;
}
}
