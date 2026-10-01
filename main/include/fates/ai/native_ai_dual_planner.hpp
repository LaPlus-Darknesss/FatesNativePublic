#pragma once
#include "fates/runtime/native_game_state.hpp"
#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>
namespace fates::ai::native {
// Resolved input contract, NOT a certificate to execute an attack. The force
// order, alliance image and service results need their own authoritative owners.
// AIDual queues a separate Mind move; this planner does not commit that action.
constexpr std::uint16_t kNoDualUnit=0xffffu;
enum class AiDualCell : std::uint8_t { Empty, Allied, Hostile, Unknown=255 };
struct AiDualUnitFacts {
    std::uint16_t slot{kNoDualUnit};
    std::uint32_t public_flags{},policy_flags{};
    bool no_dual{}; // resolved Unit | Person | Job private NoDual bit
    std::optional<std::uint16_t> partner_slot;
};
struct AiDualPlanningInput {
    std::uint16_t actor{kNoDualUnit},target{kNoDualUnit};
    std::int16_t actor_x{},actor_y{},attack_x{},attack_y{};
    std::uint8_t weapon_index{},difficulty{};
    bool actor_no_dual{},can_dual{};
    std::int16_t min_x{},min_y{},max_x{},max_y{}; // exclusive upper bounds
    std::array<AiDualCell,1024> image{}; // alliance relative to actor force
    std::span<const AiDualUnitFacts> force_order;
    fates::runtime::native::NativeRandomState ai_random{};
};
struct AiDualMoveField { std::array<std::int32_t,1024> cost{}; };
struct AiDualBattleFacts {
    std::int32_t power{},hit{},critical{},damage_rate{},attack_count{};
};
struct AiDualPreviewRequest {
    std::uint16_t actor{},target{},source{};
    std::int16_t attack_x{},attack_y{};
    std::uint8_t actor_weapon{},source_weapon{};
};
// Read-only, deterministic services. nullopt means unresolved, never false/zero.
// Movement is UnitAIMove(unit,-1,2,0); partner trials reuse that lead's field.
// Preview explicitly forces the source in side 0's dual slot and its inventory
// index in side 2. It must NOT substitute the automatically selected local source.
class AiDualPlanningServices {
public:
    virtual ~AiDualPlanningServices()=default;
    virtual std::optional<bool> AttackPermission(std::uint16_t,std::uint16_t) const=0;
    virtual std::optional<bool> ActiveCrossfireCommand(std::uint16_t,std::uint16_t) const=0;
    virtual std::optional<AiDualMoveField> Movement(std::uint16_t) const=0;
    virtual std::optional<std::int32_t> TerrainCost(std::uint16_t,std::int16_t,std::int16_t) const=0;
    virtual std::optional<bool> CanEquip(std::uint16_t,std::uint8_t) const=0;
    virtual std::optional<AiDualBattleFacts> Preview(const AiDualPreviewRequest&) const=0;
};
enum class AiDualPlanStatus : std::uint8_t { Complete,InvalidInput,MissingWorld,MissingService,MissingAiRandom };
enum class AiDualQueryKind : std::uint8_t { Permission=1,Command,Movement,Terrain,Equip,Preview,Random };
struct AiDualQuery { AiDualQueryKind kind{};std::uint32_t unit{},first{},second{}; };
struct AiDualChoice {
    std::uint16_t unit{kNoDualUnit}; // lead whose Mind move would be queued
    std::int16_t x{},y{};
    std::uint8_t inventory_index{}; // 0..4 lead, 5..9 partner
    std::uint32_t score{};
};
struct AiDualPlan {
    AiDualPlanStatus status{AiDualPlanStatus::InvalidInput};
    std::optional<AiDualChoice> choice;
    fates::runtime::native::NativeRandomState next_ai_random{};
    std::uint32_t landing_cells{},position_ties{},weapon_ties{},attempted_ai_draws{};
    std::vector<AiDualQuery> queries;
};
// Wrap every multiply/add as ARM uint32, then unsigned divide by 100. Zero
// attack count or zero wrapped power*hit rejects the score before tie handling.
std::optional<std::uint32_t> DualAttackScoreExact(const AiDualBattleFacts&) noexcept;
// No input mutation. On any unresolved service/RNG/world input, discard the
// provisional choice and RNG; diagnostic attempted draws/queries remain visible.
AiDualPlan PlanWholeForceDualExact(const AiDualPlanningInput&,const AiDualPlanningServices&);
}
