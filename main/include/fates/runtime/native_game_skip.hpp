#pragma once
#include "fates/presentation/native_fade.hpp"
#include <string_view>

namespace fates::runtime::native {
struct GameSkipIdentity final {const std::uint64_t serial;};
struct GameSkipControlIdentity final {const std::uint64_t serial;};
using GameSkipHandle=std::shared_ptr<const GameSkipIdentity>;
using GameSkipControlHandle=std::shared_ptr<const GameSkipControlIdentity>;
// A projection of the actual tutorial singleton and its +4B4 word, not an
// injected IsActive result. Missing projection stays unknown.
struct GameSkipTutorialView {bool present{};std::uint32_t mode{};};
class GameSkipInputSource {
public:
    virtual ~GameSkipInputSource()=default;
    virtual std::optional<std::uint32_t> TriggerButtons() const=0;
    virtual std::optional<GameSkipTutorialView> Tutorial() const=0;
};
class GameSkipAudioSink {
public:
    virtual ~GameSkipAudioSink()=default;
    virtual void StopBgm(std::uint32_t milliseconds)=0;
    virtual void StopLse(std::uint32_t milliseconds)=0;
};
class NullGameSkipAudioSink final:public GameSkipAudioSink {
public:
    void StopBgm(std::uint32_t) override {}
    void StopLse(std::uint32_t) override {}
};
struct GameSkipDraw {
    GameSkipHandle owner;
    ProcessHandle process;
    presentation::native::FadeColor color{};
    // Original font0/status text, priority1001, logical target0. The output
    // consumer measures/localizes text and anchors it8px from bottom/right
    // using20px lines in the original400x240 coordinate space, then maps to PC.
    std::uint32_t priority{1001};
    std::string_view message_key_cp932;
};
class GameSkipDrawSink {
public:
    virtual ~GameSkipDrawSink()=default;
    virtual void Draw(const GameSkipDraw&)=0;
};
class NullGameSkipDrawSink final:public GameSkipDrawSink {
public:
    void Draw(const GameSkipDraw&) override {}
};
enum class GameSkipStatus:std::uint8_t {
    Ready,NullScheduler,MissingInput,MissingSink,MismatchedDomain,DuplicateBinding,
    Busy,Retired,UnknownCurrent,InvalidHandle,IdentityExhausted
};
enum class GameSkipOperation:std::uint8_t {Tick,Trigger,Escape,Destroy,Render};
enum class GameSkipControlOperation:std::uint8_t {Enable,Escape,Disable,Rollback};
struct GameSkipSnapshot {
    GameSkipHandle identity;
    std::uint8_t state{};
    std::uint32_t flags{1};
    std::int32_t wait_counter{};
    std::optional<std::uint32_t> render_counter;
    std::array<std::optional<presentation::native::FadeColor>,2> saved_goals;
    ProcessHandle renderer;
};
struct GameSkipControlSnapshot {
    GameSkipControlHandle identity;
    GameSkipHandle captured;
    std::optional<bool> was_skipping;
    std::optional<std::uint32_t> flags;
};
class NativeGameSkip final:public ProcessCallbacks {
public:
    static GameSkipStatus Create(std::shared_ptr<NativeProcessScheduler>,
        std::shared_ptr<ProcessCallbackRegistry>,std::shared_ptr<presentation::native::NativeFadeSystem>,
        std::shared_ptr<GameSkipInputSource>,std::shared_ptr<GameSkipAudioSink>,
        std::shared_ptr<GameSkipDrawSink>,std::shared_ptr<NativeGameSkip>&);
    ~NativeGameSkip();
    NativeGameSkip(const NativeGameSkip&)=delete;
    NativeGameSkip& operator=(const NativeGameSkip&)=delete;
    // These mirror the constructor+publication and explicit null stores in the
    // caller. They do not claim to construct or finalize ChapterSequence.
    GameSkipStatus ConstructAndPublish(GameSkipHandle&);
    GameSkipStatus ConstructAndPublish(ProcessAccess&,GameSkipHandle&);
    GameSkipStatus PublishAbsent();
    GameSkipStatus PublishAbsent(ProcessAccess&);
    GameSkipStatus Capture(GameSkipControlHandle&);
    GameSkipStatus Capture(ProcessAccess&,GameSkipControlHandle&);
    GameSkipStatus ReleaseControl(GameSkipControlHandle);
    GameSkipStatus ReleaseControl(ProcessAccess&,GameSkipControlHandle);
    // Destroy finishes real nested process deletion and retires that owner. The
    // caller must subsequently publish absence, as the original Chapter does.
    ProcessStatus Begin(GameSkipHandle,GameSkipOperation);
    ProcessStatus BeginControl(GameSkipControlHandle,GameSkipControlOperation);
    std::optional<ProcessCall> Call(ProcessHandle,GameSkipHandle,GameSkipOperation) const;
    std::optional<ProcessCall> ControlCall(ProcessHandle,GameSkipControlHandle,GameSkipControlOperation) const;
    std::optional<GameSkipHandle> Current() const;
    std::optional<GameSkipSnapshot> Observe(GameSkipHandle) const;
    std::optional<GameSkipControlSnapshot> ObserveControl(GameSkipControlHandle) const;
    std::vector<ProcessHandle> RenderProcesses() const;
    std::optional<bool> ControlIsSkip(GameSkipControlHandle) const;
    std::optional<bool> ControlIsWait(GameSkipControlHandle) const;
    std::optional<bool> ControlIsEscape(GameSkipControlHandle) const;
    bool UsesScheduler(const NativeProcessScheduler&) const noexcept;
    // ProcEvent::FadeEnd's writes to the current skip object: flags|=2, and
    // black saved goals while skipping. Does not change the skip state.
    GameSkipStatus PrepareEventFadeEnd(ProcessAccess&,GameSkipHandle,
        bool& skipping,bool& blackout);
    std::unique_ptr<ProcessContinuation> Begin(const ProcessCall&) override;
private:
    struct State;
    struct Continuation;
    explicit NativeGameSkip(std::shared_ptr<State>);
    std::shared_ptr<State> state_;
};
}
