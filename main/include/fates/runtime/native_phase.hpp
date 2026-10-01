#pragma once
#include "fates/runtime/native_runtime.hpp"

namespace fates::runtime::native {
struct SituationTurnEndResult {
    bool valid{};
    bool advanced{};
    SituationPhaseState situation{};
    fates::chapter::native::SituationOutcomeState outcome{};
};
// Complete bounded Situation::TurnEnd transform. Outcome flags are explicit
// FlagManager inputs. Does NOT perform Sequence::TurnEnd unit/Enhance cleanup,
// Deploy::TurnReset, entry effects, events, reinforcements, or empty-force skip.
SituationTurnEndResult EvaluateSituationTurnEnd(
    const SituationPhaseState&, fates::chapter::native::SituationOutcomeState) noexcept;
SituationPhaseState SelectFirstHumanForce(SituationPhaseState) noexcept;

enum class PhaseContextStatus : std::uint8_t {
    Ok, InvalidSnapshot, AlreadyBound, NotReady, StaleChapter, RevisionExhausted,
    AwaitingExitServices
};
// Binds an unprepared context and closes command access. Defaults mirror the
// Situation initializer only; they are NOT a certificate of completed startup.
PhaseContextStatus BindSituationPhase(NativeRuntime&, const SituationPhaseState&);
// Restore boundary for an ALREADY prepared external snapshot. No ROMFS/default
// roster path calls this. The provider owns prior turn-entry effects and events.
// It is one-time on an unbound runtime, not a shortcut to acknowledge later phases.
PhaseContextStatus RestorePreparedPhaseSnapshot(
    NativeRuntime&, const SituationPhaseState&, std::uint8_t expected_chapter);
// Only closes command access. Actual exit services and applying the pure reducer
// are deliberately not replaced by fake successful lifecycle callbacks.
PhaseContextStatus RequestPhaseEnd(NativeRuntime&);
bool PhaseAllowsPlayerCommands(const NativeRuntime&, std::uint8_t requested_force) noexcept;
} // namespace fates::runtime::native
