#pragma once
#include "fates/event/native_phase_event_vm.hpp"

namespace fates::event::native {
enum class InstantCallStatus : std::uint8_t {
    Ready, InstructionBudget, Complete, VmBarrier, NullSession, StaleSession,
    StaleNativeContext, MismatchedNativeContext, Retired
};
struct InstantCallObservation {
    InstantCallStatus status{InstantCallStatus::Ready};
    std::uint32_t type{};
    std::uint64_t instructions{},contexts_started{},contexts_destroyed{};
    std::uint64_t returned{},yielded{},native_flags{},native_queries{};
    bool context_alive{};
    cmvm::native::AttachedScriptFunction function;
    std::optional<PhaseVmObservation> vm;
};
// event::InstantCall(Type): visit the current attached list without an inspector,
// create a fresh256-word context for each function, execute one interpreter tick,
// then destroy that context even if it yielded inside a nested function. A host
// work budget may suspend that tick, but must never count as a retail yield.
// Missing native owners remain a barrier with the current context retained; they
// cannot be silently consumed as a finished tick. Complete means this traversal
// finished, not chapter bootstrap/ProcEvent/phase completion.
class NativeTypedInstantCall final {
public:
    static constexpr std::size_t ContextWords=256;
    static InstantCallStatus Create(std::uint32_t,
        std::shared_ptr<const cmvm::native::ScriptAttachmentSession>,
        std::shared_ptr<const NativePhaseEventQueries>,
        std::shared_ptr<const NativeEventFlagCommands>,
        std::unique_ptr<NativeTypedInstantCall>&);
    ~NativeTypedInstantCall();
    NativeTypedInstantCall(const NativeTypedInstantCall&)=delete;
    NativeTypedInstantCall& operator=(const NativeTypedInstantCall&)=delete;
    InstantCallObservation Run(std::size_t instruction_budget);
    const InstantCallObservation& observation() const noexcept {return observation_;}
    void Retire() noexcept;
private:
    NativeTypedInstantCall()=default;
    InstantCallStatus Validate() const noexcept;
    std::shared_ptr<const cmvm::native::ScriptAttachmentSession> session_;
    std::shared_ptr<const NativePhaseEventQueries> queries_;
    std::shared_ptr<const NativeEventFlagCommands> flags_;
    std::unique_ptr<PhaseEventVm> vm_;
    InstantCallObservation observation_;
};
}
