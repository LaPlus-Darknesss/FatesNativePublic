#pragma once
#include "fates/runtime/native_runtime.hpp"
#include <cstdint>
#include <vector>
namespace fates::battle::native {
// Exact FE14 SkillDefinition IDs verified from original GameData.bin.lz.
inline constexpr std::uint16_t kSkillDraconicHex = 10;       // SEID_竜呪
inline constexpr std::uint16_t kSkillSealStrength = 11;      // SEID_力封じ
inline constexpr std::uint16_t kSkillSealMagic = 12;         // SEID_魔力封じ
inline constexpr std::uint16_t kSkillSealSpeed = 13;         // SEID_速さ封じ
inline constexpr std::uint16_t kSkillSealDefense = 14;       // SEID_守備封じ
inline constexpr std::uint16_t kSkillSealResistance = 15;    // SEID_魔防封じ
inline constexpr std::uint16_t kSkillWaryFighter = 64;       // SEID_守備隊形
inline constexpr std::uint16_t kSkillPoisonStrike = 101;     // SEID_蛇毒
inline constexpr std::uint16_t kSkillSavageBlow = 102;       // SEID_死の吐息
inline constexpr std::uint16_t kSkillGrislyWound = 103;      // SEID_四牙
inline constexpr std::uint16_t kSkillDragonskin = 114;       // SEID_竜鱗
inline constexpr std::uint16_t kSkillDivineShield = 115;     // SEID_神盾
inline constexpr std::uint16_t kSkillStatusResistance = 118; // SEID_状態異常耐性
inline constexpr std::uint16_t kSkillStatusImmunity = 119;   // SEID_状態異常無効
inline constexpr std::uint16_t kSkillInevitableEnd = 127;    // SEID_負の連鎖

enum class PostCombatEffectKind : std::uint8_t { DirectPercentDamage, AreaPercentDamage, WeaknessMerge, SelfItemWeakness };
struct PostCombatEffectRecord {
    PostCombatEffectKind kind{};
    std::uint16_t source_slot{}, target_slot{};
    std::uint16_t source_skill_id{};
    std::uint16_t source_item_id{};
    std::uint8_t lane{};
    std::uint8_t amount{};
    std::int16_t hp_before{}, hp_after{};
};
bool UnitHasSkill(const fates::runtime::native::UnitState& unit,
                  std::uint16_t skill_id) noexcept;
bool PostCombatSkillSetSupported(const fates::runtime::native::UnitState& unit) noexcept;
void ApplyBattlePostCombatEffects(fates::runtime::native::NativeRuntime& runtime,
                                  std::uint16_t attacker_slot,
                                  std::uint16_t defender_slot,
                                  std::uint8_t distance,
                                  std::vector<PostCombatEffectRecord>& out);
}
