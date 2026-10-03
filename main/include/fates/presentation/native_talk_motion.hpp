#pragma once
#include "fates/presentation/native_talk_window.hpp"
namespace fates::presentation::native {
// Reached degree2 vector profiles: linear0, acceleration1, deceleration2,
// split acceleration/deceleration3 and its inverse4. Preserve binary32 order.
// Unsupported profiles/nonfinite arithmetic remain explicit barriers.
bool TalkVectorCurveExact(TalkVectorBits origin,TalkVectorBits target,std::uint32_t elapsed,std::uint32_t duration,std::uint8_t curve,TalkVectorBits&) noexcept;
enum class TalkMotionStatus:std::uint8_t {Ready,NullScheduler,MismatchedDomain,DuplicateBinding,Busy,Retired,InvalidParent,InvalidHandle,Unavailable};
struct TalkMotionObservation {
    runtime::native::ProcessHandle process;
    runtime::native::ObjectHandle target;
    std::optional<TalkVectorBits> origin;
    TalkVectorBits destination{};
    std::uint32_t duration{},elapsed{};
    std::uint8_t curve{},fade{};
    std::optional<TalkWindowView> string;
    std::uint64_t deletion_requests{};
};
// ProcCarrier and TalkStringScroll share the existing registered movable object,
// scheduler, Default program and actual deferred Delete/Destroy. NextPage owns
// no independent clock and binds actual blocking children, not an immediate clear.
class NativeTalkMotion final:public runtime::native::ProcessCallbacks {
public:
    static TalkMotionStatus Create(std::shared_ptr<runtime::native::NativeProcessScheduler>,
        std::shared_ptr<runtime::native::ProcessCallbackRegistry>,std::shared_ptr<runtime::native::ObjectHandleRegistry>,
        std::shared_ptr<NativeTalkWindow>,std::shared_ptr<NativeTalkMotion>&);
    ~NativeTalkMotion();
    NativeTalkMotion(const NativeTalkMotion&)=delete;
    NativeTalkMotion& operator=(const NativeTalkMotion&)=delete;
    TalkMotionStatus BindCarrier(runtime::native::ProcessHandle,std::shared_ptr<TalkPositionOwner>,runtime::native::ObjectHandle,TalkVectorBits,std::int32_t,std::uint32_t,bool,runtime::native::ProcessHandle&,runtime::native::ProcessAccess* =nullptr);
    TalkMotionStatus BindScroll(runtime::native::ProcessHandle,TalkWindowView,TalkVectorBits,std::int32_t,std::uint32_t,std::uint8_t,runtime::native::ProcessHandle&,runtime::native::ProcessAccess* =nullptr);
    std::optional<runtime::native::ProcessCall> NextPageCall(runtime::native::ProcessHandle,TalkWindowHandle) const;
    TalkMotionStatus RestoreClock(runtime::native::ProcessHandle,std::uint32_t elapsed,std::uint32_t duration);
    std::optional<TalkMotionObservation> Observe(runtime::native::ProcessHandle) const;
    std::vector<runtime::native::ProcessHandle> Processes() const;
    bool UsesWindow(const NativeTalkWindow&) const noexcept;
    bool UsesScheduler(const runtime::native::NativeProcessScheduler&) const noexcept;
    std::unique_ptr<runtime::native::ProcessContinuation> Begin(const runtime::native::ProcessCall&) override;
private:
    struct State;struct Continuation;
    explicit NativeTalkMotion(std::shared_ptr<State>);
    std::shared_ptr<State> state_;
};
}
