#pragma once
#include "fates/ai/native_ai_nearest_enemy_movement.hpp"
#include "fates/ai/native_ai_candidate_scoring.hpp"
#include <cstdint>
#include <span>

namespace fates::ai::native {
// Resolved inputs to AIBattleSimulator::IsPower0Attack after its cached Skill
// identity setup. This is a decision predicate, NOT an effect application API.
struct AiPower0AttackFacts {
    std::int32_t primary_power{};
    float ordinary_outcome_probability{};
    std::int32_t defender_attack_count{};
    bool poison_strike{},grisly_wound{},savage_blow{};
    bool primary_item_weakness{},partner_item_weakness{};
    std::int8_t primary_defense_weakness{},primary_resistance_weakness{};
    std::int8_t partner_defense_weakness{},partner_resistance_weakness{};
    std::int32_t partner_attack_count{};
    bool seal_defense{},seal_resistance{},target_status_immunity{},target_status_resistance{};
    bool target_exists{},inevitable_end{};
    std::int8_t target_defense_weakness{},target_resistance_weakness{};
};
bool IsPower0AttackExact(const AiPower0AttackFacts&) noexcept;
// Resolved-fact binding shared by immediate and nearest weapon selection.
AiPower0AttackFacts BindOrdinaryPower0(
    const fates::runtime::native::UnitState&,const fates::runtime::native::UnitState&,
    const AiOrdinaryTacticalCandidateInput&);

struct AiPlanningWeaponCandidate {
    std::uint8_t inventory_slot{};
    std::uint16_t item_id{};
    std::int16_t x{},y{};
    std::uint32_t score{};
    bool power0{};
};
struct AiPlanningWeaponChoice {
    bool selected{};
    AiPlanningWeaponCandidate candidate{};
    std::uint16_t equal_score_draws{};
    bool missing_ai_random{};
};
// GetAttackScore compares scores UNSIGNED, and accepts the first qualifying
// weapon even at score zero. Equal candidates use the existing AI-local RNG.
void ConsiderPlanningWeaponExact(AiPlanningWeaponChoice&,const AiPlanningWeaponCandidate&,
                                fates::runtime::native::NativeGameState&);

// The planning argument is produced by InspectNearestEnemyPlanning. This
// composes the ordinary flags=6, non-clever, unpaired GetAttackScore branch.
// The original union-range position remains the movement/history position;
// per-weapon positions and equip identities exist only in speculative previews.
AiNearestTargetSelection SelectNearestTargetWithAttackViability(
    const fates::runtime::native::NativeRuntime&,std::uint16_t actor,
    const AiNearestEnemyPlanning&,fates::runtime::native::NativeGameState& staged_rng,
    AiNearestViabilityTrace&);
}
