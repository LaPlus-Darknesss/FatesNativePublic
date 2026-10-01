#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace fates::unit::native {

enum class CapabilityIndex : std::uint8_t {
    Mhp = 0,
    Str = 1,
    Pow = 2,
    Tech = 3,
    Quick = 4,
    Luck = 5,
    Def = 6,
    Mdef = 7,
};

inline constexpr std::array<CapabilityIndex, 8> kRetailCapabilityOrder{
    CapabilityIndex::Mhp, CapabilityIndex::Str, CapabilityIndex::Pow, CapabilityIndex::Tech,
    CapabilityIndex::Quick, CapabilityIndex::Luck, CapabilityIndex::Def, CapabilityIndex::Mdef};

struct BaseCapabilityInputs {
    int personBase{};
    int jobBase{};
    int unitStored{};
    int editBonus{};
    bool subtractStoredPenalty{};
    int storedPenalty{};
    int retailLimit{};
};

int ResolveBaseCapability(const BaseCapabilityInputs& in);
int ClampFinalCapability(int value);
int CapabilityRating(const std::array<int, 8>& finalCapabilities);

struct MhpPostInputs {
    bool halveBeforeAddends{};
    bool maxHpPlus5Skill{};
    bool medicineStatus{};
    bool medicineBoostSkill{};
    bool statusPlus2{};
    bool statusMinus1{};
};
int ResolveMhp(int baseCapability, const MhpPostInputs& in);

struct StrengthPostInputs {
    bool halveBeforeAddends{};
    int pairBonus{};
    bool strengthPlus2Skill{};
    bool strongBladePlus3Skill{};
    int absorbBonus{};
    bool statusPlus4A{};
    bool sharedStatusPlus2{};
    bool statusPlus1{};
    bool medicineStatus{};
    bool medicineBoostSkill{};
    bool statusPlus4B{};
    bool statusPlus2B{};
    bool statusMinus1{};
    int itemStatAddend{};
    int enhancementAddend{};
    bool applyDebuff{};
    int debuffAmount{};
};
int ResolveStrength(int baseCapability, const StrengthPostInputs& in);

struct TechPostInputs {
    int pairBonus{};
    bool techPlus2Skill{};
    int absorbBonus{};
    bool statusPlus4A{};
    bool sharedStatusPlus2{};
    bool statusPlus3{};
    bool medicineStatus{};
    bool medicineBoostSkill{};
    bool statusPlus4B{};
    bool statusPlus2B{};
    bool statusMinus1{};
    int itemStatAddend{};
    int enhancementAddend{};
    bool applyDebuff{};
    int debuffAmount{};
};
int ResolveTech(int baseCapability, const TechPostInputs& in);

struct DefensePostInputs {
    int pairBonus{};
    bool defensePlus2Skill{};
    bool finesseMinus1Skill{};
    int absorbBonus{};
    bool statusPlus4A{};
    bool sharedStatusPlus2{};
    bool medicineStatus{};
    bool medicineBoostSkill{};
    bool statusPlus4B{};
    bool statusPlus2B{};
    bool statusMinus1{};
    int itemStatAddend{};
    int enhancementAddend{};
    bool applyDebuff{};
    int debuffAmount{};
};
int ResolveDefense(int baseCapability, const DefensePostInputs& in);

struct ResistancePostInputs {
    int pairBonus{};
    bool resistancePlus2Skill{};
    int absorbBonus{};
    bool statusPlus4A{};
    bool sharedStatusPlus2{};
    bool medicineStatus{};
    bool medicineBoostSkill{};
    bool statusPlus4B{};
    bool statusPlus2B{};
    bool statusMinus1{};
    int itemStatAddend{};
    int enhancementAddend{};
    bool applyDebuff{};
    int debuffAmount{};
};
int ResolveResistance(int baseCapability, const ResistancePostInputs& in);

std::uint16_t ResolveCategoryMask(std::uint16_t jobMask, bool dragonPrivateSkill,
                                  bool beastPrivateSkill, bool specialWeaponBlocked,
                                  std::uint16_t dragonCategoryBit, std::uint16_t beastCategoryBit);

bool IsSkillEquipped(std::int16_t skillId, std::int16_t routeClassSkillId,
                     const std::array<std::int16_t, 5>& equippedSkills);

int ResolveWeaponExpLimit(int weaponKind, int jobLimit, bool specialWeaponBlocked,
                          bool staffEnablePrivateSkill, int staffMinimumThreshold);
int ResolveWeaponExp(int rawWeaponExp, int resolvedLimit);
int ResolveWeaponExpForJobIntro(int weaponKind, int rawWeaponExp, bool specialWeaponBlocked);

struct HoldItemCandidate {
    bool itemFlag4000{};
    bool weaponKindSix{};
    bool canEquipModeOne{};
};
int FindHeldItemIndex(const std::array<HoldItemCandidate, 5>& items);

struct AttackInputs {
    bool itemPresent{};
    int itemPower{};
    int offensiveStat{};
    bool includeWeaponRankBonus{};
    int weaponRankAttackBonus{};
};
int ResolveAttack(const AttackInputs& in);
int ResolveTerrainAttack(bool itemPresent, int itemPower, int offensiveStat,
                         int weaponRankAttackBonus);

struct HitInputs {
    bool itemPresent{};
    int jobHitBonus{};
    int itemHit{};
    int tech{};
    int luck{};
    bool includeWeaponRankBonus{};
    int weaponRankHitBonus{};
    bool privateHitPlus10{};
    bool privateHitPlus20{};
    bool privateHitPlus30{};
};
int ResolveHit(const HitInputs& in);

struct AvoidInputs {
    int jobAvoidBonus{};
    int quick{};
    int luck{};
    int itemAvoid{};
    bool privateAvoidMinus20{};
    bool privateAvoidMinus10{};
    bool privateAvoidPlus10{};
    bool privateAvoidPlus20{};
    bool unitStateMinus20{};
};
int ResolveAvoid(const AvoidInputs& in);

struct CriticalInputs {
    bool itemPresent{};
    bool itemCriticalProhibited{};
    int jobCriticalBonus{};
    bool halveJobCriticalBonus{};
    int itemCritical{};
    int tech{};
};
int ResolveCritical(const CriticalInputs& in);
int ResolveSecure(int jobSecureBonus, int luck, bool itemPresent, int itemSecure);
int ResolveContinuous(int quick, bool itemPresent, int signedItemAddend);
int ResolveUnderContinuous(int quick, bool itemPresent, int signedItemAddend);

struct EquipSkillProbabilityInputs {
    bool capabilitySelectorPresent{};
    int selectedCapability{};
    int multiplier{};
    int divisor{};
    bool hoshidanUnitySkill{};
    bool contextPlus15{};
    bool selectorIsLuck{};
    bool luckyCharmSkill{};
    bool forceProcPrivateSkill{};
    bool prohibitProcPrivateSkill{};
};
int ResolveEquipSkillProbability(const EquipSkillProbabilityInputs& in);

} // namespace fates::unit::native
