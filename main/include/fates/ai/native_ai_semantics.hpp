#pragma once
#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace fates::ai::native {

struct RankedUnit { std::uint8_t unitId{}; std::uint32_t score{}; };
void StableSortDescending(std::span<RankedUnit> entries);

struct OrderCursorState {
    int count{};
    int dispatchIndex{};
    int currentIndex{};
    bool fixedUnit{};
    bool allowIdle{};
};
void ResetOrderCursor(OrderCursorState& state);
void ResolveOrderNext(OrderCursorState& state, bool externalActionPending, std::uint8_t& outerPhase);
void ResolveGaleFixed(OrderCursorState& state);
std::uint32_t ResolvePriorityScore(std::uint8_t priorityByte, int movePower);
bool ResolveAttackCrossfireEnable(std::uint8_t difficultyByte);

// Exact retail phase/status values are kept numeric because their enum names have not all
// been independently proven. Human meaning is documented beside the retail value.
constexpr std::uint8_t kOrderPhaseCause = 1;
constexpr std::uint8_t kOrderPhaseMind = 2;
constexpr std::uint8_t kOrderPhaseAttackInterference = 6;
constexpr std::uint8_t kOrderPhaseAttackLongRange = 7;
constexpr std::uint8_t kOrderPhaseAttackHigh = 8;
constexpr std::uint8_t kOrderPhaseAttackMiddle = 9;
constexpr std::uint8_t kOrderPhaseAttackLow = 10;
constexpr std::uint8_t kOrderPhaseMove = 11;
constexpr std::uint8_t kOuterPhaseProcessingPending = 4;
constexpr std::uint8_t kOuterPhaseTurnEnd = 5;

bool IsMoveOver(bool fixedUnitPresent, int storedX, int storedY, int x, int y);
bool IsAttackPermission(std::uint32_t commandFlags, bool unitPresent, bool unitDontAttack);
int ResolveMovePowerSlow(std::uint8_t difficultyByte, int baseMovePower);
bool IsActiveCommand(std::int8_t activitySelector, std::uint8_t currentActivity);

struct RangeMaskResult { int maxOuter{}; std::uint32_t mask{}; };
void MergeInclusiveRange(RangeMaskResult& out, unsigned inner, unsigned outer);

bool PreferCandidate(bool haveCurrent, std::uint32_t currentScore, std::uint32_t candidateScore,
                     unsigned tieRoll01);
std::uint32_t SidePositionScore(int moveCost, unsigned terrainScore);
std::uint32_t DestroyScore(int durabilityMinusAttack, int moveCost);

bool HasAdjacentSameForce(bool pairPartnerPresent, bool adjacentSameForcePresent);

struct UpdateCommitInput {
    std::uint32_t dirtyFlags{};
    std::int32_t activity{}; // pending word; activation tests the full word before byte truncation
    std::array<std::uint16_t,4> values{};
};
struct UpdateCommitResult {
    bool writeActivity{};
    bool activateUnit{};
    std::uint8_t activity{};
    std::array<bool,4> writeValue{};
    std::array<std::uint16_t,4> values{};
    std::uint32_t remainingFlags{};
};
UpdateCommitResult ResolveUpdateCommit(const UpdateCommitInput& in);

} // namespace fates::ai::native
