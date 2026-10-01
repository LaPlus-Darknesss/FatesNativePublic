#pragma once
#include "fates/ai/native_ai_activation.hpp"
#include "fates/ai/native_ai_candidate_scoring.hpp"
#include "fates/ai/native_ai_nearest_enemy_movement.hpp"
#include "fates/battle/native_battle_transaction.hpp"
#include "fates/runtime/native_player_action.hpp"
#include <cstdint>
namespace fates::ai::native {
inline constexpr std::uint8_t kMissionNull = 0;
inline constexpr std::uint8_t kAttackNull = 0;
inline constexpr std::uint8_t kAttackAttack = 1;
inline constexpr std::uint8_t kMovementNull = 0;
inline constexpr std::uint8_t kMovementNearestEnemy = 1;
inline constexpr std::uint8_t kMovementPosition = 20;
enum class AiActionStatus : std::uint8_t {
    Ok, InvalidUnit, UnsupportedDescriptor, InactiveAction, MissingItem, NoHostileTarget,
    AmbiguousHostileTarget, NoAttackPosition, AmbiguousAttackPosition,
    MovementRejected, BattleRejected, MissingCandidatePreview, CandidatePreviewRejected,
    UnsupportedSpecialIndication, AmbiguousRetailTieOrder, CandidateSelectionRejected,
    AmbiguousRetailMovementTargetOrder, MovementPlanningRejected,
    ImmediateAttackRejected
};
struct AiCandidatePreviewRequest {
    std::uint16_t unit_slot{};
    std::uint16_t target_slot{};
    std::uint16_t candidate_index{};
    std::int16_t attack_x{};
    std::int16_t attack_y{};
    std::uint8_t inventory_index{0xff}; // optional slot; candidate_index is only identity
};
enum class AiCandidatePreviewStatus : std::uint8_t { Ok, UnsupportedSpecialIndication, Rejected };
using AiCandidatePreviewBuildFn = AiCandidatePreviewStatus (*)(
    const fates::runtime::native::NativeRuntime&,
    const AiCandidatePreviewRequest&,
    AiOrdinaryTacticalCandidateInput&,
    void*);
struct AiCandidatePreviewProvider {
    AiCandidatePreviewBuildFn build{};
    void* user{};
    bool retail_enumeration_order_exact{};
};
struct AiActionResult {
    AiActionStatus status{AiActionStatus::InvalidUnit};
    std::uint16_t unit_slot{};
    std::uint16_t target_slot{};
    std::int16_t start_x{}, start_y{}, end_x{}, end_y{};
    std::uint8_t immediate_attack_status{0xFFu}; // AiImmediateAttackStatus
    std::uint16_t immediate_item_id{};
    std::uint8_t immediate_inventory_slot{0xFFu};
    float immediate_cannon_score_lane0{};
    std::uint64_t immediate_attempted_ai_draws{};
    std::uint32_t immediate_position_ties{},immediate_weapon_ties{},immediate_target_ties{};
    bool moved{};
    bool attacked{};
    bool movement_only{};
    std::uint8_t position_movement_status{0xFFu}; // AiPositionMoveStatus when that route is used
    bool activation_turn_cause{};
    bool activation_attack_range_cause{};
    std::uint16_t hostile_target_count{};
    std::uint16_t candidate_target_count{};
    std::uint16_t candidate_attack_position_count{};
    std::uint16_t previewed_candidate_count{};
    std::int32_t selected_score{};
    std::uint32_t equal_score_encounters{};
    std::uint16_t movement_target_equal_score_encounters{};
    std::uint16_t movement_planning_position_tie_draws{};
    std::uint16_t movement_tile_tie_draws{};
    fates::battle::native::BattleTransactionResult battle{};
    std::uint64_t game_rng_draws{};
    std::uint64_t ai_rng_draws{};
};
bool SupportsEverytimeAttackNearestEnemy(const fates::runtime::native::UnitState&) noexcept;
bool SupportsEverytimeAttackIdle(const fates::runtime::native::UnitState&) noexcept;
bool SupportsAttackNearestEnemyDescriptor(const fates::runtime::native::UnitState&) noexcept;
AiActionResult ExecuteAttackNearestEnemy(fates::runtime::native::NativeRuntime&, std::uint16_t, std::uint16_t);
AiActionResult ExecuteAttackNearestEnemy(fates::runtime::native::NativeRuntime&, std::uint16_t, std::uint16_t,
                                         const AiCandidatePreviewProvider*);
AiActionResult ExecuteEverytimeAttackNearestEnemy(fates::runtime::native::NativeRuntime&, std::uint16_t);
AiActionResult ExecuteAttackIdle(fates::runtime::native::NativeRuntime&, std::uint16_t, std::uint16_t,
                                 const AiCandidatePreviewProvider*);
AiActionResult ExecuteConfiguredAiAction(fates::runtime::native::NativeRuntime&, std::uint16_t, std::uint16_t,
                                         const AiCandidatePreviewProvider*);
} // namespace fates::ai::native
