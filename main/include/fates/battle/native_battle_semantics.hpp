#pragma once
#include <array>
#include <cstdint>

namespace fates::battle::native {

// Retail side-flag bits whose exact numeric roles are proven by BattleInfo.
// Names stay intentionally conservative until player-facing labels are independently earned.
constexpr std::uint32_t kSideFlagModeBit0 = 0x00000001u;
constexpr std::uint32_t kSideFlagTerrainSuppressed = 0x00000010u;
constexpr std::uint32_t kSideFlagCalculationSuppressed = 0x00000040u;
constexpr std::uint32_t kSideFlagDualAttackBlocked = 0x00000100u;
constexpr std::uint32_t kSideFlagSupplementalEfficacy = 0x00000200u;
constexpr std::uint32_t kSideFlagEffective = 0x00000400u;
constexpr std::uint32_t kSideFlagBattlePenalty4000 = 0x00004000u;

bool IsDualSideKind(std::uint8_t sideKind);
std::int16_t WrapSigned16(std::int32_t value);

struct DetailHitInput {
    bool selfPresent{};
    bool opponentPresent{};
    bool equippedItemPresent{};
    bool opponentIsDualLane{};
    std::int16_t baseHit{};
    std::int32_t weaponInteractionDelta{};
    std::int32_t weaponInteractionHitPerStep{};
    bool itemAppliesTenPointPenalty{};
    std::int16_t modifierAddend{};
    std::int16_t baseAddend{};
    std::int16_t lateAddend{};
    std::uint32_t sideFlags{};
};
std::int16_t ResolveDetailHit(const DetailHitInput& input);

struct DetailAvoidInput {
    bool selfPresent{};
    bool opponentPresent{};
    std::int16_t baseAvoid{};
    bool terrainApplies{};
    std::int8_t terrainAvoid{};
    std::int16_t modifierAddend{};
    std::int16_t baseAddend{};
    std::int16_t lateAddend{};
};
std::int16_t ResolveDetailAvoid(const DetailAvoidInput& input);

struct DetailAttackInput {
    bool selfPresent{};
    bool opponentPresent{};
    bool equippedItemPresent{};
    bool opponentIsDualLane{};
    std::uint32_t sideFlags{};
    std::int32_t baseAttack{};
    std::int32_t weaponInteractionDelta{};
    std::int32_t weaponInteractionAttackPerStep{};
    std::int32_t itemPower{};
    bool itemPowerMinusFourUnlessEffective{};
    bool addItemPowerWhenModeBit0Set{};
    bool addItemPowerWhenModeBit0Clear{};
    bool addItemPowerWhenTechHigher{};
    bool attackerTechHigher{};
    std::int32_t modifierAttackAddend{};
    std::int32_t lateAttackAddendA{};
    std::int32_t lateAttackAddendB{};
};
std::int32_t ResolveDetailAttack(const DetailAttackInput& input);

struct DetailDefenseInput {
    bool selfPresent{};
    bool opponentPresent{};
    std::int32_t baseDefense{};
    std::int32_t baseResistance{};
    std::int32_t sharedModifierAddend{};
    std::int32_t defenseAddend{};
    std::int32_t resistanceAddend{};
    std::int32_t lateAddendA{};
    std::int32_t lateAddendB{};
    bool terrainApplies{};
    std::int8_t terrainDefenseResistance{};
};
struct DetailDefenseResult {
    std::int32_t defense{};
    std::int32_t defenseBonus{};
    std::int32_t resistance{};
    std::int32_t resistanceBonus{};
};
DetailDefenseResult ResolveDetailDefense(const DetailDefenseInput& input);

// Ordinary CalculateSimple critical lane. The no-critical/no-offensive-proc
// ItemSkill is accepted only while the caller still rejects active proc skills.
std::int32_t ResolveOrdinarySimpleCriticalChance(std::int32_t critical,
    std::int32_t dodge,bool item_forbids_critical) noexcept;

struct DetailTailInput {
    bool selfPresent{};
    bool opponentPresent{};
    bool equippedItemPresent{};
    bool primaryLane{};
    std::uint32_t sideFlags{};
    std::int16_t baseCritical{};
    std::int16_t criticalModifier{};
    std::int16_t criticalBaseAddend{};
    std::int16_t criticalLateAddend{};
    std::int16_t baseSecure{};
    std::int16_t secureModifier{};
    std::int16_t secureBaseAddend{};
    std::int16_t secureLateAddend{};
    std::int32_t baseContinuous{};
    std::int32_t continuousAddend{};
    std::int32_t baseUnderContinuous{};
    std::int32_t underContinuousAddend{};
};
struct DetailTailResult {
    std::int16_t critical{};
    std::int16_t secure{};
    std::int32_t continuous{};
    std::int32_t underContinuous{};
};
DetailTailResult ResolveDetailTail(const DetailTailInput& input);

struct EfficacyInput {
    bool selfPresent{};
    bool equippedItemPresent{};
    std::uint32_t sideFlags{};
    bool attackerHasSupplementalEfficacySkill{};
    std::uint16_t opponentCategoryMask{};
    std::uint16_t defenderCategorySuppressionMask{};
    std::uint16_t itemEfficacyCategoryMask{};
    bool directWeaponSubKindEffective{};
};
struct EfficacyResult {
    std::uint32_t sideFlags{};
    std::uint16_t remainingOpponentCategoryMask{};
};
EfficacyResult ResolveEfficacyFlags(const EfficacyInput& input);

std::int32_t ResolveSimpleCritical(bool equippedItemPresent, std::uint32_t sideFlags,
                                   std::int16_t detailCritical, std::int16_t opponentSecure,
                                   bool itemForcesZeroCritical);
std::int32_t ResolveSimpleDamageRate(bool equippedItemPresent, std::uint32_t sideFlags,
                                     bool itemUsesFourHundredRate, bool defenderHalvesRate);
bool ResolveDualFollowupGate(bool selfPresent, bool pairPartnerPresent, bool blockingSkillPresent);

struct BaseConditionFlagsInput {
    std::uint32_t battleFlags{};
    std::array<std::uint32_t,4> sideFlags{};
};
std::array<std::uint32_t,4> ResolveBaseConditionFlags(const BaseConditionFlagsInput& input);
std::int32_t ResolveBattleDistance(std::int32_t currentDistance, std::uint32_t battleFlags,
                                   std::int32_t x0, std::int32_t y0,
                                   std::int32_t x1, std::int32_t y1);

struct DualAttackInput {
    bool partnerPresent{};
    bool partnerIsPairPartner{};
    std::uint8_t selfGuardProgress{};
    std::uint8_t partnerGuardProgress{};
    bool pairPartnerAttackEnableSkillPresent{};
    std::uint32_t sideFlags{};
    std::int32_t primarySimpleAttackCount{};
    std::int32_t partnerSimpleAttackCount{};
};
struct DualAttackResult {
    bool active{};
    std::uint32_t guardProgressSum{};
    std::int32_t partnerSimpleAttackCount{};
};
DualAttackResult ResolveDualAttackState(const DualAttackInput& input);

struct ProgressSeed {
    std::int32_t currentHp{};
    std::uint32_t guardOrProgress{};
};
struct OptionalProgressSeed {
    bool present{};
    ProgressSeed value{};
};
struct BattleProgressState {
    std::array<std::int32_t,4> simulatedHp{};
    std::array<std::uint32_t,4> guardOrProgress{};
    std::array<std::uint32_t,4> statusFlags{};
};
BattleProgressState InitializeBattleProgress(const ProgressSeed& side0,
                                             const std::array<OptionalProgressSeed,3>& otherSides);

struct BattleSceneSnapshot {
    std::uint8_t actorSide{};
    std::uint8_t sceneType{};
    std::uint32_t skillOrEffectOrProcId{};
    std::uint32_t resultOrEffectFlags{};
    std::array<std::int32_t,4> displayDelta{};
    std::array<std::int32_t,4> finalHp{};
    std::array<std::uint32_t,4> guardOrProgress{};
};
BattleSceneSnapshot MakeDefaultBattleSceneSnapshot(const BattleProgressState& progress);

bool BattleProcRollSucceeds(std::int32_t random0To99, std::int32_t probability,
                            std::uint16_t skillId);
enum class ProcRepeatClass : std::uint8_t {
    SingleAttempt,
    SecondAttemptIfFirstFails,
    UpToFiveAttemptsUntilSuccess,
};
std::uint8_t MaxAttackAttempts(ProcRepeatClass repeatClass);

} // namespace fates::battle::native
