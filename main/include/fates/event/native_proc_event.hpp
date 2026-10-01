#pragma once
#include "fates/event/native_phase_event_vm.hpp"
#include "fates/runtime/native_game_skip.hpp"
#include "fates/runtime/native_map_binder.hpp"
#include "fates/runtime/native_resource_delay.hpp"

namespace fates::event::native {
enum class ProcEventStatus:std::uint8_t {
    Ready,NoMatch,AlreadyActive,NullScheduler,MismatchedDomain,DuplicateBinding,
    NullSession,StaleSession,StaleNativeContext,UnknownState,InvalidParent,
    InvalidFunction,VmRefused,Busy,Retired,InvalidHandle,InvalidKind
};
enum class ProcEventInspector:std::uint8_t {None,Phase};
struct ProcEventSnapshot {
    runtime::native::ProcessHandle process;
    std::uint8_t type{};
    std::uint32_t flags{};
    ProcEventInspector inspector{};
    cmvm::native::AttachedScriptFunction function;
    runtime::native::GameSkipControlHandle skip_control;
    PhaseVmObservation vm;
    std::uint64_t roots_started{};
};
// Single active ProcEvent. Outer VM return is not event completion: the original
// descriptor still performs fade/skip waits, resource cleanup, next-root search
// and deferred destruction. Unowned cleanup/ConsistencyCheck services remain
// real scheduler barriers, not externally acknowledged completion.
class NativeProcEvent final:public runtime::native::ProcessCallbacks {
public:
    static ProcEventStatus CreateWithServices(std::shared_ptr<runtime::native::NativeProcessScheduler>,
        std::shared_ptr<runtime::native::ProcessCallbackRegistry>,
        std::shared_ptr<runtime::native::NativeGameSkip>,
        std::shared_ptr<presentation::native::NativeFadeSystem>,
        std::shared_ptr<runtime::native::NativeMapBinder>,
        std::shared_ptr<runtime::native::NativeResourceDelay>,
        std::shared_ptr<const cmvm::native::ScriptAttachmentSession>,NativeEventServices,
        std::shared_ptr<NativeProcEvent>&);
    static ProcEventStatus Create(std::shared_ptr<runtime::native::NativeProcessScheduler>,
        std::shared_ptr<runtime::native::ProcessCallbackRegistry>,
        std::shared_ptr<runtime::native::NativeGameSkip>,
        std::shared_ptr<presentation::native::NativeFadeSystem>,
        std::shared_ptr<runtime::native::NativeMapBinder>,
        std::shared_ptr<runtime::native::NativeResourceDelay>,
        std::shared_ptr<const cmvm::native::ScriptAttachmentSession>,
        std::shared_ptr<const NativePhaseEventQueries>,
        std::shared_ptr<const NativeEventFlagCommands>,std::shared_ptr<NativeProcEvent>&);
    ~NativeProcEvent();
    NativeProcEvent(const NativeProcEvent&)=delete;
    NativeProcEvent& operator=(const NativeProcEvent&)=delete;
    ProcEventStatus CreateTyped(runtime::native::ProcessHandle parent,std::uint32_t type,
        ProcEventInspector,runtime::native::ProcessHandle&);
    ProcEventStatus CreateTyped(runtime::native::ProcessAccess&,runtime::native::ProcessHandle parent,
        std::uint32_t type,ProcEventInspector,runtime::native::ProcessHandle&);
    ProcEventStatus CreateFunction(runtime::native::ProcessHandle parent,
        const cmvm::native::AttachedScriptFunction&,runtime::native::ProcessHandle&);
    ProcEventStatus CreateFunction(runtime::native::ProcessAccess&,runtime::native::ProcessHandle parent,
        const cmvm::native::AttachedScriptFunction&,runtime::native::ProcessHandle&);
    // The module survives phase changes. Install the next phase's validated
    // service bindings only after the previous event was actually destroyed.
    ProcEventStatus BindContextServices(std::shared_ptr<const NativePhaseEventQueries>,
        std::shared_ptr<const NativeEventFlagCommands>);
    ProcEventStatus BindContextServices(runtime::native::ProcessAccess&,
        std::shared_ptr<const NativePhaseEventQueries>,std::shared_ptr<const NativeEventFlagCommands>);
    // Explicit write to the original +58 word for concrete event command owners.
    // This is not admission or implementation of any unowned script command.
    ProcEventStatus SetFlags(runtime::native::ProcessHandle,std::uint32_t);
    ProcEventStatus SetFlags(runtime::native::ProcessAccess&,runtime::native::ProcessHandle,std::uint32_t);
    std::optional<runtime::native::ProcessHandle> Current() const;
    std::optional<ProcEventSnapshot> Observe(runtime::native::ProcessHandle) const;
    std::uint64_t contexts_started() const noexcept;
    std::uint64_t contexts_destroyed() const noexcept;
    bool UsesScheduler(const runtime::native::NativeProcessScheduler&) const noexcept;
    bool UsesRuntime(const runtime::native::NativeRuntime&) const noexcept;
    static std::shared_ptr<const runtime::native::ProcessProgram> Program();
    std::unique_ptr<runtime::native::ProcessContinuation> Begin(const runtime::native::ProcessCall&) override;
private:
    struct State;
    struct Continuation;
    explicit NativeProcEvent(std::shared_ptr<State>);
    std::shared_ptr<State> state_;
};
}
