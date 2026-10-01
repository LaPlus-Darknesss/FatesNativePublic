#include "fates/ai/native_ai_activation.hpp"
#include "fates/runtime/native_player_action.hpp"
#include <cstdlib>
namespace fates::ai::native {
bool HasSingleTurnArgumentShape(const fates::runtime::native::UnitState& u) noexcept {
    return u.ai.action_args[0] >= 0 && u.ai.action_args[1] == -1 &&
           u.ai.action_args[2] == -1 && u.ai.action_args[3] == -1;
}
bool HasB007TurnAttackRangeArgumentShape(const fates::runtime::native::UnitState& u) noexcept {
    return HasSingleTurnArgumentShape(u);
}
bool HasHostileInsideFullMoveAttackArea(const fates::runtime::native::NativeRuntime& r,
                                        const std::uint16_t slot) {
    if(slot>=r.game.units.size()) return false;
    const auto& actor=r.game.units[slot];
    if(!actor.occupied || !actor.has_position || !fates::runtime::native::IsTacticalForce(actor.force_type) || actor.defeated || actor.pair.role==fates::runtime::native::PairRole::Partner) return false;
    const auto* item=r.definitions.FindItem(actor.equipped_item_id);
    const auto* job=r.definitions.FindJob(actor.job_id);
    if(!item || !job || !r.definitions.terrain_map() || item->min_range<0 || item->max_range<item->min_range) return false;
    const auto cells=fates::runtime::native::EnumerateUnitMovement(r,slot);
    for(std::uint16_t i=0;i<r.game.units.size();++i) {
        if(i==slot) continue;
        const auto& target=r.game.units[i];
        if(!target.occupied || !target.has_position || !fates::runtime::native::IsTacticalForce(target.force_type) || target.defeated || target.pair.role==fates::runtime::native::PairRole::Partner || target.force_type==actor.force_type) continue;
        for(const auto& c:cells) {
            if(!c.occupiable) continue;
            const int d=std::abs(static_cast<int>(c.x)-static_cast<int>(target.x))+
                        std::abs(static_cast<int>(c.y)-static_cast<int>(target.y));
            if(d>=item->min_range && d<=item->max_range) return true;
        }
    }
    return false;
}
TurnAttackRangeActivation EvaluateTurnAttackRangeActivation(
    const fates::runtime::native::NativeRuntime& r,
    const std::uint16_t slot,
    const std::uint16_t current_turn) {
    TurnAttackRangeActivation out{};
    if(slot>=r.game.units.size()) return out;
    const auto& u=r.game.units[slot];
    if(!u.ai.configured || u.ai.action_id!=kActionTurnAttackRange ||
       !HasB007TurnAttackRangeArgumentShape(u)) return out;
    const auto* item=r.definitions.FindItem(u.equipped_item_id);
    const auto* job=r.definitions.FindJob(u.job_id);
    if(!item || !job || !r.definitions.terrain_map() || item->min_range<0 || item->max_range<item->min_range) return out;
    out.supported=true;
    // Retail ActiveCauseTurn: current turn >= resolved action arg0.
    out.turn_cause=current_turn>=static_cast<std::uint16_t>(u.ai.action_args[0]);
    // TurnAttackRange's second declaration record resolves action arg1 (-1/default),
    // so ActiveCauseAttackRange uses 100% move power from the current position.
    out.attack_range_cause=HasHostileInsideFullMoveAttackArea(r,slot);
    // ProcessingActive evaluates all cause records and marks the declaration active
    // if any cause returns nonzero: TurnAttackRange is Turn OR AttackRange.
    out.active=out.turn_cause || out.attack_range_cause;
    return out;
}
} // namespace fates::ai::native
