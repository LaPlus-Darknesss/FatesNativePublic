#include "fates/battle/native_battle_semantics.hpp"
#include <algorithm>
#include <cstdlib>

namespace fates::battle::native {
namespace {
std::int16_t AddSigned16(std::int16_t lhs, std::int32_t rhs) {
    return WrapSigned16(static_cast<std::int32_t>(lhs) + rhs);
}
}

bool IsDualSideKind(std::uint8_t sideKind) { return sideKind == 2 || sideKind == 3; }

std::int16_t WrapSigned16(std::int32_t value) {
    const std::uint16_t raw = static_cast<std::uint16_t>(static_cast<std::uint32_t>(value));
    if (raw <= 0x7FFFu) return static_cast<std::int16_t>(raw);
    return static_cast<std::int16_t>(static_cast<std::int32_t>(raw) - 0x10000);
}

std::int16_t ResolveDetailHit(const DetailHitInput& in) {
    // Retail 0x00348FF0: note that side flag 0x40 is NOT a hit-lane gate here.
    if (!in.selfPresent || !in.opponentPresent || !in.equippedItemPresent || in.opponentIsDualLane)
        return 0;
    std::int16_t value = in.baseHit;
    if (in.weaponInteractionDelta != 0)
        value = AddSigned16(value, in.weaponInteractionHitPerStep * in.weaponInteractionDelta);
    if (in.itemAppliesTenPointPenalty && (in.sideFlags & kSideFlagEffective) == 0)
        value = AddSigned16(value, -10);
    value = AddSigned16(value, in.modifierAddend);
    value = AddSigned16(value, in.baseAddend);
    value = AddSigned16(value, in.lateAddend);
    if ((in.sideFlags & kSideFlagBattlePenalty4000) != 0)
        value = AddSigned16(value, -10);
    return value < 0 ? 0 : value;
}

std::int16_t ResolveDetailAvoid(const DetailAvoidInput& in) {
    if (!in.selfPresent || !in.opponentPresent) return 0;
    std::int16_t value = in.baseAvoid;
    if (in.terrainApplies) value = AddSigned16(value, in.terrainAvoid);
    value = AddSigned16(value, in.modifierAddend);
    value = AddSigned16(value, in.baseAddend);
    value = AddSigned16(value, in.lateAddend);
    return value < 0 ? 0 : value;
}

std::int32_t ResolveDetailAttack(const DetailAttackInput& in) {
    if (!in.selfPresent || !in.opponentPresent || !in.equippedItemPresent ||
        in.opponentIsDualLane || (in.sideFlags & kSideFlagCalculationSuppressed) != 0)
        return 0;
    std::int32_t value = in.baseAttack;
    if (in.weaponInteractionDelta != 0)
        value += in.weaponInteractionAttackPerStep * in.weaponInteractionDelta;
    std::int32_t adjustedPower = in.itemPower;
    if (in.itemPowerMinusFourUnlessEffective && (in.sideFlags & kSideFlagEffective) == 0)
        adjustedPower -= 4;
    value += adjustedPower - in.itemPower;
    if ((in.sideFlags & kSideFlagModeBit0) != 0 && in.addItemPowerWhenModeBit0Set)
        value += adjustedPower;
    if ((in.sideFlags & kSideFlagModeBit0) == 0 && in.addItemPowerWhenModeBit0Clear)
        value += adjustedPower;
    if (in.addItemPowerWhenTechHigher && in.attackerTechHigher)
        value += adjustedPower;
    if ((in.sideFlags & kSideFlagEffective) != 0)
        value += adjustedPower * 2;
    else if ((in.sideFlags & kSideFlagSupplementalEfficacy) != 0)
        value += adjustedPower;
    value += in.modifierAttackAddend;
    value += in.lateAttackAddendA;
    value += in.lateAttackAddendB;
    return value < 1 ? 0 : value;
}

DetailDefenseResult ResolveDetailDefense(const DetailDefenseInput& in) {
    if (!in.selfPresent || !in.opponentPresent) return {};
    DetailDefenseResult out;
    out.defense = std::max<std::int32_t>(0, in.baseDefense);
    out.resistance = std::max<std::int32_t>(0, in.baseResistance);
    out.defenseBonus = in.sharedModifierAddend + in.defenseAddend + in.lateAddendA + in.lateAddendB;
    out.resistanceBonus = in.sharedModifierAddend + in.resistanceAddend + in.lateAddendA + in.lateAddendB;
    if (in.terrainApplies) {
        out.defenseBonus += in.terrainDefenseResistance;
        out.resistanceBonus += in.terrainDefenseResistance;
    }
    return out;
}

std::int32_t ResolveOrdinarySimpleCriticalChance(const std::int32_t critical,
    const std::int32_t dodge,const bool forbidden) noexcept {
    if(forbidden)return 0;
    return static_cast<std::int32_t>(std::clamp<std::int64_t>(
        static_cast<std::int64_t>(critical)-dodge,0,100));
}

DetailTailResult ResolveDetailTail(const DetailTailInput& in) {
    DetailTailResult out;
    if (in.selfPresent && in.opponentPresent && in.equippedItemPresent && in.primaryLane &&
        (in.sideFlags & kSideFlagCalculationSuppressed) == 0) {
        auto v = in.baseCritical;
        v = AddSigned16(v, in.criticalModifier);
        v = AddSigned16(v, in.criticalBaseAddend);
        v = AddSigned16(v, in.criticalLateAddend);
        out.critical = v < 0 ? 0 : v;
    }
    if (in.selfPresent && in.opponentPresent) {
        auto v = in.baseSecure;
        v = AddSigned16(v, in.secureModifier);
        v = AddSigned16(v, in.secureBaseAddend);
        v = AddSigned16(v, in.secureLateAddend);
        out.secure = v < 0 ? 0 : v;
        if (in.primaryLane) {
            out.continuous = in.baseContinuous + in.continuousAddend;
            out.underContinuous = in.baseUnderContinuous + in.underContinuousAddend;
        }
    }
    return out;
}

EfficacyResult ResolveEfficacyFlags(const EfficacyInput& in) {
    EfficacyResult out{in.sideFlags, in.opponentCategoryMask};
    if (!in.selfPresent || !in.equippedItemPresent ||
        (in.sideFlags & kSideFlagCalculationSuppressed) != 0) return out;
    if (in.attackerHasSupplementalEfficacySkill)
        out.sideFlags |= kSideFlagSupplementalEfficacy;
    out.remainingOpponentCategoryMask = static_cast<std::uint16_t>(
        in.opponentCategoryMask & static_cast<std::uint16_t>(~in.defenderCategorySuppressionMask));
    if ((out.remainingOpponentCategoryMask & in.itemEfficacyCategoryMask) != 0 ||
        in.directWeaponSubKindEffective)
        out.sideFlags |= kSideFlagEffective;
    return out;
}

std::int32_t ResolveSimpleCritical(bool item, std::uint32_t flags, std::int16_t crit,
                                   std::int16_t opponentSecure, bool itemForcesZero) {
    if (!item || (flags & kSideFlagCalculationSuppressed) != 0) return 0;
    std::int32_t value = static_cast<std::int32_t>(crit) - static_cast<std::int32_t>(opponentSecure);
    value = std::clamp(value, 0, 100);
    if (itemForcesZero || (flags & kSideFlagBattlePenalty4000) != 0) return 0;
    return value;
}

std::int32_t ResolveSimpleDamageRate(bool item, std::uint32_t flags,
                                     bool itemUsesFourHundredRate, bool defenderHalvesRate) {
    std::int32_t value = 300;
    if (item && (flags & kSideFlagCalculationSuppressed) == 0 && itemUsesFourHundredRate)
        value = 400;
    if (defenderHalvesRate) value >>= 1;
    return value;
}

bool ResolveDualFollowupGate(bool selfPresent, bool pairPartnerPresent, bool blockingSkillPresent) {
    return selfPresent && pairPartnerPresent && !blockingSkillPresent;
}

std::array<std::uint32_t,4> ResolveBaseConditionFlags(const BaseConditionFlagsInput& in) {
    auto out = in.sideFlags;
    const auto s0 = out[0], s2 = out[2], s3 = out[3];
    out[0] = s0 | 1u;
    out[2] = s2 | 3u;
    out[3] = s3 | 2u;
    if ((in.battleFlags & 2u) != 0) {
        out[0] = s0 | 9u;
        out[1] |= 8u;
        out[2] = s2 | 0xBu;
        out[3] = s3 | 0xAu;
    } else if ((in.battleFlags & 4u) != 0) {
        out[0] = s0 | 9u;
        out[2] = s2 | 0xBu;
    }
    return out;
}

std::int32_t ResolveBattleDistance(std::int32_t currentDistance, std::uint32_t battleFlags,
                                   std::int32_t x0, std::int32_t y0,
                                   std::int32_t x1, std::int32_t y1) {
    if (currentDistance >= 1) return currentDistance;
    if ((battleFlags & 1u) != 0) return 0;
    return std::abs(x0 - x1) + std::abs(y0 - y1);
}

DualAttackResult ResolveDualAttackState(const DualAttackInput& in) {
    DualAttackResult out{false, 0u, in.partnerSimpleAttackCount};
    if (in.partnerPresent) {
        if (in.partnerIsPairPartner) {
            out.guardProgressSum = static_cast<std::uint32_t>(in.selfGuardProgress) +
                                   static_cast<std::uint32_t>(in.partnerGuardProgress);
            // Retail Pair-Up partner does not attack by default. A specific EquipSkill
            // (identity resolved outside this policy) opens the attack route.
            if (in.pairPartnerAttackEnableSkillPresent &&
                (in.sideFlags & kSideFlagDualAttackBlocked) == 0)
                out.active = true;
        } else if ((in.sideFlags & kSideFlagDualAttackBlocked) == 0) {
            out.active = true;
        }
    }
    if (in.primarySimpleAttackCount < 1 || !out.active ||
        (in.sideFlags & kSideFlagBattlePenalty4000) != 0)
        out.partnerSimpleAttackCount = 0;
    return out;
}

BattleProgressState InitializeBattleProgress(const ProgressSeed& side0,
                                             const std::array<OptionalProgressSeed,3>& other) {
    BattleProgressState out{};
    out.simulatedHp[0] = side0.currentHp;
    out.guardOrProgress[0] = side0.guardOrProgress;
    for (std::size_t i=0;i<other.size();++i) {
        if (!other[i].present) continue;
        out.simulatedHp[i+1] = other[i].value.currentHp;
        out.guardOrProgress[i+1] = other[i].value.guardOrProgress;
    }
    return out;
}

BattleSceneSnapshot MakeDefaultBattleSceneSnapshot(const BattleProgressState& progress) {
    BattleSceneSnapshot out{};
    out.finalHp = progress.simulatedHp;
    out.guardOrProgress = progress.guardOrProgress;
    return out;
}

bool BattleProcRollSucceeds(std::int32_t roll, std::int32_t probability, std::uint16_t skillId) {
    return roll < probability && skillId != 0;
}

std::uint8_t MaxAttackAttempts(ProcRepeatClass c) {
    switch(c) {
        case ProcRepeatClass::SecondAttemptIfFirstFails: return 2;
        case ProcRepeatClass::UpToFiveAttemptsUntilSuccess: return 5;
        case ProcRepeatClass::SingleAttempt: default: return 1;
    }
}

} // namespace fates::battle::native
