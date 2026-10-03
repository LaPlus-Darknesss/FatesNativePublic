#pragma once
#include "fates/runtime/native_talk_control_effects.hpp"

namespace fates::runtime::native {
enum class TalkMessageWidthStatus:std::uint8_t {
    Ready,NullScheduler,MismatchedDomain,DuplicateBinding,Busy,Retired,InvalidRequest,
    InvalidWindow,Unavailable,InvalidSource,InvalidControl,ReadLimit,IdentityExhausted
};
struct TalkWidthIdentity final {const std::uint32_t serial;};
using TalkWidthRequest=std::shared_ptr<const TalkWidthIdentity>;
struct TalkWidthObservation {
    TalkWidthRequest identity;TalkMessageWidthStatus status{TalkMessageWidthStatus::Ready};
    bool completed{};std::optional<std::uint32_t> result;
    std::optional<std::size_t> cursor{std::size_t{0}};
    std::uint32_t line_width{},maximum{};
    std::size_t reads{},controls{};
};
struct TalkWindowArgumentObservation {
    TalkMessageWidthStatus status{TalkMessageWidthStatus::Ready};bool completed{};
    std::array<std::uint8_t,64> fid{};std::uint8_t location{};
};
// Resumable exact GetFidForWindow helper used by IsEndCalcWidth. It writes the
// dedicated65-word array7531F4, distinct from GetToken752FE0. Both arrays
// share the existing token owner; empty arguments use a literal and clear neither.
// Local output is the reached 64-byte FID scratch, not a new filename cache.
struct TalkWindowArgumentState final {
    TalkWindowArgumentObservation value;
    TalkMessageWidthStatus Step(const NativeTalkTokens::Reader&,std::size_t,NativeTalkTokens&,const NativeShiftJis&);
private:
    unsigned stage{};std::size_t source_index{},copied{},length{};bool shared{};
    NativeShiftJis::WordReader TokenReader(NativeTalkTokens&) const;
};
// Original CalculateMessageWidth: saves/restores font selector, visits controls
// through their IsEnd/Skip routes, measures COPIED single UTF16 words, returns
// signed-running maximum+16. This does not Dispose controls, create/open a window,
// synthesize face readiness, render glyphs, or advance a ProcTalkManager program.
class NativeTalkMessageWidth final:public ProcessCallbacks {
public:
    using Reader=NativeTalkTokens::Reader;
    static TalkMessageWidthStatus Create(std::shared_ptr<NativeProcessScheduler>,
        std::shared_ptr<ProcessCallbackRegistry>,std::shared_ptr<NativeTalkControlContext>,
        std::shared_ptr<presentation::native::NativeTalkWindow>,
        std::shared_ptr<presentation::native::NativeFontMetrics>,std::shared_ptr<NativeTalkTokens>,
        std::shared_ptr<const NativeShiftJis>,std::shared_ptr<NativeTalkMessageWidth>&);
    ~NativeTalkMessageWidth();
    NativeTalkMessageWidth(const NativeTalkMessageWidth&)=delete;
    NativeTalkMessageWidth& operator=(const NativeTalkMessageWidth&)=delete;
    TalkMessageWidthStatus Prepare(ProcessHandle,presentation::native::TalkWindowHandle,
        presentation::native::TalkWindowSource,std::size_t,TalkWidthRequest&,ProcessAccess* =nullptr);
    // Same serialized live-reader contract as NativeFontMetrics; no byte-copy
    // snapshot substituted for borrowed message/expander storage. During a reused
    // Skip invocation the inherited serialized read-only scanner contract applies;
    // arbitrary mutation inside its individual redundant loads is not admitted.
    TalkMessageWidthStatus PrepareReader(ProcessHandle,presentation::native::TalkWindowHandle,
        Reader,std::size_t,TalkWidthRequest&,ProcessAccess* =nullptr);
    std::optional<ProcessCall> Call(TalkWidthRequest) const;
    std::optional<TalkWidthObservation> Observe(TalkWidthRequest) const;
    TalkMessageWidthStatus Release(TalkWidthRequest,ProcessAccess* =nullptr);
    void Forget(TalkWidthRequest) noexcept; // Host request cancellation, never a game-state operation.
    bool UsesOwners(const NativeTalkControlContext&,const presentation::native::NativeTalkWindow&) const noexcept;
    bool UsesTokens(const NativeTalkTokens&) const noexcept;
    bool UsesScheduler(const NativeProcessScheduler&) const noexcept;
    std::unique_ptr<ProcessContinuation> Begin(const ProcessCall&) override;
private:
    struct State;struct Continuation;explicit NativeTalkMessageWidth(std::shared_ptr<State>);
    std::shared_ptr<State> state_;
};
}
