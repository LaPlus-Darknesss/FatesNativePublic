#pragma once
#include "fates/runtime/native_phase_exit.hpp"
#include "fates/event/native_proc_event.hpp"
#include "fates/map/native_deployment_workspace.hpp"
#include "fates/map/native_map_cursor.hpp"

namespace fates::runtime::native {
enum class PhaseSequenceStatus : std::uint8_t {
    Ready, NullRuntime, NullScheduler, MismatchedDomain, DuplicateBinding,
    Busy, InvalidProcess, AlreadyBound, StaleContext, UnknownVersusState,
    VersusSenderRequired, EventBlocked, AwaitingEventDestruction, ExitBlocked,
    FlagsBlocked, ForceOrderRequired, RevisionExhausted, EntryBlocked, CursorBlocked
};
struct PhaseSequenceObservation {
    PhaseSequenceStatus status{PhaseSequenceStatus::Ready};
    ProcessHandle process;
    event::native::ProcEventStatus event_status{event::native::ProcEventStatus::Ready};
    std::optional<PhaseExitObservation> exit;
    std::optional<std::uint32_t> branch_label;
    EventFlagStatus flags_status{EventFlagStatus::Ok};
    std::optional<ForceTurnResult> entry_upkeep;
    std::optional<map::native::DeploymentWorkspaceStatus> deployment_status;
    std::optional<map::native::TurnCursorSelection> cursor;
};
// Original60-record phase program, four event triggers, TurnBegin/End and the reached
// GameEndBranch/TurnSkip/TurnBranch callbacks. Branch selection does not publish
// Ready: the actual selected child still needs its concrete lifetime owner.
// Admission carries an EXISTING process and explicit versus-config presence.
// It does not stand in for Sequence::Create, message archives, its destructor,
// remaining entry services, audio or dispatch children; their real targets still block.
// The host must complete Sweep between Exec ticks, so ProcEvent destruction and
// ConsistencyCheck finish before a subsequent phase context is published.
class NativePhaseSequence final:public ProcessCallbacks {
public:
    static PhaseSequenceStatus Create(std::shared_ptr<NativeRuntime>,
        std::shared_ptr<NativeProcessScheduler>,std::shared_ptr<ProcessCallbackRegistry>,
        std::shared_ptr<event::native::NativeProcEvent>,std::shared_ptr<NativePhaseSequence>&,
        std::shared_ptr<map::native::NativeMapCursor> cursor={});
    ~NativePhaseSequence();
    NativePhaseSequence(const NativePhaseSequence&)=delete;
    NativePhaseSequence& operator=(const NativePhaseSequence&)=delete;
    PhaseSequenceStatus BindCarried(ProcessHandle,std::optional<bool> has_versus_config);
    PhaseSequenceObservation Observe() const;
    static std::shared_ptr<const ProcessProgram> Program();
    static ProcessType Type();
    std::unique_ptr<ProcessContinuation> Begin(const ProcessCall&) override;
private:
    struct State;struct Continuation;
    explicit NativePhaseSequence(std::shared_ptr<State>);
    std::shared_ptr<State> state_;
};
}
