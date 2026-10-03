#pragma once
#include "fates/runtime/native_game_skip.hpp"

namespace fates::runtime::native {
// ProcWait type1 reads a second original clock (006DCA90), not the scheduler's
// RecursiveTick delta (006DCA9C). A missing clock is unavailable, never a default.
class TalkWaitClockSource {
public:
    virtual ~TalkWaitClockSource()=default;
    virtual std::optional<std::uint32_t> AlternateFrameDelta() const=0;
};
enum class TalkWaitStatus:std::uint8_t {
    Ready,NullScheduler,MismatchedDomain,DuplicateBinding,Busy,Retired,InvalidParent,InvalidHandle
};
struct TalkWaitSnapshot {
    ProcessHandle process;
    std::uint32_t remaining{}; // Original wrapped signed32 word.
    std::uint8_t type{};
    std::uint64_t deletion_requests{}; // Observation only; never a completion source.
};
// Real Default-program blocking children for TalkUtil::ProcWait. They have
// independent clocks and real deferred Delete/Destroy through the shared native
// scheduler. This is not the key-wait handler or ProcTalkManager implementation.
class NativeTalkWait final:public ProcessCallbacks {
public:
    static TalkWaitStatus Create(std::shared_ptr<NativeProcessScheduler>,
        std::shared_ptr<ProcessCallbackRegistry>,std::shared_ptr<NativeGameSkip>,
        std::shared_ptr<TalkWaitClockSource>,std::shared_ptr<NativeTalkWait>&);
    ~NativeTalkWait();
    NativeTalkWait(const NativeTalkWait&)=delete;
    NativeTalkWait& operator=(const NativeTalkWait&)=delete;
    TalkWaitStatus Bind(ProcessHandle,std::int32_t milliseconds,std::uint32_t type,ProcessHandle&);
    TalkWaitStatus Bind(ProcessAccess&,ProcessHandle,std::int32_t milliseconds,std::uint32_t type,ProcessHandle&);
    TalkWaitStatus RestoreCarried(ProcessHandle,std::uint32_t remaining,std::uint8_t type);
    std::optional<TalkWaitSnapshot> Observe(ProcessHandle) const;
    std::vector<ProcessHandle> Processes() const;
    bool UsesGameSkip(const NativeGameSkip&) const noexcept;
    bool UsesScheduler(const NativeProcessScheduler&) const noexcept;
    std::unique_ptr<ProcessContinuation> Begin(const ProcessCall&) override;
private:
    struct State;struct Continuation;
    explicit NativeTalkWait(std::shared_ptr<State>);
    std::shared_ptr<State> state_;
};
}
