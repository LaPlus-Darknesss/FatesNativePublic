#include "fates/runtime/native_item_inventory.hpp"
#include "fates/ai/native_ai_immediate_attack.hpp"
#include "fates/ai/native_ai_shared_battle_preview.hpp"
#include "fates/ai/native_ai_movement_fields.hpp"
#include "fates/ai/native_ai_kill_probability.hpp"
#include "fates/runtime/native_current_item_eligibility.hpp"
#include "fates/map/native_scenario_tricks.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <memory>

namespace fates::ai::native {
namespace rn=fates::runtime::native;
namespace bn=fates::battle::native;
std::uint32_t OrdinaryImmediatePositionScore(std::uint16_t cost,std::uint8_t terrain,
                                            std::uint8_t dual,bool outside) noexcept {
    const auto priority=((std::uint32_t(dual)+(outside?8u:0u))*16u+terrain)*16u;
    return 100u-cost+(priority<<8u);
}
AiImmediatePositionChoice SelectImmediatePositionExact(std::span<const AiImmediatePosition> cells,rn::NativeGameState& g) {
    AiImmediatePositionChoice out{};
    for(const auto& c:cells) {
        if(c.cost<0 || c.cost>100)continue;
        const auto score=OrdinaryImmediatePositionScore(std::uint16_t(c.cost),c.terrain_score,c.dual_score,c.outside_counter_range);
        if(score<out.score)continue;
        if(score==out.score) {
            if(!g.rng.ai_state.initialized){out.missing_ai_random=true;return out;}
            ++out.ties;if(DrawAiLocalRandomBoundedExact(g,2u)!=0u)continue;
        }
        out.selected=true;out.cell=c;out.score=score;
    }
    return out;
}
void ConsiderImmediateTargetExact(AiImmediateTargetChoice& out,std::uint16_t target,
    const AiPlanningWeaponCandidate& candidate,float lane,rn::NativeGameState& g) {
    if(out.selected) {
        if(candidate.score<out.weapon.score)return;
        if(candidate.score==out.weapon.score) {
            if(!g.rng.ai_state.initialized){out.missing_ai_random=true;return;}
            ++out.ties;if(DrawAiLocalRandomBoundedExact(g,2u)!=0u)return;
        }
    }
    out.selected=true;out.target=target;out.weapon=candidate;out.cannon_score_lane0=lane;
}
bool CannonAlternativeRejectedByScore(float lane) noexcept {
    // Signed integer compare of the float bits in the actual early-return gate.
    // Nonfinite/negative probabilities are outside the supported score contract.
    if(!std::isfinite(lane) || lane<0.0f)return false;
    return std::bit_cast<std::int32_t>(lane)>=0x3F000000;
}
namespace {
bool OnMap(const rn::UnitState& u) {
    return u.occupied && u.has_position && !u.defeated && u.force_type<3 && u.pair.role!=rn::PairRole::Partner;
}
int Distance(int x,int y,const rn::UnitState& u){return std::abs(x-u.x)+std::abs(y-u.y);}
bool OnlyActorInForce(const rn::NativeRuntime& r,std::uint16_t actor) {
    // AIDual scans the whole force list, not merely adjacent units. With only
    // self it skips the sole entry before querying candidates or drawing RNG.
    // Be conservative about hidden, defeated, and reserve-like same-force slots.
    for(std::uint16_t i=0;i<r.game.units.size();++i)
        if(i!=actor && r.game.units[i].occupied && r.game.units[i].force_type==r.game.units[actor].force_type)return false;
    return true;
}
bool CounterMask(const rn::NativeRuntime& r,const rn::UnitState& u,std::uint32_t& mask) {
    mask=0;if(!u.equipped_item_id)return true;
    const auto* item=r.definitions.FindItem(u.equipped_item_id);
    const auto* p=r.definitions.FindPerson(u.person_id);const auto* j=r.definitions.FindJob(u.job_id);
    if(!item || !p || !j)return false;
    const auto group=r.definitions.WeaponExpGroupForItem(*item);
    if(group>=8 || group==6 || item->min_range<1 || item->max_range>31 || item->max_range<item->min_range)return false;
    if(rn::ProjectCurrentItemEligibility(r,u,*item,false,true)!=rn::CurrentItemEligibility::Yes)return false;
    for(int d=item->min_range;d<=item->max_range;++d)mask|=std::uint32_t(1)<<unsigned(d);
    return true;
}
AiImmediateAttackPlan Select(const rn::NativeRuntime& r,std::uint16_t slot,rn::NativeGameState& rng) {
    AiImmediateAttackPlan out{};const auto before=rng.rng.ai;
    auto fail=[&](AiImmediateAttackStatus s){out.status=s;out.trace.attempted_ai_draws=rng.rng.ai-before;return out;};
    if(slot>=r.game.units.size())return fail(AiImmediateAttackStatus::InvalidUnit);
    auto support_query=[&](std::uint16_t subject,int x,int y) {
        ++out.trace.support_queries;
        const auto result=fates::support::native::InspectLocalSupportSelection(r,subject,std::int16_t(x),std::int16_t(y),slot);
        if(result.status!=fates::support::native::LocalSupportStatus::Ok) {
            out.trace.support_subject=subject;out.trace.support_source=result.rejected_source;
            out.trace.support_x=std::int16_t(x);out.trace.support_y=std::int16_t(y);
            out.trace.support_status=std::uint8_t(result.status);
        }
        return result;
    };
    const auto& actor=r.game.units[slot];
    if(!OnMap(actor) || actor.action_committed)return fail(AiImmediateAttackStatus::InvalidUnit);
    if(!SupportsAttackNearestEnemyDescriptor(actor) || actor.force_type!=1 || actor.pair.bound ||
       actor.pair.role!=rn::PairRole::None || actor.ai.battle_rate>2)return fail(AiImmediateAttackStatus::UnsupportedProfile);
    // AttackTo policy 0x80 adds field bit 0x8000 and considers the partner
    // inventory. GetRangeBit stops at a null partner; the unpaired profile
    // makes both partner alternatives empty without changing the authored bit.
    const auto planning=InspectNearestEnemyPlanning(r,slot);
    if(planning.status==AiNearestEnemyMoveStatus::AmbiguousRetailTargetOrder)return fail(AiImmediateAttackStatus::MissingForceOrder);
    if(planning.status==AiNearestEnemyMoveStatus::NoHostileTarget)return fail(AiImmediateAttackStatus::NoImmediateGeometry);
    if(planning.status!=AiNearestEnemyMoveStatus::Ok)return fail(AiImmediateAttackStatus::UnsupportedProfile);
    const auto& map=*r.definitions.terrain_map();
    // UnitAIMove actual-budget input, not the nearest-target 100-cost probe.
    const auto field=detail::BuildOrdinaryAiField(r,slot,actor.x,actor.y,planning.move_power,{true,true,false,false});
    auto simulator=std::make_unique<rn::NativeRuntime>(r);
    AiImmediateTargetChoice selected{};
    for(const auto& t:planning.targets) {
        const auto& target=r.game.units[t.target_slot];
        // Do not demand battle semantics for targets outside the attack union.
        bool in_range=false;
        for(int y=int(map.min_y);y<int(map.max_y);++y)for(int x=int(map.min_x);x<int(map.max_x);++x) {
            const int cost=field.get(x,y),d=Distance(x,y,target);
            if(cost>=0 && field.stop[std::size_t(y*32+x)] && d>0 && d<32 && (planning.range_mask&(std::uint32_t(1)<<unsigned(d))))in_range=true;
        }
        if(!in_range)continue;
        ++out.trace.targets;out.trace.last_target=t.target_slot;
        if(target.pair.bound || target.pair.role!=rn::PairRole::None || target.weakness[6]>99 || target.weakness[7]>99)return fail(AiImmediateAttackStatus::UnsupportedSupportProjection);
        std::uint32_t counter{};
        if(!CounterMask(r,target,counter))return fail(AiImmediateAttackStatus::InvalidEquipment);
        if(support_query(t.target_slot,target.x,target.y).status!=fates::support::native::LocalSupportStatus::Ok)return fail(AiImmediateAttackStatus::UnsupportedSupportProjection);
        AiPlanningWeaponChoice weapon{};float weapon_lane=0.0f;
        const auto inventory=rn::ReadCalculationInventory(actor);if(!inventory)return fail(AiImmediateAttackStatus::InvalidEquipment);
        for(std::uint8_t index=0;index<inventory->size();++index) {
            const auto id=(*inventory)[index].item_id;if(!id)continue;
            if(std::find(planning.usable_inventory_ids.begin(),planning.usable_inventory_ids.end(),id)==planning.usable_inventory_ids.end())continue;
            ++out.trace.inventory_visits;out.trace.last_item=id;
            const auto* item=r.definitions.FindItem(id);if(!item)return fail(AiImmediateAttackStatus::InvalidEquipment);
            std::vector<AiImmediatePosition> cells;
            for(int y=int(map.min_y);y<int(map.max_y);++y)for(int x=int(map.min_x);x<int(map.max_x);++x) {
                const int cost=field.get(x,y),d=Distance(x,y,target);
                if(cost<0 || !field.stop[std::size_t(y*32+x)] || d<item->min_range || d>item->max_range)continue;
                const auto* terrain=r.definitions.TerrainAt(x,y);if(!terrain)return fail(AiImmediateAttackStatus::UnsupportedProfile);
                // Selection consumes restored constructor/reliance facts, never AI RNG.
                const auto support=support_query(slot,x,y);
                if(support.status!=fates::support::native::LocalSupportStatus::Ok)return fail(AiImmediateAttackStatus::UnsupportedSupportProjection);
                if(support.dual_score)++out.trace.nonzero_support_positions;
                cells.push_back({std::int16_t(x),std::int16_t(y),std::int16_t(cost),
                    std::uint8_t(OrdinaryTerrainScore(terrain->defense_bonus,terrain->avoid_bonus,terrain->healing_bonus)),std::uint8_t(support.dual_score),
                    (counter&(std::uint32_t(1)<<unsigned(d)))==0});
            }
            out.trace.position_cells+=std::uint32_t(cells.size());
            const auto pos=SelectImmediatePositionExact(cells,rng);out.trace.position_ties+=pos.ties;
            if(pos.missing_ai_random)return fail(AiImmediateAttackStatus::MissingAiRandom);
            if(!pos.selected)continue;
            // Per-weapon projection selects a complete instance without a Unit write.
            AiOrdinaryTacticalCandidateInput candidate{};bn::BattlePreviewResult preview{};
            preview.status=static_cast<bn::BattleTransactionStatus>(0xffu);++out.trace.projections;
            const auto ps=ProjectSharedOrdinaryBattleCandidate(*simulator,{slot,t.target_slot,index,pos.cell.x,pos.cell.y,index},candidate,&preview);
            out.trace.projection_status=std::uint8_t(ps);out.trace.preview_status=std::uint8_t(preview.status);
            if(ps!=AiCandidatePreviewStatus::Ok)return fail(AiImmediateAttackStatus::PreviewRejected);
            const bool power0=planning.requires_attack_viability && IsPower0AttackExact(BindOrdinaryPower0(actor,target,candidate));
            if(power0){++out.trace.power0_rejections;continue;}
            const auto frame=ComposeResolvedAIBattleSimulatorScoreExact(BindOrdinaryResolvedBattleScoreInputExact(candidate));
            const auto previous=weapon;
            ConsiderPlanningWeaponExact(weapon,{index,id,pos.cell.x,pos.cell.y,std::uint32_t(frame.score),false},rng);
            if(weapon.missing_ai_random)return fail(AiImmediateAttackStatus::MissingAiRandom);
            if(weapon.selected && (!previous.selected || weapon.candidate.inventory_slot!=previous.candidate.inventory_slot))weapon_lane=frame.score_lanes.lane0;
        }
        out.trace.weapon_ties+=weapon.equal_score_draws;
        if(!weapon.selected)continue;
        ConsiderImmediateTargetExact(selected,t.target_slot,weapon.candidate,weapon_lane,rng);
        out.trace.target_ties=selected.ties;
        if(selected.missing_ai_random)return fail(AiImmediateAttackStatus::MissingAiRandom);
        if(selected.selected) {
            out.trace.target=selected.target;out.trace.item=selected.weapon.item_id;
            out.trace.inventory_slot=selected.weapon.inventory_slot;out.trace.x=selected.weapon.x;out.trace.y=selected.weapon.y;
            out.trace.score=selected.weapon.score;out.trace.cannon_score_lane0=selected.cannon_score_lane0;
        }
    }
    if(!selected.selected)return fail(out.trace.targets?AiImmediateAttackStatus::NoViableWeapon:AiImmediateAttackStatus::NoImmediateGeometry);
    // AttackTo consults AICannon even after a normal weapon has won. Only its
    // score-derived gate or a current script-registry absence proof is admitted.
    if(!CannonAlternativeRejectedByScore(selected.cannon_score_lane0)) {
        const auto world=fates::map::native::InspectScriptCannonAbsence(r,slot);
        out.trace.scenario_cannon_status=static_cast<std::uint8_t>(world.status);
        if(world.status!=fates::map::native::ScenarioTrickStatus::Ok)return fail(AiImmediateAttackStatus::CannonArbitrationRequired);
        out.trace.script_cannon_absence=true;
    }
    // The singleton-force case proves AIDual's cleared decision stays empty.
    // Merely having no adjacent ally would NOT prove this whole-force alternative.
    if(!OnlyActorInForce(r,slot)) {
        if(!fates::map::native::ScenarioDualAlternativeEarlyRejected(r,slot))return fail(AiImmediateAttackStatus::DualArbitrationRequired);
        out.trace.scenario_dual_early_reject=true;
    }
    return fail(AiImmediateAttackStatus::Ready);
}
}
AiImmediateAttackPlan InspectOrdinaryImmediateAttack(const rn::NativeRuntime& r,std::uint16_t slot) {
    auto state=std::make_unique<rn::NativeGameState>(r.game);return Select(r,slot,*state);
}
AiActionResult detail::ExecuteOrdinaryImmediateAttack(rn::NativeRuntime& r,std::uint16_t slot) {
    AiActionResult out{};out.unit_slot=slot;
    if(slot<r.game.units.size()){out.start_x=out.end_x=r.game.units[slot].x;out.start_y=out.end_y=r.game.units[slot].y;}
    auto staged=std::make_unique<rn::NativeRuntime>(r);const auto plan=Select(r,slot,staged->game);const auto& t=plan.trace;
    out.immediate_attack_status=std::uint8_t(plan.status);out.immediate_item_id=t.item;out.immediate_inventory_slot=t.inventory_slot;
    out.immediate_cannon_score_lane0=t.cannon_score_lane0;out.immediate_attempted_ai_draws=t.attempted_ai_draws;
    out.immediate_position_ties=t.position_ties;out.immediate_weapon_ties=t.weapon_ties;out.immediate_target_ties=t.target_ties;
    out.target_slot=t.target;out.selected_score=std::bit_cast<std::int32_t>(t.score);
    out.hostile_target_count=std::uint16_t(t.targets);out.candidate_target_count=std::uint16_t(t.targets);
    out.candidate_attack_position_count=std::uint16_t(t.position_cells);out.previewed_candidate_count=std::uint16_t(t.projections);
    if(plan.status==AiImmediateAttackStatus::NoImmediateGeometry){out.status=AiActionStatus::NoAttackPosition;return out;}
    out.status=AiActionStatus::ImmediateAttackRejected;
    if(plan.status!=AiImmediateAttackStatus::Ready)return out;
    auto& actor=staged->game.units[slot];
    if(actor.inventory.bound) {
        std::array<bool,5> admitted{};
        if(t.inventory_slot>=5 || actor.inventory.items[t.inventory_slot].item_id!=t.item)return out;
        // Select already validated eligibility and exp in this same staged world.
        admitted[t.inventory_slot]=true;
        if(rn::EquipCurrentInventory(actor,t.inventory_slot,admitted)!=rn::InventoryStatus::Ok)return out;
    } else actor.equipped_item_id=t.item; // prior explicit ID-only calculation scope
    if(rn::CommitUnitMove(*staged,slot,t.x,t.y)!=rn::PlayerActionStatus::Ok){out.immediate_attack_status=std::uint8_t(AiImmediateAttackStatus::MovementRejected);return out;}
    ++staged->game.ai_planner.target_move_counts[std::size_t(t.target)+1u];
    auto battle=bn::ExecuteOrdinaryBattle(*staged,slot,t.target);
    if(battle.status!=bn::BattleTransactionStatus::Ok){out.battle=std::move(battle);out.immediate_attack_status=std::uint8_t(AiImmediateAttackStatus::BattleRejected);return out;}
    out.ai_rng_draws=staged->game.rng.ai-r.game.rng.ai;out.game_rng_draws=staged->game.rng.game-r.game.rng.game;
    out.moved=t.x!=out.start_x || t.y!=out.start_y;out.end_x=t.x;out.end_y=t.y;out.attacked=true;out.battle=std::move(battle);
    r.game=std::move(staged->game);out.status=AiActionStatus::Ok;out.immediate_attack_status=std::uint8_t(AiImmediateAttackStatus::Committed);return out;
}
} // namespace fates::ai::native
