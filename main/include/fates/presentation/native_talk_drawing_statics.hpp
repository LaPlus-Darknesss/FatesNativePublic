#pragma once
#include "fates/presentation/native_talk_color.hpp"
#include "fates/presentation/native_talk_position.hpp"

namespace fates::presentation::native {
enum class TalkDrawingStaticStatus:std::uint8_t {
    Ready,NullScheduler,MismatchedDomain,DuplicateBinding,Busy,Retired,Unavailable,
    InvalidState,InvalidRequest,IdentityExhausted,Cancelled
};
struct TalkDrawingStaticStorage {
    // Shared original guard6FECB0 / vector77973C. Even nonzero guard words
    // suppress construction despite bit0 being clear; do not normalize them.
    std::optional<std::uint32_t> zero_guard;
    std::array<std::optional<std::uint32_t>,3> zero_vector;
    // Startup596734 writes palette6DD098 and eleven vectors75359C. It does
    // not initialize/reset the separate shared lazy vector or its guard.
    std::array<std::optional<TalkColorBytes>,12> palette;
    std::array<std::optional<TalkVectorBits>,11> shout_offsets;
};
struct TalkDrawingOffsetIdentity final {const std::uint32_t serial;};
using TalkDrawingOffsetRequest=std::shared_ptr<const TalkDrawingOffsetIdentity>;
struct TalkDrawingOffsetObservation {
    TalkDrawingOffsetRequest identity;TalkDrawingStaticStatus status{TalkDrawingStaticStatus::Ready};
    bool active{},completed{},cancelled{};std::optional<TalkVectorBits> result;
};
// Shared mutable presentation globals, not a copied table inside each window.
// Executes the actual startup and GetShoutAnimTextOffset entries. The simple
// Thumb guard302C6C has no TLS/OS dependency. Other owners can use the same
// lazy-vector storage through EnsureZeroVector/ReadZeroWord at reached sites.
class NativeTalkDrawingStatics final:public runtime::native::ProcessCallbacks {
public:
    static TalkDrawingStaticStatus Create(std::shared_ptr<runtime::native::NativeProcessScheduler>,
        std::shared_ptr<runtime::native::ProcessCallbackRegistry>,std::shared_ptr<NativeTalkDrawingStatics>&);
    ~NativeTalkDrawingStatics();
    NativeTalkDrawingStatics(const NativeTalkDrawingStatics&)=delete;
    NativeTalkDrawingStatics& operator=(const NativeTalkDrawingStatics&)=delete;
    // Explicit fresh zero-initialized executable storage; cannot be repeated.
    TalkDrawingStaticStatus ConstructFreshStorage(runtime::native::ProcessAccess* =nullptr);
    TalkDrawingStaticStatus Restore(TalkDrawingStaticStorage,runtime::native::ProcessAccess* =nullptr);
    std::optional<TalkDrawingStaticStorage> ObserveStorage() const;
    static runtime::native::ProcessCall InitializeCall(runtime::native::ProcessHandle);
    TalkDrawingStaticStatus PrepareOffset(std::int32_t,TalkDrawingOffsetRequest&,runtime::native::ProcessAccess* =nullptr);
    std::optional<runtime::native::ProcessCall> OffsetCall(runtime::native::ProcessHandle,TalkDrawingOffsetRequest) const;
    std::optional<TalkDrawingOffsetObservation> Observe(TalkDrawingOffsetRequest) const;
    TalkDrawingStaticStatus Release(TalkDrawingOffsetRequest,runtime::native::ProcessAccess* =nullptr);
    TalkDrawingStaticStatus EnsureZeroVector(runtime::native::ProcessAccess&);
    std::optional<std::uint32_t> ReadZeroWord(std::size_t) const;
    std::optional<TalkColorBytes> ReadPalette(std::size_t) const;
    std::uint64_t guard_calls() const noexcept;
    bool UsesScheduler(const runtime::native::NativeProcessScheduler&) const noexcept;
    std::unique_ptr<runtime::native::ProcessContinuation> Begin(const runtime::native::ProcessCall&) override;
private:
    friend class NativeTalkWindowDrawing;
    void ForgetOffset(TalkDrawingOffsetRequest);
    struct State;struct Continuation;
    explicit NativeTalkDrawingStatics(std::shared_ptr<State>);
    std::shared_ptr<State> state_;
};
}
