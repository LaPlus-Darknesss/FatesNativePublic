#include "fates/unit/native_unit_semantics.hpp"

#include <algorithm>

namespace fates::unit::native {
namespace {
int FloorZero(int value) { return std::max(0, value); }
}

int ClampFinalCapability(int value) { return std::clamp(value, 0, 99); }

int ResolveBaseCapability(const BaseCapabilityInputs& in) {
    int value = in.personBase + in.jobBase + in.unitStored + in.editBonus;
    if (in.subtractStoredPenalty) value -= in.storedPenalty;
    value = std::max(0, value);
    if (value > in.retailLimit) value = in.retailLimit;
    return value;
}

int CapabilityRating(const std::array<int, 8>& finalCapabilities) {
    int total = 0;
    for (std::size_t i = 1; i < finalCapabilities.size(); ++i) total += finalCapabilities[i];
    return total;
}

int ResolveMhp(int baseCapability, const MhpPostInputs& in) {
    int value = baseCapability;
    if (in.halveBeforeAddends) value -= value / 2;
    if (in.maxHpPlus5Skill) value += 5;
    if (in.medicineStatus) value += in.medicineBoostSkill ? 7 : 5;
    if (in.statusPlus2) value += 2;
    if (in.statusMinus1) value -= 1;
    return ClampFinalCapability(value);
}

int ResolveStrength(int baseCapability, const StrengthPostInputs& in) {
    int value = baseCapability;
    if (in.halveBeforeAddends) value -= value / 2;
    value += in.pairBonus;
    if (in.strengthPlus2Skill) value += 2;
    if (in.strongBladePlus3Skill) value += 3;
    value += in.absorbBonus;
    if (in.statusPlus4A) value += 4;
    if (in.sharedStatusPlus2) value += 2;
    if (in.statusPlus1) value += 1;
    if (in.medicineStatus) value += in.medicineBoostSkill ? 3 : 2;
    if (in.statusPlus4B) value += 4;
    if (in.statusPlus2B) value += 2;
    if (in.statusMinus1) value -= 1;
    value += in.itemStatAddend;
    value += in.enhancementAddend;
    if (in.applyDebuff) value -= in.debuffAmount;
    return ClampFinalCapability(value);
}

int ResolveTech(int baseCapability, const TechPostInputs& in) {
    int value = baseCapability + in.pairBonus;
    if (in.techPlus2Skill) value += 2;
    value += in.absorbBonus;
    if (in.statusPlus4A) value += 4;
    if (in.sharedStatusPlus2) value += 2;
    if (in.statusPlus3) value += 3;
    if (in.medicineStatus) value += in.medicineBoostSkill ? 3 : 2;
    if (in.statusPlus4B) value += 4;
    if (in.statusPlus2B) value += 2;
    if (in.statusMinus1) value -= 1;
    value += in.itemStatAddend;
    value += in.enhancementAddend;
    if (in.applyDebuff) value -= in.debuffAmount;
    return ClampFinalCapability(value);
}

int ResolveDefense(int baseCapability, const DefensePostInputs& in) {
    int value = baseCapability + in.pairBonus;
    if (in.defensePlus2Skill) value += 2;
    if (in.finesseMinus1Skill) value -= 1;
    value += in.absorbBonus;
    if (in.statusPlus4A) value += 4;
    if (in.sharedStatusPlus2) value += 2;
    if (in.medicineStatus) value += in.medicineBoostSkill ? 3 : 2;
    if (in.statusPlus4B) value += 4;
    if (in.statusPlus2B) value += 2;
    if (in.statusMinus1) value -= 1;
    value += in.itemStatAddend;
    value += in.enhancementAddend;
    if (in.applyDebuff) value -= in.debuffAmount;
    return ClampFinalCapability(value);
}

int ResolveResistance(int baseCapability, const ResistancePostInputs& in) {
    int value = baseCapability + in.pairBonus;
    if (in.resistancePlus2Skill) value += 2;
    value += in.absorbBonus;
    if (in.statusPlus4A) value += 4;
    if (in.sharedStatusPlus2) value += 2;
    if (in.medicineStatus) value += in.medicineBoostSkill ? 3 : 2;
    if (in.statusPlus4B) value += 4;
    if (in.statusPlus2B) value += 2;
    if (in.statusMinus1) value -= 1;
    value += in.itemStatAddend;
    value += in.enhancementAddend;
    if (in.applyDebuff) value -= in.debuffAmount;
    return ClampFinalCapability(value);
}

std::uint16_t ResolveCategoryMask(std::uint16_t jobMask, bool dragonPrivateSkill,
                                  bool beastPrivateSkill, bool specialWeaponBlocked,
                                  std::uint16_t dragonCategoryBit, std::uint16_t beastCategoryBit) {
    std::uint16_t value = jobMask;
    if (dragonPrivateSkill) value = static_cast<std::uint16_t>(value | dragonCategoryBit);
    if (beastPrivateSkill) value = static_cast<std::uint16_t>(value | beastCategoryBit);
    if (specialWeaponBlocked) value = static_cast<std::uint16_t>(value & ~dragonCategoryBit);
    return value;
}

bool IsSkillEquipped(std::int16_t skillId, std::int16_t routeClassSkillId,
                     const std::array<std::int16_t, 5>& equippedSkills) {
    if (skillId == 0) return false;
    if (routeClassSkillId == skillId) return true;
    return std::find(equippedSkills.begin(), equippedSkills.end(), skillId) != equippedSkills.end();
}

int ResolveWeaponExpLimit(int weaponKind, int jobLimit, bool specialWeaponBlocked,
                          bool staffEnablePrivateSkill, int staffMinimumThreshold) {
    if (weaponKind == 7 && specialWeaponBlocked) return 0;
    int limit = jobLimit;
    if (weaponKind == 6 && staffEnablePrivateSkill) limit = std::max(limit, staffMinimumThreshold);
    return limit;
}

int ResolveWeaponExp(int rawWeaponExp, int resolvedLimit) {
    return std::min(rawWeaponExp, resolvedLimit);
}

int ResolveWeaponExpForJobIntro(int weaponKind, int rawWeaponExp, bool specialWeaponBlocked) {
    return weaponKind == 7 && specialWeaponBlocked ? 0 : rawWeaponExp;
}

int FindHeldItemIndex(const std::array<HoldItemCandidate, 5>& items) {
    for (std::size_t i = 0; i < items.size(); ++i) {
        const auto& item = items[i];
        if (item.itemFlag4000 || (item.weaponKindSix && item.canEquipModeOne)) return static_cast<int>(i);
    }
    return -1;
}

int ResolveAttack(const AttackInputs& in) {
    if (!in.itemPresent) return 0;
    int value = in.itemPower + in.offensiveStat;
    if (in.includeWeaponRankBonus) value += in.weaponRankAttackBonus;
    return FloorZero(value);
}

int ResolveTerrainAttack(bool itemPresent, int itemPower, int offensiveStat,
                         int weaponRankAttackBonus) {
    if (!itemPresent) return 0;
    return FloorZero(itemPower + offensiveStat + weaponRankAttackBonus);
}

int ResolveHit(const HitInputs& in) {
    if (!in.itemPresent) return 0;
    int value = in.jobHitBonus + in.itemHit + ((in.tech * 3 + in.luck) >> 1);
    if (in.includeWeaponRankBonus) value += in.weaponRankHitBonus;
    if (in.privateHitPlus10) value += 10;
    if (in.privateHitPlus20) value += 20;
    if (in.privateHitPlus30) value += 30;
    return FloorZero(value);
}

int ResolveAvoid(const AvoidInputs& in) {
    int value = in.jobAvoidBonus + ((in.quick * 3 + in.luck) >> 1) + in.itemAvoid;
    if (in.privateAvoidMinus20) value -= 20;
    if (in.privateAvoidMinus10) value -= 10;
    if (in.privateAvoidPlus10) value += 10;
    if (in.privateAvoidPlus20) value += 20;
    if (in.unitStateMinus20) value -= 20;
    return FloorZero(value);
}

int ResolveCritical(const CriticalInputs& in) {
    if (!in.itemPresent || in.itemCriticalProhibited) return 0;
    int job = in.halveJobCriticalBonus ? (in.jobCriticalBonus >> 1) : in.jobCriticalBonus;
    int tech = std::max(in.tech - 4, 0) >> 1;
    return FloorZero(job + in.itemCritical + tech);
}

int ResolveSecure(int jobSecureBonus, int luck, bool itemPresent, int itemSecure) {
    return jobSecureBonus + (luck >> 1) + (itemPresent ? itemSecure : 0);
}

int ResolveContinuous(int quick, bool itemPresent, int signedItemAddend) {
    return FloorZero(quick + (itemPresent ? signedItemAddend : 0));
}

int ResolveUnderContinuous(int quick, bool itemPresent, int signedItemAddend) {
    return FloorZero(quick + (itemPresent ? signedItemAddend : 0));
}

int ResolveEquipSkillProbability(const EquipSkillProbabilityInputs& in) {
    if (!in.capabilitySelectorPresent) return 0;
    // Retail FE14 skill data is valid and uses a non-zero divisor. Zero/negative is a host-data guard only.
    if (in.divisor <= 0) return 0;
    int value = (in.multiplier * in.selectedCapability) / in.divisor;
    if (in.hoshidanUnitySkill) value += 10;
    if (in.contextPlus15) value += 15;
    if (in.selectorIsLuck && in.luckyCharmSkill) value += 20;
    value = std::clamp(value, 0, 100);
    if (in.forceProcPrivateSkill) value = 100;
    if (in.prohibitProcPrivateSkill) value = 0;
    return value;
}

} // namespace fates::unit::native
