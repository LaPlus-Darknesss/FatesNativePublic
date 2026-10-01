#pragma once
#include "fates/runtime/native_runtime.hpp"
#include <array>
#include <span>
#include <vector>
namespace fates::support::native {
inline constexpr std::uint16_t kNoSupportUnit=0xffffu;
// Resolved input kernels. These do not infer relationships, edit/family data,
// map membership, constructor random keys, or combat participation.
bool CanDualExact(std::uint32_t situation_flags,std::uint32_t user_flags,
                  std::uint8_t force_control) noexcept;
std::uint32_t RelianceScoreForDualExact(bool found,std::int32_t level,
                                      std::int32_t points_to_next) noexcept;
std::int32_t PersonAttackStanceTotalExact(const std::array<std::int8_t,20>&,
                                        std::uint8_t capability,std::int32_t level) noexcept;
struct SupportBonuses { std::int16_t hit{},critical{},avoid{},dodge{}; };
void AddPrimarySupportBonuses(SupportBonuses&,const std::array<std::int16_t,4>&) noexcept;
void AddExtraSupportBonuses(SupportBonuses&,const std::array<std::int16_t,4>&) noexcept;
struct SupportImageUnit {
    std::uint16_t slot{};std::int16_t x{},y{};std::uint8_t force{};bool no_dual{};std::uint32_t flags{};
};
// Prevalidated unique-cell image; active bounds are half-open. This traversal
// is shared by the enumerator and calculator, with their different flag gates.
std::vector<std::uint16_t> EnumerateAdjacentSupportExact(std::uint8_t force,int x,int y,
    int min_x,int min_y,int max_x,int max_y,std::span<const SupportImageUnit>,bool enumerator_filter);
struct SupportCandidate {std::uint16_t slot{};std::uint32_t score{},tie_key{};};
std::int32_t SelectSupportCandidateExact(std::span<const SupportCandidate>) noexcept;
std::int32_t DualScoreExact(bool selected,bool has_weapon,bool is_reliance,std::int32_t level) noexcept;

enum class LocalSupportStatus : std::uint8_t {
    Ok, InvalidContext, MissingDefinition, AmbiguousImage, MissingContext,
    StaleContext, MissingRelationship, MissingTieKey, MissingBonusTotals,
    UnsupportedPartner, InvalidEquipment, UnsupportedSourceSkill, InvalidPairTopology
};
struct LocalSupportSelection {
    LocalSupportStatus status{LocalSupportStatus::InvalidContext};
    std::uint16_t selected{kNoSupportUnit}, rejected_source{kNoSupportUnit};
    std::int32_t dual_score{}, reliance_level{};
    std::vector<std::uint16_t> candidates{};
};
struct LocalSupportProjection {
    LocalSupportStatus status{LocalSupportStatus::InvalidContext};
    LocalSupportSelection selection{};
    SupportBonuses bonuses{};
    std::vector<std::uint16_t> secondary_sources{};
    bool selected_has_weapon{};
    std::uint16_t rejected_source{kNoSupportUnit};
};
// Restores explicit resolved persistent inputs only. Atomically validates all
// identities/duplicates/ranges. Does not create Units, alter stats or draw RNG.
LocalSupportStatus RestoreResolvedSupportContext(
    fates::runtime::native::NativeRuntime&,
    const fates::runtime::native::ResolvedSupportContext&);
// AI::Processing removes its actor from the tactical image around thinking
// (0x223A68 -> 0x223A8C -> 0x223AB8). The scoped image projection omits exactly
// that actor; never mutates the live image. At physical positions omit self
// identically: its own tile is outside the enumerator's Manhattan-one scan.
LocalSupportSelection InspectLocalSupportSelection(
    const fates::runtime::native::NativeRuntime&,std::uint16_t subject,
    std::int16_t x,std::int16_t y,std::uint16_t removed_actor);
LocalSupportProjection ProjectLocalSupportBonuses(
    const fates::runtime::native::NativeRuntime&,std::uint16_t subject,
    std::int16_t x,std::int16_t y,std::uint16_t removed_actor);
// Calculation-only paths. They require the same identity-bound support inputs.
// Explicit source bypasses local eligibility/selection, not relationship or
// bonus provenance. Native pair topology is validated; no live state is changed.
LocalSupportProjection ProjectSpecifiedSupportBonuses(
    const fates::runtime::native::NativeRuntime&,std::uint16_t subject,
    std::uint16_t source,std::int16_t x,std::int16_t y,std::uint16_t removed_actor);
LocalSupportProjection ProjectBattleSupportBonuses(
    const fates::runtime::native::NativeRuntime&,std::uint16_t subject,
    std::int16_t x,std::int16_t y,std::uint16_t removed_actor);
}
