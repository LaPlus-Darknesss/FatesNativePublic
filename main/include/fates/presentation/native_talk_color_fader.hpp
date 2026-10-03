#pragma once
#include "fates/presentation/native_talk_color.hpp"
namespace fates::presentation::native {
// Original ColorCurve table profiles0..4: linear, degree2 accel, degree2 decel,
// byte-midpoint accel/decel, triangular repeat. Color conversion is VFP unsigned
// saturation followed by low-byte extraction, NOT clamp-to-255 or host uint8 cast.
bool TalkColorCurveExact(TalkColorBytes origin,TalkColorBytes destination,std::uint32_t elapsed,std::uint32_t duration,std::uint8_t curve,TalkColorBytes&) noexcept;
enum class TalkColorFaderStatus:std::uint8_t {Ready,NullScheduler,MismatchedDomain,DuplicateBinding,Busy,Retired,InvalidParent,InvalidHandle,Unavailable};
struct TalkColorFaderObservation {
    runtime::native::ProcessHandle process;
    runtime::native::ObjectHandle target;
    std::optional<TalkColorBytes> origin;
    TalkColorBytes destination{};
    std::uint32_t duration{},elapsed{};
    std::uint8_t curve{},channels{};
    std::uint64_t deletion_requests{};
};
// Fresh ProcColorFader constructors set their optional completion-callback pointer
// to known-null. Callback injection used by other engine consumers remains outside
// this owner. Live colors, flags/time, blocking relationships and destruction use
// the existing owner/scheduler; no fake window completion or alternate dispatcher.
class NativeTalkColorFader final:public runtime::native::ProcessCallbacks {
public:
    static TalkColorFaderStatus Create(std::shared_ptr<runtime::native::NativeProcessScheduler>,
        std::shared_ptr<runtime::native::ProcessCallbackRegistry>,std::shared_ptr<runtime::native::ObjectHandleRegistry>,std::shared_ptr<NativeTalkColorFader>&);
    ~NativeTalkColorFader();
    NativeTalkColorFader(const NativeTalkColorFader&)=delete;
    NativeTalkColorFader& operator=(const NativeTalkColorFader&)=delete;
    TalkColorFaderStatus Bind(runtime::native::ProcessHandle,std::shared_ptr<TalkColorOwner>,runtime::native::ObjectHandle,
        TalkColorBytes,std::int32_t milliseconds,std::uint32_t curve,std::uint32_t channels,bool blocking,runtime::native::ProcessHandle&,
        runtime::native::ProcessAccess* =nullptr);
    TalkColorFaderStatus RestoreClock(runtime::native::ProcessHandle,std::uint32_t elapsed,std::uint32_t duration);
    std::optional<TalkColorFaderObservation> Observe(runtime::native::ProcessHandle) const;
    std::vector<runtime::native::ProcessHandle> Processes() const;
    bool UsesObjectRegistry(const runtime::native::ObjectHandleRegistry&) const noexcept;
    bool UsesScheduler(const runtime::native::NativeProcessScheduler&) const noexcept;
    std::unique_ptr<runtime::native::ProcessContinuation> Begin(const runtime::native::ProcessCall&) override;
private:
    struct State;struct Continuation;
    explicit NativeTalkColorFader(std::shared_ptr<State>);
    std::shared_ptr<State> state_;
};
}
