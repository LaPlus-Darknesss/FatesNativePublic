#pragma once
#include "fates/runtime/native_runtime.hpp"
#include "fates/runtime/native_turn_state.hpp"
#include <cstdint>

namespace fates::runtime::native {
enum class ForceTurnOperation : std::uint8_t { Begin, End };
enum class ForceTurnStatus : std::uint8_t {
    Ok, InvalidOperation, InvalidPhase, StaleChapter, StaleRevision, ChangedContext,
    AlreadyApplied, MissingOrder, MissingDefinition, MissingSkill, MissingEnhance,
    MissingMapEnd, MissingCounter, MissingCloneState, CloneOwnerRequired,
    RevisionExhausted, InvalidUnit, AlreadyBound, InvalidCarriedState
};
struct ForceTurnResult {
    ForceTurnStatus status{ForceTurnStatus::Ok};
    std::uint16_t unit_slot{0xffffu};
    std::uint16_t updated_units{};
};
ForceTurnStatus RestoreUnitTurnState(UnitState&, const NativeUnitTurnState&) noexcept;

// Executes just the original ordered force's Unit upkeep. Whole-force preflight
// precedes mutation. The phase-revision-bound progress prevents repeating it.
// Begin leaves entry services pending. End leaves exit services pending; it does
// not run Reinforce, refresh Danger, apply Situation progression or open commands.
// A composed controller must place this at the original sequence position.
ForceTurnResult ApplyCurrentForceTurnUpkeep(NativeRuntime&, ForceTurnOperation,
    std::uint64_t expected_phase_revision);
}
