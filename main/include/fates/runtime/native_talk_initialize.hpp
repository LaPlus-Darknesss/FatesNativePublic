#pragma once
#include "fates/runtime/native_talk_lifecycle.hpp"
#include <string>
namespace fates::runtime::native {
struct TalkInitializeIdentity final {const std::uint32_t serial;};
using TalkInitializeRequest=std::shared_ptr<const TalkInitializeIdentity>;
struct TalkInitializeObservation {
    TalkInitializeRequest identity;ProcessHandle manager;
    TalkControlStatus status{TalkControlStatus::Ready};bool completed{};
    bool message_identifier{};std::string selected_identifier;
    TalkTextStatus text_status{TalkTextStatus::Ready};
};
// Original Initialize/InitializeDirect over each manager's existing expander,
// shared live text, window/log and Resume owners. Manager construction/global
// publication remain distinct. An existing manager must attach its own expander;
// two live managers cannot claim the same embedded text allocation.
// The initializers themselves supply NULL face identifiers for all three slots.
// No absent FaceManager service is reinterpreted as completed face creation.
class NativeTalkInitialize final:public ProcessCallbacks {
public:
    static TalkControlStatus Create(std::shared_ptr<NativeProcessScheduler>,std::shared_ptr<ProcessCallbackRegistry>,
        std::shared_ptr<NativeTalkControlContext>,std::shared_ptr<presentation::native::NativeTalkWindow>,
        std::shared_ptr<NativeTalkLog>,std::shared_ptr<NativeTalkTokens>,std::shared_ptr<NativeGameSkip>,
        std::shared_ptr<NativeTalkLifecycle>,std::shared_ptr<NativeTalkInitialize>&);
    ~NativeTalkInitialize();
    NativeTalkInitialize(const NativeTalkInitialize&)=delete;NativeTalkInitialize& operator=(const NativeTalkInitialize&)=delete;
    TalkControlStatus AttachExpander(ProcessHandle,std::shared_ptr<NativeTalkText>,ProcessAccess* =nullptr);
    TalkControlStatus DetachExpander(ProcessHandle,ProcessAccess* =nullptr);
    TalkControlStatus PrepareDirect(ProcessHandle,presentation::native::TalkWindowSource,TalkInitializeRequest&,ProcessAccess* =nullptr);
    TalkControlStatus PrepareIdentifier(ProcessHandle,std::string_view,TalkInitializeRequest&,ProcessAccess* =nullptr);
    std::optional<ProcessCall> Call(TalkInitializeRequest) const;
    std::optional<TalkInitializeObservation> Observe(TalkInitializeRequest) const;
    TalkControlStatus Release(TalkInitializeRequest,ProcessAccess* =nullptr);
    void Forget(TalkInitializeRequest) noexcept;
    static ProcessCall ShadowInCall(ProcessHandle);
    static ProcessCall ShadowOutCall(ProcessHandle);
    // Exact original vtable targets for the ALREADY existing canonical program.
    // Persistent and derived destruction deliberately remain unregistered here;
    // callers cannot replace them with base no-ops and claim a complete manager.
    static ProcessType ManagerType();
    bool UsesOwners(const NativeTalkControlContext&,const presentation::native::NativeTalkWindow&,const NativeTalkTokens&)const noexcept;
    bool UsesScheduler(const NativeProcessScheduler&) const noexcept;
    std::unique_ptr<ProcessContinuation> Begin(const ProcessCall&) override;
private:
    struct State;struct Continuation;explicit NativeTalkInitialize(std::shared_ptr<State>);
    std::shared_ptr<State> state_;
};
}
