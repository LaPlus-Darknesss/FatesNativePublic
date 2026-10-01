#pragma once
#include "fates/runtime/native_runtime.hpp"
#include <cstdint>
namespace fates::ai::native {
inline constexpr std::uint8_t kActionEverytime = 1;         // AI_AC_Everytime
inline constexpr std::uint8_t kActionTurn = 12;               // AI_AC_Turn
inline constexpr std::uint8_t kActionTurnAttackRange = 15; // AI_AC_TurnAttackRange
struct TurnAttackRangeActivation {
    bool supported{};
    bool turn_cause{};
    bool attack_range_cause{};
    bool active{};
};
// B007's TurnAttackRange records provide one action argument (2/3/4/5).
// Retail AIValue defaults all later slots to -1; this bounded portable lane
// deliberately refuses non-default AttackRange parameters until separately earned.
bool HasSingleTurnArgumentShape(const fates::runtime::native::UnitState& unit) noexcept;
bool HasB007TurnAttackRangeArgumentShape(const fates::runtime::native::UnitState& unit) noexcept;
bool HasHostileInsideFullMoveAttackArea(const fates::runtime::native::NativeRuntime& runtime,
                                        std::uint16_t unit_slot);
TurnAttackRangeActivation EvaluateTurnAttackRangeActivation(
    const fates::runtime::native::NativeRuntime& runtime,
    std::uint16_t unit_slot,
    std::uint16_t current_turn);
} // namespace fates::ai::native
