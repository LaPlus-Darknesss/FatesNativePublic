#pragma once
#include "fates/runtime/native_runtime.hpp"
#include "fates/battle/native_battle_postcombat.hpp"
#include "fates/battle/native_around_skills.hpp"
#include "fates/support/native_local_support.hpp"
#include <array>
#include <cstdint>
#include <vector>
namespace fates::battle::native {
enum class BattleTransactionStatus : std::uint8_t {
    Ok, InvalidUnit, AlliedTarget, MissingCombatState, MissingDefinition,
    UnsupportedComplexRules, UnsupportedPairProjection, UnsupportedGuardResolution, UnsupportedPairDefeatResolution, UnsupportedItem, OutOfRange, MissingGameRngState, UnsupportedAroundProjection, UnsupportedLocalSupportProjection, UnsupportedAttackStance
};
struct BattlePreviewSide {
    // Semantic simulator equipment: ID 0 is legitimate for a defending Unit.
    // Distinguish absence from an armed unit that cannot counter at this range.
    bool has_attack_weapon{};
    std::uint16_t equipped_item_id{};
    std::uint32_t condition_flags{};
    std::uint16_t unit_category_mask{};
    bool terrain_bonus_applied{};
    AroundSkillProjection around{};
    bool local_support_context_applied{};
    fates::support::native::LocalSupportProjection local_support{};
    std::int32_t attack{}, hit{}, avoid{}, critical{}, dodge{}, defense{}, resistance{};
    bool uses_magic{};
    bool effective{};
    std::int32_t continuous{}, under_continuous{};
    std::int32_t simple_hit{}, simple_critical{}, simple_damage{}, simple_damage_rate{};
    std::uint8_t attack_count{};
    bool guard_stance_pair{};
    std::uint16_t pair_partner_slot{0xFFFFu};
    std::uint32_t guard_progress_sum{};
    std::array<std::int16_t,8> pair_capability_bonus{};
};
// An assist is never the counterattack target. Forced previews also admit
// validated paired sources under their private BattleInfo calculation context.
// All identity and relationship inputs remain owned by ResolvedSupportContext.
struct BattleAssistPreview {
    bool present{};
    bool active{};
    std::uint16_t unit_slot{0xFFFFu};
    std::uint16_t primary_slot{0xFFFFu};
    BattlePreviewSide side{};
    // The opposing primary's defensive lane is recalculated against THIS
    // source, not reused from its matchup against the other primary.
    std::int32_t opposing_avoid{}, opposing_dodge{}, opposing_defense{}, opposing_resistance{};
};
struct BattlePreviewResult {
    BattleTransactionStatus status{BattleTransactionStatus::InvalidUnit};
    std::uint8_t movement_rule_byte{1}; // preserved preparation metadata; no counter interpretation
    std::uint8_t distance{};
    BattlePreviewSide attacker{}, defender{};
    std::array<BattleAssistPreview,2> assists{};
};
struct BattleStrike {
    bool support_strike{};
    std::uint8_t battle_side{};
    std::uint16_t actor_slot{}, target_slot{};
    std::uint32_t hit_roll{};
    std::uint32_t hit_threshold{};
    bool hit{};
    bool critical_roll_consumed{};
    std::uint32_t critical_roll{};
    bool critical{};
    std::int32_t damage{};
    std::int16_t hp_before{}, hp_after{};
    bool defeated{};
    bool guard_stance_intercepted{};
    std::uint8_t target_guard_progress_before{};
    std::uint8_t target_guard_progress_after{};
    std::uint8_t actor_guard_progress_before{};
    std::uint8_t actor_guard_progress_after{};
};
struct PairSeparationRecord {
    std::uint16_t defeated_lead_slot{};
    std::uint16_t surviving_partner_slot{};
    std::int16_t partner_x{};
    std::int16_t partner_y{};
};
struct BattleTransactionResult {
    BattleTransactionStatus status{BattleTransactionStatus::InvalidUnit};
    std::uint8_t distance{};
    BattlePreviewSide attacker{}, defender{};
    std::array<BattleAssistPreview,2> assists{};
    std::vector<BattleStrike> strikes{};
    std::vector<PostCombatEffectRecord> post_combat{};
    std::vector<PairSeparationRecord> pair_separations{};
    std::uint64_t game_rng_draws{};
};
std::uint32_t RetailHybridHitThreshold(std::int32_t displayed_hit) noexcept;
BattlePreviewResult PreviewOrdinaryBattle(const fates::runtime::native::NativeRuntime& runtime,
                                          std::uint16_t attacker_slot,
                                          std::uint16_t defender_slot);
BattlePreviewResult PreviewOrdinaryBattleAt(const fates::runtime::native::NativeRuntime& runtime,
                                            std::uint16_t attacker_slot,
                                            std::uint16_t defender_slot,
                                            std::int16_t attacker_x,
                                            std::int16_t attacker_y);
// Projects the selected current inventory instance without equipping/reordering
// the live Unit. Used for per-weapon AI trials; index is distinct from candidate ID.
BattlePreviewResult PreviewOrdinaryBattleAtInventory(
    const fates::runtime::native::NativeRuntime&,std::uint16_t actor,std::uint16_t target,
    std::int16_t x,std::int16_t y,std::uint8_t inventory_index);
struct ForcedSourceBattleRequest {
    std::uint16_t attacker{},defender{},source{};
    std::int16_t attack_x{},attack_y{};
    std::uint8_t attacker_item_index{},source_item_index{};
};
// Calculation only. Selects existing semantic inventory IDs on a private copy,
// forces side0 source and side2 inventory, and preserves the source's actual map
// position. Ordinary supported items/skills; no clone or inventory slots 5..9.
// Uses the same detail/simple/assist owners as ordinary forecast and execution.
BattlePreviewResult PreviewBattleWithForcedSource(
    const fates::runtime::native::NativeRuntime&,const ForcedSourceBattleRequest&);
// Execute on a private game image. Refusal publishes no damage, RNG, pair
// effects or committed-effect records, including failures after a lethal strike.
BattleTransactionResult ExecuteOrdinaryBattle(fates::runtime::native::NativeRuntime& runtime,
                                               std::uint16_t attacker_slot,
                                               std::uint16_t defender_slot);
BattleTransactionResult ExecuteOrdinaryPhysicalBattle(fates::runtime::native::NativeRuntime& runtime,
                                                       std::uint16_t attacker_slot,
                                                       std::uint16_t defender_slot);
}
