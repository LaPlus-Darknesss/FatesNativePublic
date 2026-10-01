#pragma once
#include "fates/runtime/native_runtime.hpp"
#include <cstdint>
#include <vector>

namespace fates::battle::native {
// USA SE Skill IDs, bound by the pass's original Skill table and code witnesses.
inline constexpr std::uint16_t kSkillMaleficAura = 76;
inline constexpr std::uint16_t kSkillHeartseeker = 89;
inline constexpr std::uint16_t kSkillLilysPoise = 186;
inline constexpr std::uint16_t kSkillMisfortune = 187;
bool LocalAuraForcesAllied(std::uint8_t a, std::uint8_t b) noexcept;
bool IsSupportedLocalAroundSkill(std::uint16_t id) noexcept;
bool IsKnownLocalAroundSkill(std::uint16_t id) noexcept;

// These are calculator addends, not modifications to permanent Unit capability.
// The applied flags project the corresponding bits of CalculateAround's +0x32
// applied-skill set. Equal-skill sources do not stack within one calculation.
struct AroundSkillModifiers {
    std::int32_t resistance_addend{};
    std::int16_t avoid_addend{};
    bool malefic_applied{};
    bool heartseeker_applied{};
    std::int32_t attack_addend{}, defense_resistance_addend{};
    std::int16_t dodge_addend{};
    bool lilys_poise_applied{}, misfortune_applied{};
};
// The hostile contributions only; alliance, source skills and distance are resolved
// inputs. This kernel also preserves the original word/halfword narrowing.
void ApplyHostileAroundSkill(AroundSkillModifiers& out, std::uint16_t skill,
                            std::int32_t distance, bool source_has_skill) noexcept;

// Calculate applies the bearer penalty before the local source scan. Its
// applied bit is shared with the outward aura and suppresses a second source.
void ApplySelfMisfortune(AroundSkillModifiers& out, bool self_has_skill) noexcept;

void ApplyAlliedAroundSkill(AroundSkillModifiers& out, std::uint16_t skill,
                           std::int32_t distance, bool source_has_skill) noexcept;

enum class AroundProjectionStatus : std::uint8_t {
    Ok, InvalidContext, MissingTerrain, AmbiguousCell, UnsupportedAlliance,
    UnsupportedAroundSkill, UnresolvedPersonalSkill, MissingSourceDefinition
};
struct AroundSkillSource {
    std::uint16_t slot{};
    std::int16_t x{}, y{};
    std::uint8_t distance{};
    bool explicit_opponent{};
    bool applied_malefic{}, applied_heartseeker{}, applied_lilys_poise{}, applied_misfortune{};
};
struct AroundSkillProjection {
    AroundProjectionStatus status{AroundProjectionStatus::InvalidContext};
    AroundSkillModifiers modifiers{};
    std::uint16_t rejected_source{0xffffu};
    std::uint16_t rejected_skill{};
    std::vector<AroundSkillSource> sources{};
};
// Projects the local Calculate() / CalculateAround() subset: explicit opponent
// first, then the active-rectangle-clipped Manhattan diamond in Y/X order.
// Coordinates are simulator coordinates. The authoritative tactical image is
// never moved. The explicit opponent and its partner are omitted from the map
// scan, so a hypothetical attacker is not also seen at its old tactical tile.
// Source skills come from the native equipped set, plus a uniform Personal
// local-aura entry. Difficulty-dependent Personal local auras fail closed.
// Other potentially applicable local auras are not promoted. Force alliance
// follows the original equal-force / 0-and-2 rule (not numeric inequality).
// The unowned force-wide hit provider (204) is refused even outside this radius.
AroundSkillProjection ProjectLocalAroundSkills(
    const fates::runtime::native::NativeRuntime& runtime,
    std::uint16_t self_slot, std::uint16_t opponent_slot,
    std::int16_t self_x, std::int16_t self_y,
    std::int16_t opponent_x, std::int16_t opponent_y);
// Private BattleInfo participant view: pair partners may be explicit sides.
// Physical map occupancy still uses the unchanged lead/partner roles.
AroundSkillProjection ProjectBattleAroundSkills(
    const fates::runtime::native::NativeRuntime&,std::uint16_t self,std::uint16_t opponent,
    std::int16_t self_x,std::int16_t self_y,std::int16_t opponent_x,std::int16_t opponent_y);
}
