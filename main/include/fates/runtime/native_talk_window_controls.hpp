#pragma once
#include "fates/runtime/native_talk_message_width.hpp"
namespace fates::runtime::native {
struct TalkWindowControlIdentity final {const std::uint32_t serial;};
using TalkWindowControlRequest=std::shared_ptr<const TalkWindowControlIdentity>;
struct TalkWindowControlObservation {
    TalkWindowControlRequest identity;ProcessHandle manager;
    TalkControlStatus status{TalkControlStatus::Ready};
    bool completed{};std::optional<std::uint32_t> result,measured_width;
};
// TcdiWindow Dispose/Flash and reached manager/SetActive composition. Uses the
// actual already-owned constructor-null-face windows. Not a FaceManager factory.
// Unmatched selection observes the existing GameSkip singleton and falls back
// to the first window. Wm admits the existing inactive same-location shortcut. Wf's null-face path
// is a genuine no-op. Every other skipped dependency stays a barrier.
//
// +5D3 means open, +5D5 means active speaker. Clearing the log's talker precedes
// a0->1 speaker write. Mode1 closes other windows before their speaker write;
// other modes StopAllVoice(100) first. The final current window is reread.
//
// Wo snapshots manager+38 and its current target for measurement, but rereads
// the current window after font restoration before StartOpen. Measurement
// delegates to the exact existing CalculateMessageWidth owner.
class NativeTalkWindowControls final:public ProcessCallbacks {
public:
    static TalkControlStatus Create(std::shared_ptr<NativeProcessScheduler>,
        std::shared_ptr<ProcessCallbackRegistry>,std::shared_ptr<NativeTalkControlContext>,
        std::shared_ptr<presentation::native::NativeTalkWindow>,
        std::shared_ptr<presentation::native::NativeTalkWindowEffects>,
        std::shared_ptr<NativeTalkMessageWidth>,std::shared_ptr<NativeGameSkip>,std::shared_ptr<NativeTalkLog>,
        std::shared_ptr<NativeTalkTokens>,std::shared_ptr<const NativeShiftJis>,
        std::shared_ptr<NativeTalkWindowControls>&);
    ~NativeTalkWindowControls();
    NativeTalkWindowControls(const NativeTalkWindowControls&)=delete;
    NativeTalkWindowControls& operator=(const NativeTalkWindowControls&)=delete;
    TalkControlStatus Prepare(ProcessHandle,presentation::native::TalkWindowSource,std::size_t,TalkCodeOperation,
        TalkWindowControlRequest&,ProcessAccess* =nullptr);
    std::optional<ProcessCall> Call(TalkWindowControlRequest) const;
    std::optional<TalkWindowControlObservation> Observe(TalkWindowControlRequest) const;
    TalkControlStatus Release(TalkWindowControlRequest,ProcessAccess* =nullptr);
    void Forget(TalkWindowControlRequest) noexcept; // Host request bookkeeping only.
    bool UsesOwners(const NativeTalkControlContext&,const presentation::native::NativeTalkWindow&,
                    const NativeTalkTokens&,const NativeTalkLog&,const NativeGameSkip&) const noexcept;
    std::unique_ptr<ProcessContinuation> Begin(const ProcessCall&) override;
private:
    struct State;struct Continuation;
    explicit NativeTalkWindowControls(std::shared_ptr<State>);
    std::shared_ptr<State> state_;
};
}
