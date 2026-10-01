#pragma once
#include "fates/runtime/native_unit_turn.hpp"
#include <cstdint>

namespace fates::runtime::native {
// Value projection of fields used by Sequence::TurnBegin/TurnEnd. The native
// Unit remains their persistent owner. Unknown counter meanings stay unnamed.
struct UnitTurnSnapshot {
    std::uint32_t flags{}, secondary_flags{};
    std::uint8_t counter_12d{}, counter_135{};
    UnitEnhanceSnapshot enhance{};
    bool operator==(const UnitTurnSnapshot&) const = default;
};

// These split at the original Unit::UpdateClone call. A true return requires
// clone synchronization before Finish, or evidence that the clone is absent.
// Neither function invents clone state or acknowledges an unowned service.
bool BeginUnitTurnEndExact(UnitTurnSnapshot&) noexcept;
void FinishUnitTurnEndExact(UnitTurnSnapshot&) noexcept;
bool BeginUnitTurnBeginExact(UnitTurnSnapshot&, bool recovery_skill) noexcept;
void FinishUnitTurnBeginExact(UnitTurnSnapshot&) noexcept;

enum class TurnControlRoute : std::uint8_t { Human = 2, Ai = 3, Link = 4, Exit = 5 };
TurnControlRoute SelectTurnControlRouteExact(std::uint8_t control) noexcept;
bool SkipEmptyTurnExact(std::uint8_t control, std::uint32_t force_count) noexcept;
}
