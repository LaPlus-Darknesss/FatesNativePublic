#pragma once
#include "fates/runtime/native_talk_control_effects.hpp"
namespace fates::runtime::native {
struct TalkModeFadeIdentity final {const std::uint32_t serial;};
using TalkModeFadeRequest=std::shared_ptr<const TalkModeFadeIdentity>;
struct TalkModeFadeObservation {
    TalkModeFadeRequest identity;ProcessHandle manager;
    TalkControlStatus status{TalkControlStatus::Ready};bool completed{};
    std::optional<std::uint32_t> result;
};
struct TalkPriorityState {
    // Original global halfwords +6DCDA8 (input base), +6BDCA4 (Talk output).
    // Explicitly supplied until the upstream priority initializer is owned.
    std::optional<std::uint16_t> base,window;
};
// Original t/F Dispose + Flash and ProcTalkManager mode/fade methods. Uses the
// actual current GameSkip/Fade/scheduler state, no separate renderer clock.
// Context mode is the SAME +1294 byte used by window lookup. New context fields
// retain original +12E0,+12F8,+1300,+14FA identities, never inferred from chapter.
class NativeTalkModeFade final:public ProcessCallbacks {
public:
    static TalkControlStatus Create(std::shared_ptr<NativeProcessScheduler>,std::shared_ptr<ProcessCallbackRegistry>,
        std::shared_ptr<NativeTalkControlContext>,std::shared_ptr<presentation::native::NativeTalkWindow>,
        std::shared_ptr<presentation::native::NativeFadeSystem>,std::shared_ptr<NativeGameSkip>,std::shared_ptr<NativeTalkModeFade>&);
    ~NativeTalkModeFade();
    NativeTalkModeFade(const NativeTalkModeFade&)=delete;NativeTalkModeFade& operator=(const NativeTalkModeFade&)=delete;
    TalkControlStatus RestorePriority(TalkPriorityState,ProcessAccess* =nullptr);
    std::optional<TalkPriorityState> Priority()const;
    TalkControlStatus Prepare(ProcessHandle,presentation::native::TalkWindowSource,std::size_t,TalkCodeOperation,char16_t handler,TalkModeFadeRequest&,ProcessAccess* =nullptr);
    std::optional<ProcessCall> Call(TalkModeFadeRequest)const;
    std::optional<TalkModeFadeObservation> Observe(TalkModeFadeRequest)const;
    TalkControlStatus Release(TalkModeFadeRequest,ProcessAccess* =nullptr);
    void Forget(TalkModeFadeRequest) noexcept;
    static ProcessCall SetTalkTypeCall(ProcessHandle,std::uint32_t);
    static ProcessCall FadeInCall(ProcessHandle,std::int32_t);
    static ProcessCall BlackOutCall(ProcessHandle,std::int32_t);
    static ProcessCall WhiteOutCall(ProcessHandle,std::int32_t);
    static ProcessCall IsFadingCall(ProcessHandle);
    bool UsesOwners(const NativeTalkControlContext&,const presentation::native::NativeTalkWindow&,const NativeGameSkip&)const noexcept;
    std::unique_ptr<ProcessContinuation> Begin(const ProcessCall&)override;
private:
    struct State;struct Continuation;explicit NativeTalkModeFade(std::shared_ptr<State>);
    std::shared_ptr<State> state_;
};
}
