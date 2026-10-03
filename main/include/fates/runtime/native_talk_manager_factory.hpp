#pragma once
#include "fates/runtime/native_talk_manager_storage.hpp"

namespace fates::runtime::native {
struct TalkManagerFactoryIdentity final {const std::uint32_t serial;};
using TalkManagerFactoryRequest=std::shared_ptr<const TalkManagerFactoryIdentity>;
struct TalkManagerFactoryObservation {
    TalkManagerFactoryRequest identity;
    ProcessHandle caller,parent,manager,auxiliary;
    TalkControlStatus status{TalkControlStatus::Ready};
    bool started{},completed{},reused{},allocated{},attached{},auxiliary_created{};
};
// Original CreateInstanceBind(ProcInst*) at 001E56A0. The scheduler owns a
// detached base while the real member constructor runs; only then does Create
// attach it, republish the global and create the caller-blocking ProcBind sibling.
// A known existing global is returned unchanged, even if a different parent was
// requested. It does not recreate a cleared auxiliary or repeat initialization.
// Native request identities are host continuation bookkeeping, not game objects.
class NativeTalkManagerFactory final:public ProcessCallbacks {
public:
    static TalkControlStatus Create(std::shared_ptr<NativeProcessScheduler>,std::shared_ptr<ProcessCallbackRegistry>,
        std::shared_ptr<NativeTalkManagerStorage>,std::shared_ptr<NativeTalkControlContext>,
        std::shared_ptr<NativeTalkLifecycle>,std::shared_ptr<NativeTalkManagerFactory>&);
    ~NativeTalkManagerFactory();
    NativeTalkManagerFactory(const NativeTalkManagerFactory&)=delete;
    NativeTalkManagerFactory& operator=(const NativeTalkManagerFactory&)=delete;
    // caller is the already live service-call context; parent is the original
    // argument. Parent validation is deliberately NOT a prerequisite for reuse.
    TalkControlStatus Prepare(ProcessHandle caller,ProcessHandle parent,TalkManagerFactoryRequest&,ProcessAccess* =nullptr);
    std::optional<ProcessCall> Call(TalkManagerFactoryRequest)const;
    std::optional<TalkManagerFactoryObservation> Observe(TalkManagerFactoryRequest)const;
    TalkControlStatus Release(TalkManagerFactoryRequest,ProcessAccess* =nullptr);
    const NativeRuntime& runtime() const noexcept;
    bool UsesOwners(const NativeTalkControlContext&,const NativeTalkInitialize&) const noexcept;
    bool UsesScheduler(const NativeProcessScheduler&)const noexcept;
    std::unique_ptr<ProcessContinuation> Begin(const ProcessCall&)override;
private:
    struct State;struct Continuation;explicit NativeTalkManagerFactory(std::shared_ptr<State>);
    std::shared_ptr<State> state_;
};
}
