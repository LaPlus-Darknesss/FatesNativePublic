#pragma once
#include "fates/ai/native_ai_action.hpp"
#include "fates/ai/native_ai_attack_viability.hpp"
#include <cstdint>
#include <span>
#include <vector>

namespace fates::ai::native {
// Ordinary bounded GetAttackPosition flags=0. Sentinel ranges and Sigh of Death
// use other branches. Unsigned arithmetic preserves the retail packed score.
std::uint32_t OrdinaryImmediatePositionScore(std::uint16_t cost,
    std::uint8_t terrain_score,std::uint8_t dual_score,bool outside_counter_range) noexcept;
struct AiImmediatePosition {
    std::int16_t x{},y{},cost{};
    std::uint8_t terrain_score{},dual_score{};
    bool outside_counter_range{};
};
struct AiImmediatePositionChoice {
    bool selected{},missing_ai_random{};
    AiImmediatePosition cell{};
    std::uint32_t score{},ties{};
};
AiImmediatePositionChoice SelectImmediatePositionExact(std::span<const AiImmediatePosition>,
    fates::runtime::native::NativeGameState&);

struct AiImmediateTargetChoice {
    bool selected{},missing_ai_random{};
    std::uint16_t target{};
    AiPlanningWeaponCandidate weapon{};
    float cannon_score_lane0{};
    std::uint32_t ties{};
};
void ConsiderImmediateTargetExact(AiImmediateTargetChoice&,std::uint16_t,
    const AiPlanningWeaponCandidate&,float,fates::runtime::native::NativeGameState&);

// AICannon::Think has an early bitwise comparison against +0.5f. True means
// ONLY that the cannon alternative is rejected without reading its world state.
// The caller supplies the resolved +0x8D4 score lane, never a trust boolean.
bool CannonAlternativeRejectedByScore(float resolved_lane0) noexcept;

enum class AiImmediateAttackStatus : std::uint8_t {
    Ready, InvalidUnit, NoImmediateGeometry, UnsupportedProfile, MissingForceOrder,
    InvalidEquipment, UnsupportedSupportProjection, MissingAiRandom,
    PreviewRejected, NoViableWeapon, CannonArbitrationRequired,
    DualArbitrationRequired, MovementRejected, BattleRejected, Committed
};
struct AiImmediateAttackTrace {
    std::uint32_t targets{},inventory_visits{},position_cells{},projections{},power0_rejections{};
    std::uint32_t position_ties{},weapon_ties{},target_ties{};
    std::uint16_t support_subject{0xffffu},support_source{0xffffu};
    std::int16_t support_x{},support_y{};
    std::uint8_t support_status{0xffu};
    std::uint32_t support_queries{},nonzero_support_positions{};
    std::uint16_t last_target{0xffffu},last_item{},target{0xffffu},item{};
    std::uint8_t inventory_slot{0xffu},projection_status{0xffu},preview_status{0xffu};
    std::int16_t x{},y{};
    std::uint32_t score{};
    float cannon_score_lane0{};
    std::uint64_t attempted_ai_draws{};
    std::uint8_t scenario_cannon_status{0xffu};
    bool script_cannon_absence{},scenario_dual_early_reject{};
};
struct AiImmediateAttackPlan {
    AiImmediateAttackStatus status{AiImmediateAttackStatus::InvalidUnit};
    AiImmediateAttackTrace trace{};
};
// Read-only diagnostic. Replays selection using a private RNG copy; its result
// cannot be submitted as authorization. Execute recomputes on current state.
AiImmediateAttackPlan InspectOrdinaryImmediateAttack(
    const fates::runtime::native::NativeRuntime&,std::uint16_t);
// Fresh ordinary, unpaired force-1 Attack/NearestEnemy subset. A successful
// weapon plan still has to pass cannon and support alternative arbitration.
namespace detail {
// The configured action owner has already checked its activation condition.
AiActionResult ExecuteOrdinaryImmediateAttack(
    fates::runtime::native::NativeRuntime&,std::uint16_t);
}
} // namespace fates::ai::native
