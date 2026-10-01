#pragma once
#include "fates/runtime/native_process.hpp"

namespace fates::presentation::native {
using FadeColor=std::array<std::uint8_t,4>;
enum class FadeTone:std::uint8_t {Black,White};
enum class FadeDirection:std::uint8_t {In,Out};
enum class FadeStatus:std::uint8_t {Ready,NullScheduler,MissingSink,MismatchedRegistry,DuplicateBinding};
struct FadeDraw {
    runtime::native::ProcessHandle process;
    std::uint8_t logical_target{};
    FadeColor color{};
    std::uint32_t priority{1000};
};
// Full logical-target rectangle. A PC consumer maps it to its presentation
// layout; the native fade owns all timing/color/active/wait state. The output
// service accepts synchronously and never supplies gameplay completion.
class FadeDrawSink {
public:
    virtual ~FadeDrawSink()=default;
    virtual void Draw(const FadeDraw&)=0;
};
class NullFadeDrawSink final:public FadeDrawSink {
public:
    void Draw(const FadeDraw&) override {}
};
struct FadeProcessSnapshot {
    runtime::native::ProcessHandle process;
    std::uint8_t logical_target{},tone{},direction{};
    std::int32_t elapsed{},duration{};
    FadeColor start{},goal{},color{};
    bool active{},first_draw_pending{},wait_process{};
};
struct FadeChannelSnapshot {
    runtime::native::ProcessHandle process;
    FadeColor color{},goal{};
    bool active{},active_fade_in{},blackout{};
};
// Concrete original FadeCreate, ProcFade and FadeWait owners. All process
// callbacks run through the shared scheduler; no parallel fade clock exists.
// Initialize clears current channel bindings exactly as retail, without deleting
// already-linked fade processes. Destruction clears only its own current binding.
class NativeFadeSystem final:public runtime::native::ProcessCallbacks {
public:
    static FadeStatus Create(std::shared_ptr<runtime::native::NativeProcessScheduler>,
        std::shared_ptr<runtime::native::ProcessCallbackRegistry>,std::shared_ptr<FadeDrawSink>,
        std::shared_ptr<NativeFadeSystem>&);
    ~NativeFadeSystem();
    NativeFadeSystem(const NativeFadeSystem&)=delete;
    NativeFadeSystem& operator=(const NativeFadeSystem&)=delete;
    runtime::native::ProcessStatus BeginFade(std::int32_t milliseconds,std::uint32_t target,FadeTone,FadeDirection);
    runtime::native::ProcessStatus BeginWaitBind(runtime::native::ProcessHandle parent,std::uint32_t target);
    runtime::native::ProcessStatus BeginInitialize();
    // Reusable invocation for another native continuation to await. Context is
    // the invoking process identity, not a fabricated retail this pointer.
    static runtime::native::ProcessCall FadeCall(runtime::native::ProcessHandle context,
        std::int32_t milliseconds,std::uint32_t target,FadeTone,FadeDirection);
    static runtime::native::ProcessCall WaitCall(runtime::native::ProcessHandle parent,std::uint32_t target);
    std::optional<FadeChannelSnapshot> ObserveChannel(std::uint32_t target) const;
    std::vector<FadeProcessSnapshot> ObserveProcesses() const;
    std::optional<bool> IsBlackOutAll() const;
    bool UsesScheduler(const runtime::native::NativeProcessScheduler&) const noexcept;
    std::unique_ptr<runtime::native::ProcessContinuation> Begin(const runtime::native::ProcessCall&) override;
private:
    struct State;
    struct Continuation;
    explicit NativeFadeSystem(std::shared_ptr<State>);
    std::shared_ptr<State> state_;
};
}
