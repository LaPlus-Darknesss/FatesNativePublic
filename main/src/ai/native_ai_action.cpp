#include "fates/ai/native_ai_immediate_attack.hpp"
#include "fates/ai/native_ai_position_movement.hpp"
#include "fates/ai/native_ai_action.hpp"
#include "fates/ai/native_ai_nearest_enemy_movement.hpp"
#include <algorithm>
#include <cstdlib>
#include <limits>
#include <utility>
#include <vector>
namespace fates::ai::native {
using fates::runtime::native::CommitUnitMove;
using fates::runtime::native::EnumerateUnitMovement;
using fates::runtime::native::PlayerActionStatus;
bool SupportsEverytimeAttackNearestEnemy(const fates::runtime::native::UnitState& u) noexcept {
    return u.ai.configured && u.ai.action_id==kActionEverytime && u.ai.mission_id==kMissionNull &&
           u.ai.attack_id==kAttackAttack && u.ai.movement_id==kMovementNearestEnemy;
}
bool SupportsEverytimeAttackIdle(const fates::runtime::native::UnitState& u) noexcept {
    return u.ai.configured && u.ai.action_id==kActionEverytime && u.ai.mission_id==kMissionNull &&
           u.ai.attack_id==kAttackAttack && u.ai.movement_id==kMovementNull;
}
bool SupportsAttackNearestEnemyDescriptor(const fates::runtime::native::UnitState& u) noexcept {
    const bool action=u.ai.action_id==kActionEverytime || u.ai.action_id==kActionTurnAttackRange;
    return u.ai.configured && action && u.ai.mission_id==kMissionNull && u.ai.attack_id==kAttackAttack &&
           u.ai.movement_id==kMovementNearestEnemy;
}
struct LiveGeometryCandidate { std::uint16_t target{}; fates::runtime::native::ReachableCell cell{}; };
static bool SelfCertifyRetailAttackPositionOrder(std::vector<LiveGeometryCandidate>& geometry) {
    // AIThink::GetAttackPosition scans candidate map cells with Y as the outer
    // ascending loop and X as the inner ascending loop in both its full-map and
    // bounded-range paths. We can therefore own that order without a caller trust
    // bit whenever this candidate set belongs to one target. Target enumeration is
    // a separate retail contract and remains fail-closed when multiple targets are
    // present.
    if(geometry.empty()) return false;
    const auto target=geometry.front().target;
    for(const auto& c:geometry) if(c.target!=target) return false;
    std::stable_sort(geometry.begin(),geometry.end(),[](const auto& a,const auto& b){
        if(a.cell.y!=b.cell.y) return a.cell.y<b.cell.y;
        return a.cell.x<b.cell.x;
    });
    return true;
}
static AiActionStatus ScoreLiveCandidates(fates::runtime::native::NativeRuntime& r, const std::uint16_t actor_slot,
    const std::vector<LiveGeometryCandidate>& geometry, const AiCandidatePreviewProvider& provider,
    const bool retail_attack_position_order_exact,
    LiveGeometryCandidate& chosen, std::int32_t& selected_score, std::uint16_t& previewed,
    std::uint32_t& equal_encounters) {
    if(!provider.build) return AiActionStatus::MissingCandidatePreview;
    std::vector<AiOrdinaryTacticalCandidateInput> in; in.reserve(geometry.size());
    for(std::size_t i=0;i<geometry.size();++i) {
        AiCandidatePreviewRequest req{actor_slot,geometry[i].target,static_cast<std::uint16_t>(i),geometry[i].cell.x,geometry[i].cell.y};
        AiOrdinaryTacticalCandidateInput c{};
        const auto s=provider.build(r,req,c,provider.user);
        if(s==AiCandidatePreviewStatus::UnsupportedSpecialIndication) return AiActionStatus::UnsupportedSpecialIndication;
        if(s!=AiCandidatePreviewStatus::Ok) return AiActionStatus::CandidatePreviewRejected;
        c.candidate_index=req.candidate_index; c.target_slot=req.target_slot; c.attack_x=req.attack_x; c.attack_y=req.attack_y;
        if(c.requires_special_indication) return AiActionStatus::UnsupportedSpecialIndication;
        in.push_back(c);
    }
    previewed=static_cast<std::uint16_t>(in.size());
    if(provider.retail_enumeration_order_exact || retail_attack_position_order_exact) {
        auto result=ScoreAndSelectOrdinaryTacticalCandidatesExact(in,r.game);
        if(result.status!=AiOrdinaryMultiCandidateStatus::Selected) return AiActionStatus::CandidateSelectionRejected;
        chosen=geometry[result.selected.candidate_index]; selected_score=result.selected.score;
        equal_encounters=result.equal_score_encounters; return AiActionStatus::Ok;
    }
    std::int32_t best=0; std::size_t best_i=0; unsigned ties=0;
    for(std::size_t i=0;i<in.size();++i) {
        const auto sc=ScoreOrdinaryTacticalCandidateExact(in[i]);
        if(i==0 || RetailAttackScoreGreater(sc,best)) { best=sc; best_i=i; ties=1; }
        else if(sc==best) { ++ties; }
    }
    if(ties!=1) return AiActionStatus::AmbiguousRetailTieOrder;
    chosen=geometry[best_i]; selected_score=best; equal_encounters=0; return AiActionStatus::Ok;
}
static AiActionResult ExecuteNoImmediateMovement(fates::runtime::native::NativeRuntime& r,
    const std::uint16_t slot,AiActionResult out) {
        const auto ai_before=r.game.rng.ai;
        const auto movement=ExecuteNearestEnemyMovementOnly(r,slot);
        out.ai_rng_draws=r.game.rng.ai-ai_before;
        out.target_slot=movement.target_slot;
        out.movement_target_equal_score_encounters=movement.target_equal_score_encounters;
        out.movement_planning_position_tie_draws=movement.planning_position_tie_draws;
        out.movement_tile_tie_draws=movement.movement_tile_tie_draws;
        if(movement.status==AiNearestEnemyMoveStatus::Ok) {
            out.movement_only=true; out.moved=movement.moved;
            out.end_x=movement.end_x; out.end_y=movement.end_y;
            out.status=AiActionStatus::Ok; return out;
        }
        if(movement.status==AiNearestEnemyMoveStatus::AmbiguousRetailTargetOrder) {
            out.status=AiActionStatus::AmbiguousRetailMovementTargetOrder; return out;
        }
        out.status=AiActionStatus::MovementPlanningRejected; return out;
}
AiActionResult ExecuteAttackNearestEnemy(fates::runtime::native::NativeRuntime& r,const std::uint16_t slot,
                                         const std::uint16_t turn) {
    return ExecuteAttackNearestEnemy(r,slot,turn,nullptr);
}
AiActionResult ExecuteAttackNearestEnemy(fates::runtime::native::NativeRuntime& r,const std::uint16_t slot,
                                         const std::uint16_t current_turn,const AiCandidatePreviewProvider* preview) {
    AiActionResult out{}; out.unit_slot=slot;
    if(slot>=r.game.units.size()) return out;
    auto& actor=r.game.units[slot];
    if(!actor.occupied || !actor.has_position || !fates::runtime::native::IsTacticalForce(actor.force_type) || actor.defeated || actor.pair.role==fates::runtime::native::PairRole::Partner) return out;
    out.start_x=actor.x; out.start_y=actor.y; out.end_x=actor.x; out.end_y=actor.y;
    if(!SupportsAttackNearestEnemyDescriptor(actor)) { out.status=AiActionStatus::UnsupportedDescriptor; return out; }
    if(actor.ai.action_id==kActionTurnAttackRange) {
        const auto a=EvaluateTurnAttackRangeActivation(r,slot,current_turn);
        if(!a.supported) { out.status=AiActionStatus::UnsupportedDescriptor; return out; }
        out.activation_turn_cause=a.turn_cause; out.activation_attack_range_cause=a.attack_range_cause;
        if(!a.active) { out.status=AiActionStatus::InactiveAction; return out; }
    }
    // Required ordinary fresh-enemy attacks now use the recovered position ->
    // weapon -> target selection and explicit alternative-action arbitration.
    // Neither a lone geometry cell nor the legacy provider trust bit bypasses it.
    if(actor.force_type==1 && actor.create_from_dispos_combat_init_bound && !actor.pair.bound &&
       actor.pair.role==fates::runtime::native::PairRole::None && (actor.ai.policy_flags&0x40u)==0) {
        auto immediate=detail::ExecuteOrdinaryImmediateAttack(r,slot);
        immediate.activation_turn_cause=out.activation_turn_cause;
        immediate.activation_attack_range_cause=out.activation_attack_range_cause;
        if(immediate.status!=AiActionStatus::NoAttackPosition)return immediate;
        // Retain the established no-contact diagnostic movement subset. This
        // is not a claim that InterferenceTo or cannon fallback are implemented.
        return ExecuteNoImmediateMovement(r,slot,out);
    }
    const auto* item=r.definitions.FindItem(actor.equipped_item_id);
    if(!item) { out.status=AiActionStatus::MissingItem; return out; }
    const auto cells=EnumerateUnitMovement(r,slot);
    std::vector<LiveGeometryCandidate> geometry; unsigned hostile_count=0; unsigned attackable_targets=0;
    for(std::uint16_t i=0;i<r.game.units.size();++i) {
        if(i==slot) continue;
        const auto& u=r.game.units[i];
        if(!u.occupied || !u.has_position || !fates::runtime::native::IsTacticalForce(u.force_type) || u.defeated || u.pair.role==fates::runtime::native::PairRole::Partner || u.force_type==actor.force_type) continue;
        ++hostile_count; bool target_added=false;
        for(const auto& c:cells) {
            if(!c.occupiable) continue;
            const int d=std::abs(static_cast<int>(c.x)-static_cast<int>(u.x))+std::abs(static_cast<int>(c.y)-static_cast<int>(u.y));
            if(d>=item->min_range && d<=item->max_range) { geometry.push_back({i,c}); target_added=true; }
        }
        if(target_added) ++attackable_targets;
    }
    out.hostile_target_count=static_cast<std::uint16_t>(hostile_count); out.candidate_target_count=static_cast<std::uint16_t>(attackable_targets);
    out.candidate_attack_position_count=static_cast<std::uint16_t>(geometry.size());
    if(hostile_count==0) { out.status=AiActionStatus::NoHostileTarget; return out; }
    if(geometry.empty()) return ExecuteNoImmediateMovement(r,slot,out);
    LiveGeometryCandidate chosen{};
    const auto ai_before=r.game.rng.ai;
    const bool retail_attack_position_order_exact=
        geometry.size()>1 && SelfCertifyRetailAttackPositionOrder(geometry);
    if(geometry.size()==1) chosen=geometry.front();
    else {
        if(!preview) { out.status=AiActionStatus::MissingCandidatePreview; return out; }
        const auto s=ScoreLiveCandidates(r,slot,geometry,*preview,retail_attack_position_order_exact,
                                         chosen,out.selected_score,out.previewed_candidate_count,out.equal_score_encounters);
        if(s!=AiActionStatus::Ok) { out.status=s; return out; }
    }
    out.ai_rng_draws=r.game.rng.ai-ai_before; out.target_slot=chosen.target;
    const auto old_x=actor.x,old_y=actor.y; const auto move=CommitUnitMove(r,slot,chosen.cell.x,chosen.cell.y);
    if(move!=PlayerActionStatus::Ok) { out.status=AiActionStatus::MovementRejected; return out; }
    out.moved=(chosen.cell.x!=old_x || chosen.cell.y!=old_y); out.end_x=actor.x; out.end_y=actor.y;
    const auto game_before=r.game.rng.game;
    auto battle=fates::battle::native::ExecuteOrdinaryBattle(r,slot,chosen.target);
    if(battle.status!=fates::battle::native::BattleTransactionStatus::Ok) {
        actor.x=old_x; actor.y=old_y; out.end_x=old_x; out.end_y=old_y; out.status=AiActionStatus::BattleRejected; return out;
    }
    out.attacked=true; out.battle=std::move(battle); out.game_rng_draws=r.game.rng.game-game_before; out.status=AiActionStatus::Ok; return out;
}
AiActionResult ExecuteEverytimeAttackNearestEnemy(fates::runtime::native::NativeRuntime& r,const std::uint16_t slot) {
    if(slot>=r.game.units.size()) return {};
    if(!SupportsEverytimeAttackNearestEnemy(r.game.units[slot])) { AiActionResult o{};o.unit_slot=slot;o.status=AiActionStatus::UnsupportedDescriptor;return o; }
    return ExecuteAttackNearestEnemy(r,slot,0,nullptr);
}

AiActionResult ExecuteAttackIdle(fates::runtime::native::NativeRuntime& r,const std::uint16_t slot,
                                 const std::uint16_t current_turn,const AiCandidatePreviewProvider* preview) {
    (void)current_turn;
    AiActionResult out{};out.unit_slot=slot;
    if(slot>=r.game.units.size()) return out;
    auto& actor=r.game.units[slot];
    if(!actor.occupied||!actor.has_position||!fates::runtime::native::IsTacticalForce(actor.force_type)||actor.defeated||
       actor.pair.role==fates::runtime::native::PairRole::Partner) return out;
    out.start_x=actor.x;out.start_y=actor.y;out.end_x=actor.x;out.end_y=actor.y;
    if(!SupportsEverytimeAttackIdle(actor)){out.status=AiActionStatus::UnsupportedDescriptor;return out;}
    const auto* item=r.definitions.FindItem(actor.equipped_item_id);
    if(!item){out.status=AiActionStatus::MissingItem;return out;}

    std::vector<LiveGeometryCandidate> geometry;unsigned hostile_count=0;unsigned attackable_targets=0;
    for(std::uint16_t i=0;i<r.game.units.size();++i){
        if(i==slot)continue;
        const auto& u=r.game.units[i];
        if(!u.occupied||!u.has_position||!fates::runtime::native::IsTacticalForce(u.force_type)||u.defeated||
           u.pair.role==fates::runtime::native::PairRole::Partner||u.force_type==actor.force_type)continue;
        ++hostile_count;
        const int d=std::abs(static_cast<int>(actor.x)-static_cast<int>(u.x))+
                    std::abs(static_cast<int>(actor.y)-static_cast<int>(u.y));
        if(d>=item->min_range&&d<=item->max_range){
            geometry.push_back({i,{actor.x,actor.y,0,true}});++attackable_targets;
        }
    }
    out.hostile_target_count=static_cast<std::uint16_t>(hostile_count);
    out.candidate_target_count=static_cast<std::uint16_t>(attackable_targets);
    out.candidate_attack_position_count=static_cast<std::uint16_t>(geometry.size());
    if(hostile_count==0){out.status=AiActionStatus::NoHostileTarget;return out;}
    if(geometry.empty()){
        // AIThink::ActionMoveIdle/MoveTo keeps the ordinary non-mutation unit at
        // its current tile. With no attackable target this is a completed idle
        // action, not NearestEnemy pathfinding.
        actor.action_committed=true;out.movement_only=true;out.status=AiActionStatus::Ok;return out;
    }

    LiveGeometryCandidate chosen{};const auto ai_before=r.game.rng.ai;
    if(geometry.size()==1) chosen=geometry.front();
    else {
        if(!preview){out.status=AiActionStatus::MissingCandidatePreview;return out;}
        const auto s=ScoreLiveCandidates(r,slot,geometry,*preview,false,chosen,out.selected_score,
                                         out.previewed_candidate_count,out.equal_score_encounters);
        if(s!=AiActionStatus::Ok){out.status=s;return out;}
    }
    out.ai_rng_draws=r.game.rng.ai-ai_before;out.target_slot=chosen.target;
    const auto game_before=r.game.rng.game;
    auto battle=fates::battle::native::ExecuteOrdinaryBattle(r,slot,chosen.target);
    if(battle.status!=fates::battle::native::BattleTransactionStatus::Ok){out.status=AiActionStatus::BattleRejected;return out;}
    out.attacked=true;out.battle=std::move(battle);out.game_rng_draws=r.game.rng.game-game_before;
    out.status=AiActionStatus::Ok;return out;
}

AiActionResult ExecuteConfiguredAiAction(fates::runtime::native::NativeRuntime& r,const std::uint16_t slot,
                                         const std::uint16_t current_turn,const AiCandidatePreviewProvider* preview){
    if(slot<r.game.units.size() && r.game.units[slot].ai.attack_id==kAttackNull &&
       r.game.units[slot].ai.movement_id==kMovementPosition) {
        const auto move=ExecuteConfiguredPositionMovement(r,slot,current_turn);
        AiActionResult out{};out.unit_slot=slot;out.start_x=move.start_x;out.start_y=move.start_y;
        out.end_x=move.end_x;out.end_y=move.end_y;out.moved=move.moved;out.movement_only=true;
        out.ai_rng_draws=move.ai_rng_draws;out.movement_tile_tie_draws=static_cast<std::uint16_t>(move.tie_draws);
        out.position_movement_status=static_cast<std::uint8_t>(move.status);
        out.status=move.status==AiPositionMoveStatus::Ok?AiActionStatus::Ok:
            move.status==AiPositionMoveStatus::Inactive?AiActionStatus::InactiveAction:AiActionStatus::MovementPlanningRejected;
        return out;
    }

    if(slot>=r.game.units.size()) return {};
    const auto& u=r.game.units[slot];
    if(SupportsAttackNearestEnemyDescriptor(u)) return ExecuteAttackNearestEnemy(r,slot,current_turn,preview);
    if(SupportsEverytimeAttackIdle(u)) return ExecuteAttackIdle(r,slot,current_turn,preview);
    AiActionResult out{};out.unit_slot=slot;out.status=AiActionStatus::UnsupportedDescriptor;return out;
}
} // namespace fates::ai::native
