#include "fates/runtime/native_force_turn.hpp"
#include "fates/runtime/native_force_order.hpp"
#include "fates/runtime/native_current_item_eligibility.hpp"
#include "fates/runtime/native_turn_upkeep.hpp"
#include <limits>
#include <vector>

namespace fates::runtime::native {
namespace {
using S = ForceTurnStatus;
NativeForceUpkeepProgress Capture(const TacticalPhaseContext& phase) {
    const auto& s = phase.situation;
    return {true,false,false,phase.revision,phase.chapter_index,s.active_force,
        s.human_force,s.turn,s.turn_limit,s.control};
}
bool Matches(const NativeForceUpkeepProgress& progress, const TacticalPhaseContext& phase) {
    const auto& s = phase.situation;
    return progress.phase_revision == phase.revision && progress.chapter == phase.chapter_index &&
        progress.force == s.active_force && progress.human_force == s.human_force &&
        progress.turn == s.turn && progress.turn_limit == s.turn_limit && progress.control == s.control;
}
}
ForceTurnStatus RestoreUnitTurnState(UnitState& unit, const NativeUnitTurnState& input) noexcept {
    if (!unit.occupied || unit.force_type >= 9) return S::InvalidUnit;
    if (input.clone > UnitClonePresence::Present ||
        (!input.counter_135 && input.clone == UnitClonePresence::Unknown)) return S::InvalidCarriedState;
    if ((input.counter_135 && unit.turn.counter_135) ||
        (input.clone != UnitClonePresence::Unknown && unit.turn.clone != UnitClonePresence::Unknown)) return S::AlreadyBound;
    if (unit.map_end_revision == std::numeric_limits<std::uint64_t>::max()) return S::RevisionExhausted;
    if (input.counter_135) unit.turn.counter_135 = input.counter_135;
    if (input.clone != UnitClonePresence::Unknown) unit.turn.clone = input.clone;
    unit.combat_state_valid = false;
    ++unit.map_end_revision;
    return S::Ok;
}
ForceTurnResult ApplyCurrentForceTurnUpkeep(NativeRuntime& runtime, ForceTurnOperation operation,
    std::uint64_t expected_revision) {
    auto& game = runtime.game; const auto& phase = game.phase;
    const auto fail = [](S status, std::uint16_t slot = 0xffffu) { return ForceTurnResult{status,slot,0}; };
    if (operation > ForceTurnOperation::End) return fail(S::InvalidOperation);
    const bool begin = operation == ForceTurnOperation::Begin;
    const auto required = begin ? PhaseAccessStage::AwaitingEntryServices : PhaseAccessStage::AwaitingExitServices;
    if (!game.map_active || phase.stage != required || phase.situation.active_force >= 3 ||
        phase.situation.human_force >= 3) return fail(S::InvalidPhase);
    if (phase.chapter_index != game.campaign.current_chapter_index) return fail(S::StaleChapter);
    if (phase.revision != expected_revision) return fail(S::StaleRevision);
    auto progress = game.force_upkeep;
    if (!progress.bound || progress.phase_revision != phase.revision) progress = Capture(phase);
    else if (!Matches(progress,phase)) return fail(S::ChangedContext);
    if (begin ? progress.begin_done : progress.end_done) return fail(S::AlreadyApplied);
    const auto* order = GetVerifiedForceOrder(game,phase.situation.active_force);
    if (!order) return fail(S::MissingOrder);
    std::int16_t recovery_skill{};
    if (begin) {
        // Original cached skill lookup precedes the force loop, including empty
        // forces. Definition identifiers retain their original CP932 bytes.
        const auto* skill = runtime.definitions.FindSkill("SEID_\x90\x53\x93\xaa\x96\xc5\x8b\x70");
        if (!skill || skill->id > 32767u) return fail(S::MissingDefinition);
        recovery_skill = static_cast<std::int16_t>(skill->id);
    }
    struct Pending { std::uint16_t slot; UnitTurnSnapshot state; bool enhancement; };
    std::vector<Pending> pending; pending.reserve(order->count);
    for (unsigned i = 0; i < order->count; ++i) {
        const auto slot = order->slots[i]; const auto& unit = game.units[slot];
        if (unit.map_end_revision == std::numeric_limits<std::uint64_t>::max()) return fail(S::RevisionExhausted,slot);
        if (!begin && !unit.map_end.bound) return fail(S::MissingMapEnd,slot);
        if (begin && !(unit.flags & 0x800000u) && !unit.turn.counter_135) return fail(S::MissingCounter,slot);
        const bool enhancement = !(unit.flags & 0x40000u);
        bool recovery{};
        if (enhancement) {
            if (!unit.enhance.bound) return fail(S::MissingEnhance,slot);
            if (unit.turn.clone == UnitClonePresence::Unknown) return fail(S::MissingCloneState,slot);
            if (unit.turn.clone != UnitClonePresence::Absent) return fail(S::CloneOwnerRequired,slot);
            if (begin) {
                const auto skill = ProjectCurrentEquippedSkill(runtime,unit,recovery_skill);
                if (!skill) return fail(S::MissingSkill,slot);
                recovery = *skill;
            }
        }
        UnitTurnSnapshot next{unit.flags,unit.map_end.fields.secondary_flags,
            unit.map_end.fields.counters[0],unit.turn.counter_135.value_or(0),CurrentUnitEnhanceSnapshot(unit)};
        const bool clone = begin ? BeginUnitTurnBeginExact(next,recovery) : BeginUnitTurnEndExact(next);
        // Only a proven absent clone is admitted when the original call is
        // reached. No callback substitutes success for a present/unknown clone.
        if (clone && unit.turn.clone != UnitClonePresence::Absent) return fail(S::CloneOwnerRequired,slot);
        if (begin) FinishUnitTurnBeginExact(next); else FinishUnitTurnEndExact(next);
        pending.push_back({slot,next,enhancement});
    }
    for (const auto& update : pending) {
        auto& unit = game.units[update.slot]; const auto& next = update.state;
        unit.flags = next.flags;
        if (begin) {
            // The flag800000 branch preserves a possibly unknown counter.
            if (unit.turn.counter_135) unit.turn.counter_135 = next.counter_135;
        } else {
            unit.map_end.fields.secondary_flags = next.secondary_flags;
            unit.map_end.fields.counters[0] = next.counter_12d;
            // End the host command-action lifetime alongside original flag1
            // cleanup. This does not assert that the two fields are aliases.
            unit.action_committed = false;
        }
        if (update.enhancement) {
            unit.enhance.flags = next.enhance.flags; unit.enhance.values = next.enhance.values;
            unit.weakness = next.enhance.weakness;
        }
        unit.combat_state_valid = false; ++unit.map_end_revision;
    }
    if (begin) progress.begin_done = true; else progress.end_done = true;
    game.force_upkeep = progress;
    return {S::Ok,0xffffu,static_cast<std::uint16_t>(pending.size())};
}
}
