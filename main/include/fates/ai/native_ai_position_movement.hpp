#pragma once
#include "fates/runtime/native_runtime.hpp"
#include <cstdint>
namespace fates::ai::native {
// Exact arithmetic slice after GetTerrainScore's category/skill branches.
int OrdinaryTerrainScore(std::int8_t defense,std::int8_t avoid,std::int8_t healing) noexcept;
std::uint32_t OrdinaryMoveToScore(int actual_cost,int reverse_cost,int terrain_score,
                                 int adjacent_allies,int goal_x,int goal_y,int x,int y) noexcept;
enum class AiPositionMoveStatus : std::uint8_t {
    Ok, InvalidUnit, UnsupportedDescriptor, Inactive, UnsupportedProfile,
    InvalidGoal, GoalUnreachable, NoProgress, MissingAiRandom,
    UnsupportedObstacleAction, MovementRejected
};
struct AiPositionMoveResult {
    AiPositionMoveStatus status{AiPositionMoveStatus::InvalidUnit};
    std::uint16_t unit_slot{};
    std::int16_t goal_x{},goal_y{},start_x{},start_y{},end_x{},end_y{};
    std::uint32_t selected_score{},candidate_count{},tie_draws{};
    std::uint64_t ai_rng_draws{};
    bool moved{},reverse_retry_without_hostile_block{};
};
// Bounded ordinary, unpaired, no-skill Position action. Destination comes from
// AIValue arguments. Obstacle combat, special movement and fixed-order modes
// are deliberately not promoted by this entry point.
AiPositionMoveResult ExecuteConfiguredPositionMovement(
    fates::runtime::native::NativeRuntime&,std::uint16_t,std::uint16_t current_turn);
} // namespace fates::ai::native
