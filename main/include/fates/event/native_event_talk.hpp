#pragma once
#include "fates/runtime/native_talk_manager_factory.hpp"

namespace fates::event::native {
class NativeProcEvent;
struct EventTalkIdentity final {const std::uint32_t serial;};
using EventTalkRequest=std::shared_ptr<const EventTalkIdentity>;
struct EventTalkObservation {
    EventTalkRequest identity;
    runtime::native::ProcessHandle caller,parent,manager,final_event;
    std::string message_identifier;
    runtime::native::TalkControlStatus status{runtime::native::TalkControlStatus::Ready};
    bool no_shadow{},started{},completed{},request_yield{};
};
// Original ev::Talk / TalkNoShadowFrame native entry. The current event is read
// from the same NativeProcEvent owner before factory and again after Initialize.
// This service never runs the VM or a second scheduler. Its result requests a
// write of the VM yield byte only when the final event's real child count != 0.
// The original void return is not converted into a known integer result.
class NativeEventTalk final:public runtime::native::ProcessCallbacks {
public:
    static runtime::native::TalkControlStatus Create(
        std::shared_ptr<runtime::native::NativeProcessScheduler>,std::shared_ptr<runtime::native::ProcessCallbackRegistry>,
        std::shared_ptr<runtime::native::NativeTalkManagerFactory>,std::shared_ptr<runtime::native::NativeTalkInitialize>,
        std::shared_ptr<runtime::native::NativeTalkControlContext>,std::shared_ptr<NativeEventTalk>&);
    ~NativeEventTalk();
    NativeEventTalk(const NativeEventTalk&)=delete;NativeEventTalk& operator=(const NativeEventTalk&)=delete;
    bool CanBindEvent()const noexcept;
    runtime::native::TalkControlStatus BindEvent(const std::shared_ptr<NativeProcEvent>&);
    runtime::native::TalkControlStatus Prepare(runtime::native::ProcessHandle,std::string_view,bool,
        EventTalkRequest&,runtime::native::ProcessAccess* =nullptr);
    std::optional<runtime::native::ProcessCall> Call(EventTalkRequest)const;
    std::optional<EventTalkObservation> Observe(EventTalkRequest)const;
    // Drops only host request bookkeeping, never a game process or allocation.
    void Forget(EventTalkRequest)noexcept;
    bool UsesScheduler(const runtime::native::NativeProcessScheduler&)const noexcept;
    bool UsesRuntime(const runtime::native::NativeRuntime&)const noexcept;
    std::unique_ptr<runtime::native::ProcessContinuation> Begin(const runtime::native::ProcessCall&)override;
private:
    struct State;struct Continuation;explicit NativeEventTalk(std::shared_ptr<State>);
    std::shared_ptr<State> state_;
};
}
