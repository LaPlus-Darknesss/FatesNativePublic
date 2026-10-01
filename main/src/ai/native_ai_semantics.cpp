#include "fates/ai/native_ai_semantics.hpp"
#include <algorithm>

namespace fates::ai::native {

void StableSortDescending(std::span<RankedUnit> entries) {
    // Retail 0x004C721C swaps only when left.score < right.score, so equal scores retain order.
    std::stable_sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) {
        return a.score > b.score;
    });
}

void ResetOrderCursor(OrderCursorState& state) {
    state.count = 0;
    state.dispatchIndex = 0;
    state.currentIndex = 0;
    state.fixedUnit = false;
    state.allowIdle = false;
}

void ResolveOrderNext(OrderCursorState& state, bool externalActionPending, std::uint8_t& outerPhase) {
    if (externalActionPending) {
        outerPhase = kOuterPhaseProcessingPending;
        return;
    }
    if (state.fixedUnit) {
        state.fixedUnit = false;
        return;
    }
    if (state.currentIndex < state.count) ++state.currentIndex;
}

void ResolveGaleFixed(OrderCursorState& state) {
    if (state.currentIndex > 0) --state.currentIndex;
}

std::uint32_t ResolvePriorityScore(std::uint8_t priorityByte, int movePower) {
    return static_cast<std::uint32_t>((0x10 - movePower) + static_cast<int>(priorityByte) * 0x100);
}

bool ResolveAttackCrossfireEnable(std::uint8_t difficultyByte) { return difficultyByte > 1; }

bool IsMoveOver(bool fixedUnitPresent, int storedX, int storedY, int x, int y) {
    return fixedUnitPresent && storedX == x && storedY == y;
}

bool IsAttackPermission(std::uint32_t commandFlags, bool unitPresent, bool unitDontAttack) {
    return (commandFlags & 4u) == 0 && (!unitPresent || !unitDontAttack);
}

int ResolveMovePowerSlow(std::uint8_t difficultyByte, int baseMovePower) {
    if (difficultyByte == 1) return baseMovePower - 1;
    if (difficultyByte == 2) return baseMovePower;
    return baseMovePower - 2;
}

bool IsActiveCommand(std::int8_t selector, std::uint8_t currentActivity) {
    if (selector == -2) return currentActivity == 0;
    if (selector == -1) return true;
    if (selector == 0) return currentActivity != 0;
    return static_cast<int>(selector) == static_cast<int>(currentActivity);
}

void MergeInclusiveRange(RangeMaskResult& out, unsigned inner, unsigned outer) {
    if (outer == 0) return;
    if (outer == 0xFFu) {
        out.maxOuter = 0xFF;
        out.mask = 0xFFFFFFFFu;
        return;
    }
    if (inner > outer) return;
    for (unsigned d = inner; d <= outer; ++d) {
        if (d < 32u) out.mask |= (1u << d);
    }
    if (out.maxOuter < static_cast<int>(outer)) out.maxOuter = static_cast<int>(outer);
}

bool PreferCandidate(bool haveCurrent, std::uint32_t currentScore, std::uint32_t candidateScore,
                     unsigned tieRoll01) {
    if (!haveCurrent) return true;
    if (candidateScore > currentScore) return true;
    return candidateScore == currentScore && tieRoll01 == 0;
}

std::uint32_t SidePositionScore(int moveCost, unsigned terrainScore) {
    return static_cast<std::uint32_t>((100 - moveCost) * 0x10) + terrainScore;
}

std::uint32_t DestroyScore(int durabilityMinusAttack, int moveCost) {
    const int remaining = std::max(0, durabilityMinusAttack);
    return static_cast<std::uint32_t>((0xFF - remaining) * 0x100 + (100 - moveCost));
}

bool HasAdjacentSameForce(bool pairPartnerPresent, bool adjacentSameForcePresent) {
    return pairPartnerPresent || adjacentSameForcePresent;
}

UpdateCommitResult ResolveUpdateCommit(const UpdateCommitInput& in) {
    UpdateCommitResult out;
    out.writeActivity = (in.dirtyFlags & 1u) != 0;
    out.activity = static_cast<std::uint8_t>(in.activity);
    out.activateUnit = out.writeActivity && in.activity != 0;
    for (unsigned i = 0; i < 4; ++i) {
        out.writeValue[i] = (in.dirtyFlags & (2u << i)) != 0;
        out.values[i] = in.values[i];
    }
    out.remainingFlags = in.dirtyFlags & 0xFFFFFFE0u;
    return out;
}

} // namespace fates::ai::native
