#pragma once
#include "fates/event/native_phase_event_catalog.hpp"
#include "fates/cmvm/native_script_functions.hpp"
#include "fates/cmvm/native_script_session.hpp"
#include "fates/event/native_phase_event_queries.hpp"
#include "fates/event/native_event_flag_commands.hpp"
#include "fates/event/native_unit_event_queries.hpp"
#include "fates/map/native_camera_wait.hpp"
#include "fates/event/native_event_camera.hpp"
#include "fates/event/native_event_talk.hpp"
#include <optional>
#include <string>

namespace fates::event::native {
class NativeProcEvent;
// Borrowed capability constructed only by the real current ProcEvent callback.
// It is never retained by the VM or a camera continuation beyond Run.
class ProcEventVmAccess final {
    friend class NativeProcEvent;
    ProcEventVmAccess(runtime::native::ProcessAccess& a,runtime::native::ProcessHandle p):access_(a),current_(std::move(p)){}
public:
    ProcEventVmAccess(const ProcEventVmAccess&)=delete;
    ProcEventVmAccess& operator=(const ProcEventVmAccess&)=delete;
    runtime::native::ProcessAccess& process_access() const noexcept{return access_;}
    const runtime::native::ProcessHandle& current_event() const noexcept{return current_;}
private:
    runtime::native::ProcessAccess& access_;
    runtime::native::ProcessHandle current_;
};
struct NativeEventServices {
    std::shared_ptr<const NativePhaseEventQueries> phase;
    std::shared_ptr<const NativeEventFlagCommands> flags;
    std::shared_ptr<const NativeUnitEventQueries> units;
    std::shared_ptr<map::native::NativeCameraWait> camera;
    std::shared_ptr<NativeEventCamera> camera_commands;
    std::shared_ptr<NativeEventTalk> talk;
    bool UsesOneRuntime() const noexcept {
        const runtime::native::NativeRuntime* owner=nullptr;
        const auto check=[&](const auto& service){if(!service)return true;const auto* r=service->runtime().get();
            if(owner && owner!=r)return false;
            owner=r;return true;};
        return check(phase)&&check(flags)&&check(units)&&check(camera)&&check(camera_commands) &&
            (!owner || !talk || talk->UsesRuntime(*owner));
    }
};
enum class PhaseVmStatus : std::uint8_t {
    Ready, InstructionBudget, Returned, Yielded, InvalidSelection, StackLimit,
    CodeBounds, StackUnderflow, UnknownLocal, InvalidLocal, InvalidValue,
    InvalidString, UnsupportedOpcode, IdentifierCallOwnerRequired,
    ScriptCallOwnerRequired, InterpreterMismatch, InvalidFunction, InvalidCall,
    FrameLimit, DanglingLocal, NativeCallOwnerRequired, InvalidNativeArguments,
    NativeStateUnavailable, StaleNativeContext, DivisionByZero, StaleScriptSession,
    MismatchedNativeContext, ContextNotReturned, NativeCallPending
};
struct RetainedVmArchiveAddress {
    std::shared_ptr<const PhaseEventArchive> archive;
    std::uint32_t offset{};
};
struct PhaseVmObservation {
    PhaseVmStatus status{PhaseVmStatus::Ready};
    std::uint32_t code_offset{};
    std::uint64_t instructions{};
    std::size_t evaluation_words{};
    std::optional<std::int32_t> return_value;
    std::optional<std::uint8_t> opcode;
    std::string identifier;
    std::uint16_t call_index{};
    std::uint8_t call_arguments{};
    std::size_t frame_depth{};
    cmvm::native::ScriptFunctionRef current_function;
    std::optional<RetainedVmArchiveAddress> return_archive_address;
    std::string registered_identifier;
    std::optional<PhaseQueryStatus> native_query_status;
    std::uint64_t native_queries{};
    std::optional<runtime::native::EventFlagStatus> native_flag_status;
    std::uint64_t native_flags{};
    std::optional<runtime::native::EventFlagResult> return_flag_bits_address;
    std::optional<UnitEventQueryResult> native_unit_result;
    std::uint64_t native_units{};
    std::optional<map::native::CameraWaitStatus> native_camera_status;
    std::uint64_t native_cameras{};
    std::optional<EventCameraStatus> native_camera_command_status;
    std::uint64_t native_camera_commands{};
    std::optional<runtime::native::TalkControlStatus> native_talk_status;
    std::uint64_t native_talk_calls{};
};

// Owns the selected phase function and its retained nested script frames.
// Uses the existing CmContext interpreter after preflighting every supported
// instruction. Unowned calls stop BEFORE execution; missing registration never becomes
// the interpreter's retail missing-identifier return-zero path. Locals remain
// unknown until assigned; zero-filled host allocation is not carried VM state.
// Host instruction-budget suspension is not a retail event yield. Returned is
// only the outer function's return, not ProcEvent/phase completion. Named calls
// use an explicitly supplied registry snapshot; index calls use the retained
// current archive. Optional concrete queries use the live native runtime owner;
// missing query state can be retried, while changed/retired phase contexts are
// terminal refusals. Concrete flag services prepare before the interpreter step
// and commit once from its native hook; unknown callbacks remain barriers.
// Local tags carry frame lifetimes; archive/flag addresses retain their owners.
class PhaseEventVm final {
public:
    static PhaseVmStatus CreateWithServices(const PhaseEventSelection&,std::size_t stack_words,
        std::shared_ptr<const cmvm::native::ScriptFunctionRegistry>,NativeEventServices,
        std::unique_ptr<PhaseEventVm>&);
    static PhaseVmStatus CreateAttachedWithServices(const cmvm::native::AttachedPhaseSelection&,std::size_t stack_words,
        std::shared_ptr<const cmvm::native::ScriptAttachmentSession>,NativeEventServices,
        std::unique_ptr<PhaseEventVm>&);
    static PhaseVmStatus CreateAttachedFunctionWithServices(const cmvm::native::AttachedScriptFunction&,
        std::span<const std::int32_t> arguments,std::size_t stack_words,
        std::shared_ptr<const cmvm::native::ScriptAttachmentSession>,NativeEventServices,
        std::unique_ptr<PhaseEventVm>&);
    static PhaseVmStatus Create(const PhaseEventSelection&, std::size_t stack_words,
        std::unique_ptr<PhaseEventVm>&);
    static PhaseVmStatus Create(const PhaseEventSelection&, std::size_t stack_words,
        std::shared_ptr<const cmvm::native::ScriptFunctionRegistry>, std::unique_ptr<PhaseEventVm>&);
    static PhaseVmStatus Create(const PhaseEventSelection&, std::size_t stack_words,
        std::shared_ptr<const cmvm::native::ScriptFunctionRegistry>,
        std::shared_ptr<const NativePhaseEventQueries>, std::unique_ptr<PhaseEventVm>&);
    static PhaseVmStatus Create(const PhaseEventSelection&, std::size_t stack_words,
        std::shared_ptr<const cmvm::native::ScriptFunctionRegistry>,
        std::shared_ptr<const NativePhaseEventQueries>,std::shared_ptr<const NativeEventFlagCommands>,
        std::unique_ptr<PhaseEventVm>&);
    // Uses current session lookup for each named call. Frame attachment tokens
    // survive yields and permanently reject detachment/retirement, even after
    // the same archive is attached again under a new token.
    static PhaseVmStatus CreateAttached(const cmvm::native::AttachedPhaseSelection&,std::size_t stack_words,
        std::shared_ptr<const cmvm::native::ScriptAttachmentSession>,
        std::shared_ptr<const NativePhaseEventQueries>,std::unique_ptr<PhaseEventVm>&);
    static PhaseVmStatus CreateAttached(const cmvm::native::AttachedPhaseSelection&,std::size_t stack_words,
        std::shared_ptr<const cmvm::native::ScriptAttachmentSession>,
        std::shared_ptr<const NativePhaseEventQueries>,std::shared_ptr<const NativeEventFlagCommands>,
        std::unique_ptr<PhaseEventVm>&);
    // Shared root entry for typed events and ordinary script functions. Only
    // type0 consumes supplied integer arguments; event metadata is not VM input.
    // This does not select an inspector or own InstantCall/ProcEvent scheduling.
    static PhaseVmStatus CreateAttachedFunction(const cmvm::native::AttachedScriptFunction&,
        std::span<const std::int32_t> arguments,std::size_t stack_words,
        std::shared_ptr<const cmvm::native::ScriptAttachmentSession>,
        std::shared_ptr<const NativePhaseEventQueries>,std::unique_ptr<PhaseEventVm>&);
    static PhaseVmStatus CreateAttachedFunction(const cmvm::native::AttachedScriptFunction&,
        std::span<const std::int32_t> arguments,std::size_t stack_words,
        std::shared_ptr<const cmvm::native::ScriptAttachmentSession>,
        std::shared_ptr<const NativePhaseEventQueries>,std::shared_ptr<const NativeEventFlagCommands>,
        std::unique_ptr<PhaseEventVm>&);
    ~PhaseEventVm();
    PhaseEventVm(const PhaseEventVm&) = delete;
    PhaseEventVm& operator=(const PhaseEventVm&) = delete;
    PhaseVmObservation Run(std::size_t instruction_budget);
    PhaseVmObservation Run(std::size_t instruction_budget,const ProcEventVmAccess&);
    // Only the current ProcEvent callback can consume this already-prepared
    // nested service. The VM commits its native opcode after that service has
    // actually returned, retaining operands/PC across all nested barriers.
    std::optional<runtime::native::ProcessCall> PendingNativeCall(const ProcEventVmAccess&) const;
    // SetFunction on this same context/stack after an outer return. Both the old
    // and next attachment must still belong to this session. Rejection leaves
    // the context and observation untouched. Locals do not inherit value tags.
    // Cumulative counters and the last completed return are retained; per-call
    // diagnostics reset on success. This does not perform typed traversal.
    PhaseVmStatus RetargetAttachedFunction(const cmvm::native::AttachedScriptFunction&,
        std::span<const std::int32_t> arguments={});
    const PhaseVmObservation& observation() const noexcept;
    const cmvm::native::ScriptFunctionRef& root_function() const noexcept;
    // Empty for a direct function entry; phase factories retain their selection.
    const PhaseEventSelection& selection() const noexcept;
private:
    struct State;
    static PhaseVmStatus CreateRoot(const cmvm::native::ScriptFunctionRef&,
        std::span<const std::int32_t>,std::size_t,std::unique_ptr<State>,std::unique_ptr<PhaseEventVm>&);
    explicit PhaseEventVm(std::unique_ptr<State>);
    PhaseVmObservation RunImpl(std::size_t,const ProcEventVmAccess*);
    std::unique_ptr<State> state_;
};
}
