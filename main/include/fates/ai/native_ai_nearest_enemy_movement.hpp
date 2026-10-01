#pragma once
#include "fates/runtime/native_runtime.hpp"
#include <cstdint>
#include <span>
#include <vector>
namespace fates::ai::native {
enum class AiNearestEnemyMoveStatus : std::uint8_t {
    Ok, InvalidUnit, MissingItem, UnsupportedProfile, NoHostileTarget,
    NoPlanningAttackPosition, AmbiguousRetailTargetOrder, MissingAiRandom,
    NoMovementDestination, MovementRejected, UnsupportedAttackViability,
    UnsupportedItemRange, UnsupportedStaffTargeting, UnsupportedObstacleAction,
    UnsupportedTargetPermission
};
struct AiPlanningAttackCell {std::int16_t x{},y{},cost{};};
struct AiNearestPlanningTarget {
    std::uint16_t target_slot{};
    std::uint8_t history_count{};
    std::vector<AiPlanningAttackCell> cells; // row-major; occupancy ignored by retail flags 6
};
struct AiNearestEnemyPlanning {
    AiNearestEnemyMoveStatus status{AiNearestEnemyMoveStatus::InvalidUnit};
    std::uint32_t range_mask{};
    std::uint8_t move_power{};
    bool requires_attack_viability{},planning_blocks_hostiles{};
    std::vector<std::uint16_t> usable_inventory_ids;
    std::vector<AiNearestPlanningTarget> targets;
};
struct AiNearestViabilityTrace {
    std::uint8_t last_around_status{0xffu},last_around_side{0xffu};
    std::uint16_t last_around_source{0xffffu},last_around_skill{};
    std::uint16_t target_checks{},weapon_previews{},power0_rejections{};
    std::uint16_t weapon_position_ties{},weapon_score_ties{};
    std::uint16_t last_target{0xFFFFu},last_item{},selected_item{};
    std::uint8_t last_projection_status{0xFFu},last_preview_status{0xFFu};
    // Attempts are diagnostic only. A refused action publishes no live RNG.
    std::uint64_t attempted_ai_draws{};
};
struct AiNearestEnemyMoveResult {
    AiNearestViabilityTrace viability{};
    AiNearestEnemyMoveStatus status{AiNearestEnemyMoveStatus::InvalidUnit};
    std::uint16_t unit_slot{},target_slot{};
    std::int16_t start_x{},start_y{},end_x{},end_y{};
    bool moved{};
    std::int32_t target_score{};
    std::uint16_t target_equal_score_encounters{},planning_position_tie_draws{},movement_tile_tie_draws{};
    std::uint64_t ai_rng_draws{};
    bool planning_retry_without_hostile_block{},fallback_to_attack_position{};
};
// Diagnostic geometry only: no RNG, no final target, and no battle permission
// is inferred from a reachable cell. Force-list order is derived from state.
AiNearestEnemyPlanning InspectNearestEnemyPlanning(const fates::runtime::native::NativeRuntime&,std::uint16_t);
// Ordinary ActionMoveAttackRange score after resolved path cost and move power.
std::uint32_t OrdinaryNearestTargetScore(std::uint8_t history,std::uint16_t cost,std::uint8_t move_power) noexcept;
struct AiNearestTargetSelection {
    AiNearestEnemyMoveStatus status{AiNearestEnemyMoveStatus::NoPlanningAttackPosition};
    std::uint16_t target_slot{};std::int16_t attack_x{},attack_y{};
    std::uint32_t score{0x6500u};
    std::uint16_t position_ties{},target_ties{};
};
struct AiPlanningPositionSelection {
    AiNearestEnemyMoveStatus status{AiNearestEnemyMoveStatus::NoPlanningAttackPosition};
    std::int16_t x{},y{},cost{};
    std::uint16_t ties{};
};
AiPlanningPositionSelection SelectNearestPlanningPosition(
    std::span<const AiPlanningAttackCell>, fates::runtime::native::NativeGameState&);
// Pure ordering/selection kernel over an ALREADY PROVEN ordered geometry list.
// Uses an isolated RNG state; caller publishes it only with the successful action.
AiNearestTargetSelection SelectNearestTargetGeometry(std::span<const AiNearestPlanningTarget>,std::uint8_t,
                                                     fates::runtime::native::NativeGameState&);
// Bounded generic movement. Required viability uses the shared ordinary preview
// and ordered per-weapon selection; unowned preview/partner branches fail closed.
AiNearestEnemyMoveResult ExecuteNearestEnemyMovementOnly(fates::runtime::native::NativeRuntime&,std::uint16_t);
// Source compatibility only. The old caller trust boolean no longer authorizes
// multi-target order. This delegates to the same generic production operation.
AiNearestEnemyMoveResult ExecuteB007NearestEnemyMovementOnly(fates::runtime::native::NativeRuntime&,std::uint16_t,
                                                           bool retail_target_enumeration_order_exact=false);
}
