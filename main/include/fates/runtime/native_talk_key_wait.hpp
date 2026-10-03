#pragma once
#include "fates/runtime/native_game_skip.hpp"
#include "fates/runtime/native_talk_log.hpp"
#include "fates/presentation/native_talk_window_effects.hpp"

namespace fates::runtime::native {
// Same carried manager scope as the effect owner; the future actual manager
// supplies its current window. Outer nullopt is unknown, a null handle is the
// original known-null pointer (not a valid substitute for a required window).
class TalkKeyWaitManagerState:public presentation::native::TalkEffectManagerState {
public:
    virtual std::optional<presentation::native::TalkWindowHandle> CurrentWindow(ProcessHandle) const=0;
};
enum class TalkKeyWaitStatus:std::uint8_t {Ready,NullScheduler,MismatchedDomain,DuplicateBinding,Busy,Retired,InvalidParent,InvalidHandle};
struct TalkKeyWaitSnapshot {
    ProcessHandle process,manager;
    std::uint32_t counter{1};
    std::uint64_t deletion_requests{},skip_jumps{},confirm_calls{},log_viewer_calls{};
};
// TcdiKeyWait::Dispose and the real TalkKeyWait Tick/destructor. Reuses the same
// GameSkipInputSource as NativeGameSkip: trigger word+14 and Tutorial+4B4, not a
// separate keyboard snapshot or guessed "pressed" boolean. No device polling,
// voice engine, TalkLogViewer implementation, or complete Talk manager claimed.
//
// Explicit external callbacks through the EXISTING registry:
// 41F6C4: StopAllVoice(milliseconds=100), constructor skip branch.
// 4200FC: play original SE_SYS_MESSAGE_NEXT1 literal tag 1A6510; result ignored.
// 1CB1F8: create a real blocking log viewer under call.process, arguments
//         [current NativeTalkLog allocation serial (or known0), blocking1].
// The provider must validate that allocation against the SAME NativeTalkLog.
// Missing callbacks suspend; they never become completed input/log/audio effects.
class NativeTalkKeyWait final:public ProcessCallbacks {
public:
    static TalkKeyWaitStatus Create(std::shared_ptr<NativeProcessScheduler>,std::shared_ptr<ProcessCallbackRegistry>,
        std::shared_ptr<NativeGameSkip>,std::shared_ptr<GameSkipInputSource>,
        std::shared_ptr<presentation::native::NativeTalkWindow>,std::shared_ptr<presentation::native::NativeTalkWindowEffects>,
        std::shared_ptr<TalkKeyWaitManagerState>,std::shared_ptr<NativeTalkLog>,std::shared_ptr<NativeTalkKeyWait>&);
    ~NativeTalkKeyWait();
    NativeTalkKeyWait(const NativeTalkKeyWait&)=delete;
    NativeTalkKeyWait& operator=(const NativeTalkKeyWait&)=delete;
    // The handler returns original code2 after constructing its blocking child,
    // or after the real voice-stop call when manager skip byte is already set.
    std::optional<ProcessCall> DisposeCall(ProcessHandle manager) const;
    TalkKeyWaitStatus RestoreCounter(ProcessHandle,std::uint32_t);
    std::optional<TalkKeyWaitSnapshot> Observe(ProcessHandle) const;
    std::vector<ProcessHandle> Processes() const;
    bool UsesOwners(const TalkKeyWaitManagerState&,const NativeTalkLog&,const presentation::native::NativeTalkWindow&) const noexcept;
    bool UsesScheduler(const NativeProcessScheduler&) const noexcept;
    std::unique_ptr<ProcessContinuation> Begin(const ProcessCall&) override;
private:
    struct State;struct Continuation;
    explicit NativeTalkKeyWait(std::shared_ptr<State>);
    std::shared_ptr<State> state_;
};
}
