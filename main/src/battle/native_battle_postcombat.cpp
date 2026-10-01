#include "fates/battle/native_battle_postcombat.hpp"
#include "fates/battle/native_around_skills.hpp"
#include <algorithm>
#include <array>
#include <cstdlib>
namespace fates::battle::native {
namespace {
using fates::runtime::native::DefinitionStore;
using fates::runtime::native::UnitState;
constexpr std::array<std::uint16_t,15> kSupported = {
    kSkillDraconicHex, kSkillSealStrength, kSkillSealMagic, kSkillSealSpeed,
    kSkillSealDefense, kSkillSealResistance, kSkillPoisonStrike, kSkillSavageBlow,
    kSkillGrislyWound, kSkillInevitableEnd, kSkillWaryFighter, kSkillDragonskin,
    kSkillDivineShield, kSkillStatusResistance, kSkillStatusImmunity
};
bool AllowedSkill(const std::uint16_t id) noexcept {
    return id == 0 || IsSupportedLocalAroundSkill(id) || std::find(kSupported.begin(), kSupported.end(), id) != kSupported.end();
}
std::uint8_t IncomingAdjusted(const UnitState& target, const std::uint8_t v) noexcept {
    if (UnitHasSkill(target, kSkillStatusImmunity)) return 0;
    if (UnitHasSkill(target, kSkillStatusResistance)) return static_cast<std::uint8_t>(v >> 1);
    return v;
}
void MergeWeakness(const UnitState& source, UnitState& target, const std::uint8_t lane,
                   std::uint8_t v, const std::uint16_t source_slot,
                   const std::uint16_t target_slot, const std::uint16_t skill_id,
                   std::vector<PostCombatEffectRecord>& out) {
    v = IncomingAdjusted(target, v);
    if (!v || lane >= target.weakness.size()) return;
    const auto before = target.weakness[lane];
    if (UnitHasSkill(source, kSkillInevitableEnd)) {
        target.weakness[lane] = static_cast<std::uint8_t>(
            std::min(99, static_cast<int>(before) + v));
    } else {
        target.weakness[lane] = std::max(before, v);
    }
    if (target.weakness[lane] != before) {
        out.push_back({PostCombatEffectKind::WeaknessMerge, source_slot, target_slot,
                       skill_id, 0, lane, v, 0, 0});
    }
}
void AddSelfWeakness(UnitState& unit, const std::uint8_t lane, const std::uint8_t v,
                     const std::uint16_t slot, const std::uint16_t item,
                     std::vector<PostCombatEffectRecord>& out) {
    if (lane >= unit.weakness.size()) return;
    const auto before = unit.weakness[lane];
    unit.weakness[lane] = static_cast<std::uint8_t>(
        std::min(99, static_cast<int>(before) + v));
    if (unit.weakness[lane] != before) {
        out.push_back({PostCombatEffectKind::SelfItemWeakness, slot, slot, 0,
                       item, lane, v, 0, 0});
    }
}
int PoisonPercentAfterResistance(const UnitState& target, const int pct) noexcept {
    if (UnitHasSkill(target, kSkillDragonskin) || UnitHasSkill(target, kSkillDivineShield) ||
        UnitHasSkill(target, kSkillStatusImmunity)) return 0;
    if (UnitHasSkill(target, kSkillStatusResistance)) return pct >> 1;
    return pct;
}
void ApplyNonlethalPercent(const UnitState& source, UnitState& target, int pct,
                           const std::uint16_t source_slot, const std::uint16_t target_slot,
                           const std::uint16_t skill, const PostCombatEffectKind kind,
                           std::vector<PostCombatEffectRecord>& out) {
    (void)source;
    pct = PoisonPercentAfterResistance(target, pct);
    if (pct <= 0 || target.defeated || target.current_hp <= 1 || target.max_hp <= 0) return;
    const int before = target.current_hp;
    int damage = static_cast<int>(target.max_hp) * pct / 100;
    damage = std::min(damage, before - 1);
    if (damage <= 0) return;
    target.current_hp = static_cast<std::int16_t>(before - damage);
    out.push_back({kind, source_slot, target_slot, skill, 0, 0,
                   static_cast<std::uint8_t>(pct), static_cast<std::int16_t>(before),
                   target.current_hp});
}
void ApplySealVector(const UnitState& source, UnitState& target,
                     const std::uint16_t source_slot, const std::uint16_t target_slot,
                     std::vector<PostCombatEffectRecord>& out) {
    if (source.defeated || target.defeated) return;
    if (UnitHasSkill(source, kSkillDraconicHex)) {
        for (std::uint8_t lane = 1; lane < 8; ++lane) {
            MergeWeakness(source, target, lane, 4, source_slot, target_slot,
                          kSkillDraconicHex, out);
        }
    }
    const auto do_skill = [&](const std::uint16_t skill_id, const std::uint8_t lane) {
        if (UnitHasSkill(source, skill_id)) {
            MergeWeakness(source, target, lane, 6, source_slot, target_slot, skill_id, out);
        }
    };
    do_skill(kSkillSealStrength, 1);
    do_skill(kSkillSealMagic, 2);
    do_skill(kSkillSealSpeed, 4);
    do_skill(kSkillSealDefense, 6);
    do_skill(kSkillSealResistance, 7);
}
void ApplyLowerStatsItem(const DefinitionStore& definitions, UnitState& unit,
                         const std::uint16_t slot,
                         std::vector<PostCombatEffectRecord>& out) {
    if (unit.defeated) return;
    const auto* item = definitions.FindItem(unit.equipped_item_id);
    if (!item || (item->bitflags[4] & 0x01u) == 0) return;
    const auto lane = static_cast<std::uint8_t>(definitions.IsMagicItem(*item) ? 2 : 1);
    AddSelfWeakness(unit, lane, 2, slot, item->id, out);
    AddSelfWeakness(unit, 3, 2, slot, item->id, out);
}
} // namespace

bool UnitHasSkill(const UnitState& unit, const std::uint16_t skill_id) noexcept {
    return skill_id != 0 &&
           std::find(unit.equipped_skill_ids.begin(), unit.equipped_skill_ids.end(), skill_id) !=
               unit.equipped_skill_ids.end();
}
bool PostCombatSkillSetSupported(const UnitState& unit) noexcept {
    for (const auto id : unit.equipped_skill_ids) if (!AllowedSkill(id)) return false;
    return true;
}
void ApplyBattlePostCombatEffects(fates::runtime::native::NativeRuntime& runtime,
                                  const std::uint16_t attacker_slot,
                                  const std::uint16_t defender_slot,
                                  const std::uint8_t distance,
                                  std::vector<PostCombatEffectRecord>& out) {
    auto& attacker = runtime.game.units[attacker_slot];
    auto& defender = runtime.game.units[defender_slot];
    int to_defender = 0;
    std::uint16_t direct_skill = 0;
    if (!attacker.defeated && !defender.defeated) {
        if (UnitHasSkill(attacker, kSkillPoisonStrike)) {
            to_defender += 20;
            direct_skill = kSkillPoisonStrike;
        }
        if (distance < 3 && UnitHasSkill(attacker, kSkillSavageBlow)) {
            to_defender += 20;
            direct_skill = kSkillSavageBlow;
        }
        if (UnitHasSkill(attacker, kSkillGrislyWound)) {
            to_defender += 20;
            direct_skill = kSkillGrislyWound;
        }
        if (to_defender) {
            ApplyNonlethalPercent(attacker, defender, to_defender, attacker_slot,
                                  defender_slot, direct_skill,
                                  PostCombatEffectKind::DirectPercentDamage, out);
        }
        if (UnitHasSkill(defender, kSkillGrislyWound)) {
            ApplyNonlethalPercent(defender, attacker, 20, defender_slot, attacker_slot,
                                  kSkillGrislyWound,
                                  PostCombatEffectKind::DirectPercentDamage, out);
        }
    }
    if (!attacker.defeated && distance < 3 && UnitHasSkill(attacker, kSkillSavageBlow)) {
        for (std::uint16_t i = 0; i < runtime.game.units.size(); ++i) {
            if (i == attacker_slot || i == defender_slot) continue;
            auto& target = runtime.game.units[i];
            if (!target.occupied || target.defeated || !target.has_position ||
                target.force_type == attacker.force_type || !fates::runtime::native::IsTacticalForce(target.force_type)) continue;
            const int manhattan = std::abs(static_cast<int>(attacker.x) - target.x) +
                                  std::abs(static_cast<int>(attacker.y) - target.y);
            if (manhattan >= 1 && manhattan <= 2) {
                ApplyNonlethalPercent(attacker, target, 20, attacker_slot, i,
                                      kSkillSavageBlow,
                                      PostCombatEffectKind::AreaPercentDamage, out);
            }
        }
    }
    ApplySealVector(attacker, defender, attacker_slot, defender_slot, out);
    ApplySealVector(defender, attacker, defender_slot, attacker_slot, out);
    ApplyLowerStatsItem(runtime.definitions, attacker, attacker_slot, out);
    ApplyLowerStatsItem(runtime.definitions, defender, defender_slot, out);
}
} // namespace fates::battle::native
